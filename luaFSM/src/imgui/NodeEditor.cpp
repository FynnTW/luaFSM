#include "pch.h"
#include "NodeEditor.h"

#include <fstream>

#include "ImGuiNotify.hpp"
#include "Graphics/Window.h"
#include "IO/FileReader.h"

namespace LuaFsm
{

    NodeEditor* NodeEditor::m_Instance = nullptr;
    
    NodeEditor::NodeEditor()
    {
        m_Instance = this;
    }

    NodeEditor::~NodeEditor()
    = default;

    ImVec2 normalize(const ImVec2& vec)
    {
        const float length = std::sqrt(vec.x * vec.x + vec.y * vec.y);
        if (length == 0.0f)
            return {0, 0};
        return {vec.x / length, vec.y / length};
    }
    
    void NodeEditor::DrawLine(const ImVec2 fromPos, const ImVec2 toPos, ImU32 color,
    const float thickness, float arrowHeadWidth, float arrowHeadLength,
    const float curve, VisualNode* fromNode, VisualNode* targetNode
    )
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const auto scale = Get()->GetScale();
        arrowHeadWidth *= scale;
        arrowHeadLength *= scale;
        color = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]);

        const ImVec2 direction = toPos - fromPos;
        const ImVec2 normalizedDirection = normalize(direction);
        const ImVec2 perpendicular(-normalizedDirection.y, normalizedDirection.x);

        const float lineLength = Math::Distance(fromPos, toPos);
        const ImVec2 midPoint = fromPos + normalizedDirection * (lineLength * 0.5f);
        if (fromNode && fromNode->GetType() == NodeType::Transition)
        {
            color = ImColor(155, 255, 155, 255);
            fromNode->SetOutLineMidPoint(midPoint);
        }
        if (targetNode && targetNode->GetType() == NodeType::Transition)
        {
            color = ImColor(255, 155, 155, 255);
            targetNode->SetInLineMidPoint(midPoint);
        }
        ImVec2 arrowTip = toPos;
        ImVec2 leftCorner, rightCorner;

        if (curve > 0.01f || curve < -0.01f && fromNode && targetNode)
        {
            const ImVec2 controlPoint = midPoint + perpendicular * (lineLength * curve);
            if (fromNode->GetType() == NodeType::Transition)
            {
                color = ImColor(155, 255, 155, 255);
                fromNode->SetOutLineMidPoint(controlPoint);
            }
            if (targetNode->GetType() == NodeType::Transition)
            {
                color = ImColor(255, 155, 155, 255);
                targetNode->SetInLineMidPoint(controlPoint);
            }
            const ImVec2 newFromPos = fromNode->GetFromPoint(controlPoint);
            const ImVec2 newToPos = targetNode->GetFromPoint(controlPoint);
            targetNode->SetLastConnectionPoint(newToPos);

            // Calculate the tangent at the end of the Bezier curve
            const ImVec2 tangent = normalize(newToPos - controlPoint);

            // Adjust the arrowhead direction to align with the tangent
            arrowTip = newToPos;
            leftCorner = (arrowTip - tangent * arrowHeadLength) + (ImVec2(-tangent.y, tangent.x) * arrowHeadWidth);
            rightCorner = (arrowTip - tangent * arrowHeadLength) - (ImVec2(-tangent.y, tangent.x) * arrowHeadWidth);

            drawList->PathLineTo(newFromPos);
            drawList->PathBezierCubicCurveTo(controlPoint, controlPoint, newToPos);
            drawList->PathStroke(color, false, thickness);
        }
        else
        {
            drawList->AddLine(fromPos, toPos, color, thickness);
            if (targetNode)
                targetNode->SetLastConnectionPoint(toPos);

            // For a straight line, use the original direction for the arrowhead
            leftCorner = (arrowTip - normalizedDirection * arrowHeadLength) + (perpendicular * arrowHeadWidth);
            rightCorner = (arrowTip - normalizedDirection * arrowHeadLength) - (perpendicular * arrowHeadWidth);
        }

        // Draw the arrow head
        drawList->AddTriangleFilled(arrowTip, leftCorner, rightCorner, color);
    }
    void NodeEditor::DrawConnection(VisualNode* fromNode, VisualNode* toNode)
    {
        const auto fromPos = fromNode->GetFromPoint(toNode);
        const auto toPos = toNode->GetFromPoint(fromNode);
        float curve;
        if (fromNode->GetType() == NodeType::Transition)
        {
            curve = fromNode->GetOutArrowCurve();
        }
        else
        {
            curve = toNode->GetInArrowCurve();
        }
        DrawLine(fromPos,
                 toPos,
                 ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]),
                 2.0f,
                 10.0f,
                 15.0f,
                 curve,fromNode,toNode);
    }

    void NodeEditor::DeserializeSettings(const nlohmann::json& settings)
    {
        if (settings.contains("appendToFile"))
            m_AppendStates = settings["appendToFile"];
        if (settings.contains("showPriority"))
            m_ShowPriority = settings["showPriority"];
        if (settings.contains("defaultPath"))
            FileReader::lastPath = settings["defaultPath"];
        if (settings.contains("defaultTheme"))
            Window::SetTheme(settings["defaultTheme"]);
    }

    nlohmann::json NodeEditor::SerializeSettings() const
    {
        nlohmann::json settings;
        settings["appendToFile"] = m_AppendStates;
        settings["showPriority"] = m_ShowPriority;
        settings["defaultPath"] = FileReader::lastPath;
        settings["defaultTheme"] = Window::GetActiveTheme();
        return settings;
    }

    void NodeEditor::SaveSettings() const
    {
        FileReader::SaveFile("settings.json", SerializeSettings().dump(4));
    }

    void NodeEditor::LoadSettings()
    {
        if (!FileReader::FileExists("settings.json"))
            return;
        const auto code = FileReader::ReadAllText("settings.json");
        if (code.empty())
            return;
        DeserializeSettings(nlohmann::json::parse(code));
    }
    
    void NodeEditor::ExportLua(const std::string& filePath) const
    {
        if (!m_Fsm)
            return;
        std::ofstream file(filePath);
        if (!file.is_open())
        {
            ImGui::InsertNotification({ImGuiToastType::Error, 3000, "Failed to export file at: %s", filePath.c_str()});
            return;
        }
        file << m_Fsm->GetLuaCode();
        file.close();
        m_Fsm->SetLinkedFile(filePath);
        ImGui::InsertNotification({ImGuiToastType::Success, 3000, "Exported file at: %s", filePath.c_str()});
    }

    IdValidityError NodeEditor::CheckIdValidity(const std::string& id) const
    {
        if (id.empty())
            return IdValidityError::EmptyId;
        if (std::regex_search(id, FsmRegex::InvalidIdRegex()))
            return IdValidityError::InvalidId;
        if (m_Fsm)
        {
            if (m_Fsm->GetState(id))
                return IdValidityError::DuplicateIdState;
            if (m_Fsm->GetTrigger(id))
                return IdValidityError::DuplicateIdTrigger;
        }
        return IdValidityError::Valid;
    }

    void NodeEditor::InformValidityError(const IdValidityError error)
    {
        if (error == IdValidityError::Valid)
            return;
        if (error == IdValidityError::EmptyId)
        {
            ImGui::Text("FSM ID cannot be empty");
        }
        else if (error == IdValidityError::InvalidId)
        {
            ImGui::Text("Invalid FSM ID");
        }
        else if (error == IdValidityError::DuplicateIdState)
        {
            ImGui::Text("State with this ID already exists");
        }
        else if (error == IdValidityError::DuplicateIdTrigger)
        {
            ImGui::Text("Trigger with this ID already exists");
        }
    }

    void NodeEditor::MoveToNode(const std::string& id, const NodeType type)
    {
        VisualNode* node = GetNode(id, type);
        if (!node)
            return;
        SetSelectedNode(node);
        SetCanvasScroll(node->GetGridPos() - GetCanvasSize() / 2);
    }

    void NodeEditor::DeselectAllNodes()
    {
        if (!m_Fsm)
            return;
        for (const auto& [key, state] : m_Fsm->GetStates())
            state->GetNode()->Deselect();
        for (const auto& [key, trigger] : m_Fsm->GetTriggers())
            trigger->GetNode()->Deselect();
        m_SelectedNode = nullptr;
        m_ShowFsmProps = true;
    }

    VisualNode* NodeEditor::GetNode(const std::string& id, const NodeType type) const
    {
        if (!m_Fsm)
            return nullptr;
        switch (type)
        {
        case NodeType::State:
            if (m_Fsm->GetState(id))
                return m_Fsm->GetState(id)->GetNode();
            break;
        case NodeType::Transition:
            if (m_Fsm->GetTrigger(id))
                return m_Fsm->GetTrigger(id)->GetNode();
            break;
        case NodeType::Fsm:
            break;
        }
        return nullptr;
    }

    void NodeEditor::SetSelectedNode(VisualNode* node)
    {
        DeselectAllNodes();
        m_ShowFsmProps = false;
        if (!node)
            return;
        node->Select();
        m_SelectedNode = node;
    }
}
