#include "pch.h"
#include "NodeEditor.h"

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

    void NodeEditor::DrawLine(const ImVec2 fromPos, const ImVec2 toPos, const ImU32 color, const float thickness, const float arrowHeadWidth, const float arrowHeadLength)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 direction = Math::SubtractVec2(toPos, fromPos);
        const ImVec2 normalizedDirection = normalize(direction);
        const ImVec2 perpendicular(-normalizedDirection.y, normalizedDirection.x);
        // Arrow parameters
        const ImVec2 arrowTip = toPos;
        const ImVec2 leftCorner = Math::AddVec2(
            Math::SubtractVec2(
            arrowTip,
            Math::MultiplyVec2(normalizedDirection, arrowHeadLength)),
            Math::MultiplyVec2(perpendicular, arrowHeadWidth));
        const ImVec2 rightCorner = Math::SubtractVec2(
            Math::SubtractVec2(
            arrowTip,
            Math::MultiplyVec2(normalizedDirection, arrowHeadLength)),
            Math::MultiplyVec2(perpendicular, arrowHeadWidth));
                        
        drawList->AddLine(fromPos, toPos, color, thickness);
        drawList->AddTriangleFilled(arrowTip, leftCorner, rightCorner, color);
    }

    void NodeEditor::DrawConnection(VisualNode* fromNode, VisualNode* toNode)
    {
        const auto fromPos = fromNode->GetFromPoint(toNode);
        const auto toPos = toNode->GetFromPoint(fromNode);
        DrawLine(fromPos, toPos);
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
        node->Select();
        m_SelectedNode = node;
        m_ShowFsmProps = false;
    }
}
