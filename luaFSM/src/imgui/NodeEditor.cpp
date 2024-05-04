#include "pch.h"
#include "NodeEditor.h"

namespace LuaFsm
{
    int NodeEditor::AddNode(const std::string& name)
    {
        return 0;
    }

    void NodeEditor::AddNode(VisualNode* node)
    {
        m_Nodes[node->GetId()] = node;
    }

    ImVec2 NodeEditor::GetNextNodePos()
    {
        if (m_LastNodePos.x < 1 && m_LastNodePos.y < 1)
            m_LastNodePos = MathHelpers::AddVec2(m_CanvasPos, ImVec2(50, 100));
        m_LastNodePos.x += m_NodeSize.x + m_NodeRadius * 2;
        if (m_LastNodePos.x > GetCanvasSize().x)
        {
            m_LastNodePos.x = 0;
            m_LastNodePos.y += m_NodeSize.y + m_NodeRadius * 2;
        }
        return m_LastNodePos;
    }

    ImVec2 Normalize(const ImVec2& vec)
    {
        float length = std::sqrt(vec.x * vec.x + vec.y * vec.y);
        if (length == 0.0f)
            return ImVec2(0, 0);
        return ImVec2(vec.x / length, vec.y / length);
    }

    void NodeEditor::DrawConnection(VisualNode* fromNode, VisualNode* toNode)
    {
        auto fromPos = fromNode->GetFromPoint(toNode);
        auto toPos = toNode->GetFromPoint(fromNode);
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 direction = MathHelpers::SubtractVec2(toPos, fromPos);
        ImVec2 normalizedDirection = Normalize(direction);
        ImVec2 perpendicular(-normalizedDirection.y, normalizedDirection.x);
        // Arrow parameters
        float arrowHeadLength = 15.0f;
        float arrowHeadWidth = 10.0f;
        ImVec2 arrowTip = toPos;
        ImVec2 leftCorner = MathHelpers::AddVec2(
            MathHelpers::SubtractVec2(
            arrowTip,
            MathHelpers::MultiplyVec2(normalizedDirection, arrowHeadLength)),
            MathHelpers::MultiplyVec2(perpendicular, arrowHeadWidth));
        ImVec2 rightCorner = MathHelpers::SubtractVec2(
            MathHelpers::SubtractVec2(
            arrowTip,
            MathHelpers::MultiplyVec2(normalizedDirection, arrowHeadLength)),
            MathHelpers::MultiplyVec2(perpendicular, arrowHeadWidth));
                        
        drawList->AddLine(fromPos, toPos, IM_COL32_WHITE, 2.0f);
        drawList->AddTriangleFilled(arrowTip, leftCorner, rightCorner, IM_COL32_WHITE);
        
    }

    void NodeEditor::DeselectAllNodes()
    {
        for (const auto& [key, value] : m_Nodes)
            value->Deselect();
        m_SelectedNode = nullptr;
    }

    void NodeEditor::SetSelectedNode(VisualNode* node)
    {
        DeselectAllNodes();
        node->Select();
        m_SelectedNode = node;
    }
}
