#include "pch.h"
#include "NodeEditor.h"

#include <fstream>

#include "ImGuiNotify.hpp"
#include "Graphics/Window.h"

namespace LuaFsm
{

    NodeEditor* NodeEditor::m_Instance = nullptr;
    
    NodeEditor::NodeEditor()
    {
        m_Instance = this;
    }

    NodeEditor::~NodeEditor()
    = default;

    int NODE_COUNT = 0;

    ImVec2 NodeEditor::GetNextNodePos(VisualNode* node)
    {
        NODE_COUNT++;
        
        return {100 + (NODE_COUNT * 10.f), 50 + (NODE_COUNT * 10.f)};
        /*
        * 
        auto newPos = ImVec2(100, 0);
        if (m_LastNode)
            newPos = m_LastNodePos;
            //m_LastNodePos = MathHelpers::AddVec2(m_CanvasPos, ImVec2(50, 100));
        m_LastNodePos = newPos;
        m_LastNodePos.x += node->GetSize().x + 50;
        m_LastNodePos.y += node->GetSize().y + 25;
        if (m_LastNodePos.x > GetCanvasSize().x)
        {
            m_LastNodePos.x = 100;
            m_LastNodePos.y += node->GetSize().y * 2;
        }
        m_LastNode = node;
        newPos.x = std::max(newPos.x, 100.0f);
        newPos.y = std::max(newPos.y, 100.0f);
        return newPos;
         */
    }

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

        ImVec2 arrowTip = toPos;
        ImVec2 leftCorner, rightCorner;

        if (curve > 0.01f || curve < -0.01f && fromNode && targetNode)
        {
            ImVec2 controlPoint = midPoint + perpendicular * (lineLength * curve);
            ImVec2 newFromPos = fromNode->GetFromPoint(controlPoint);
            ImVec2 newToPos = targetNode->GetFromPoint(controlPoint);
            targetNode->SetLastConnectionPoint(newToPos);

            // Calculate the tangent at the end of the Bezier curve
            ImVec2 tangent = normalize(newToPos - controlPoint);

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
        auto fromPos = fromNode->GetFromPoint(toNode);
        auto toPos = toNode->GetFromPoint(fromNode);
        float curve;
        if (fromNode->GetType() == NodeType::Transition)
        {
            curve = fromNode->GetInArrowCurve();
        }
        else
        {
            curve = toNode->GetOutArrowCurve();
        }
        DrawLine(fromPos,
                 toPos,
                 ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]),
                 2.0f,
                 10.0f,
                 15.0f,
                 curve,fromNode,toNode);
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

    void NodeEditor::SaveFsm(const std::string& filePath) const
    {
        if (!m_Fsm)
            return;
        const nlohmann::json j = m_Fsm->Serialize();
        std::ofstream file(filePath);
        if (!file.is_open())
        {
            ImGui::InsertNotification({ImGuiToastType::Error, 3000, "Failed to save file at: %s", filePath.c_str()});
            return;
        }
        file << j.dump(4);
        file.close();
        ImGui::InsertNotification({ImGuiToastType::Success, 3000, "Saved file at: %s", filePath.c_str()});
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
        }
        return nullptr;
    }

    void NodeEditor::SetSelectedNode(VisualNode* node)
    {
        DeselectAllNodes();
        if (!node)
            return;
        node->Select();
        m_SelectedNode = node;
        m_ShowFsmProps = false;
    }
}
