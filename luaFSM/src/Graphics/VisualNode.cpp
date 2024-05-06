#include "pch.h"
#include "VisualNode.h"

#include "data/DrawableObject.h"
#include "imgui/NodeEditor.h"

namespace LuaFsm
{
    ImVec2 VisualNode::GetPosition()
    {
        if (m_TargetPosition.x < 1 && m_TargetPosition.y < 1)
            return m_TargetPosition;
        
        ImVec2 canvasPos = ImGui::GetWindowPos();
        const ImVec2 canvasSize = ImGui::GetWindowSize();
        const ImVec2 canvasEnd = ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y);
        // Adjust the node position for scrolling
        ImVec2 scrollingOffset = {ImGui::GetScrollX() / 5, ImGui::GetScrollY() / 5};
        auto adjustedPos = ImVec2(canvasPos.x + m_TargetPosition.x - scrollingOffset.x, canvasPos.y + m_TargetPosition.y  - scrollingOffset.y);
        const auto nodeEnd = ImVec2(adjustedPos.x + m_Size.x, adjustedPos.y + m_Size.y);

        // Check if the node is off-screen
        if (adjustedPos.x < canvasPos.x || nodeEnd.x > canvasEnd.x ||
            adjustedPos.y < canvasPos.y || nodeEnd.y > canvasEnd.y)
        {
            return {-1, -1}; // Node is off-screen, don't draw
        }
        return adjustedPos;
    }

    int WINDOW_COUNT = 0;

    VisualNode* VisualNode::Draw(const DrawableObject* object)
    {
        const auto editor = NodeEditor::Get();
        if (m_Shape == NodeShape::Ellipse)
            ImGui::SetNextWindowSize(InitSizesEllipse(object->GetName()), ImGuiCond_Always);
        else if (m_Shape == NodeShape::Diamond)
            ImGui::SetNextWindowSize(InitSizesDiamond2(), ImGuiCond_Always);
        else if (m_Shape == NodeShape::Circle)
            ImGui::SetNextWindowSize(InitSizes2(), ImGuiCond_Always);
        if (m_TargetPosition.x < 0)
            m_TargetPosition = editor->GetNextNodePos(this);
        ImVec2 adjustedPos = GetPosition();
        if (adjustedPos.x < 0)
        {
            SetInvisible(true);
            return nullptr;
        }
        //adjustedPos = Math::AddVec2(editor->GetCanvasPos(), adjustedPos);
        SetInvisible(false);
        bool force = false;
        if (adjustedPos.x < editor->GetCanvasPos().x)
        {
            adjustedPos.x = editor->GetCanvasPos().x;
            force = true;
        }
        if (adjustedPos.y < editor->GetCanvasPos().y)
        {
            adjustedPos.y = editor->GetCanvasPos().y;
            force = true;
        }
        if (adjustedPos.x + GetSize().x > editor->GetCanvasSize().x)
        {
            adjustedPos.x = editor->GetCanvasSize().x - GetSize().x;
            force = true;
        }
        if (adjustedPos.y + GetSize().y > editor->GetCanvasSize().y)
        {
            adjustedPos.y = editor->GetCanvasSize().y - GetSize().y;
            force = true;
        }
        if (!Math::ImVec2Equal(m_LastPosition, adjustedPos) || force)
            ImGui::SetNextWindowPos(adjustedPos);
        m_LastPosition = adjustedPos;
        if (m_WindowLabel == 0)
            m_WindowLabel = ++WINDOW_COUNT;
        const std::string label = "##Window" + std::to_string(m_WindowLabel);
        ImGui::Begin(label.c_str(), nullptr, NodeEditor::nodeWindowFlags);
        {
            auto drawList = ImGui::GetWindowDrawList();
            HandleSelection(editor);
            ImVec2 right = {};
            if (m_Shape == NodeShape::Ellipse)
            {
                drawList->AddEllipseFilled(GetDrawPos(),  m_EllipseRadius, GetCurrentColor());
                if (NodeEditor::Get()->GetCurrentFsm()->GetInitialState() == m_Id)
                    drawList->AddEllipse(GetDrawPos(), m_EllipseRadius, IM_COL32(0, 255, 0, 255));
                else
                    drawList->AddEllipse(GetDrawPos(), m_EllipseRadius, GetBorderColor());
                drawList->AddText(GetTextPos(object->GetName().c_str()), IM_COL32_WHITE, object->GetName().c_str());
            }
            else if (m_Shape == NodeShape::Diamond)
            {
                const auto center = GetDrawPos();
                const float width = GetSize().x;
                const float height = GetSize().y;
                const auto top = ImVec2(center.x, center.y - height / 2 + 3.0f);
                const auto bottom = ImVec2(center.x, center.y + height / 2 - 3.0f);
                const auto left = ImVec2(center.x - width / 2 + 3.0f, center.y);
                right = ImVec2(center.x + width / 2 - 3.0f, center.y);
                drawList->AddQuadFilled(left, top, right, bottom, GetCurrentColor());
                drawList->AddQuad(left, top, right, bottom, GetBorderColor());
            }
            else if (m_Shape == NodeShape::Circle)
            {
                drawList->AddCircleFilled(GetDrawPos(), m_Radius, GetCurrentColor());
                drawList->AddCircle(GetDrawPos(), m_Radius, GetBorderColor());
            }
            const auto windowPos = ImGui::GetWindowPos();
            const auto movedAmount =
                Math::SubtractVec2(windowPos, Math::AddVec2(adjustedPos, {ImGui::GetScrollX() / 5, ImGui::GetScrollY() / 5}));
            auto newPos = Math::AddVec2(GetTargetPosition(), movedAmount);
            newPos.x = std::max(editor->GetCanvasPos().x + 1, newPos.x);
            newPos.y = std::max(editor->GetCanvasPos().y + 1, newPos.y);
            SetTargetPosition(newPos);
            ImGui::End();
            if (m_Type == NodeType::Transition)
            {
                drawList = ImGui::GetWindowDrawList();
                drawList->AddText(
                    Math::AddVec2X(
                        Math::SubtractVec2Y(Math::AddVec2X(GetLastDrawPos(), m_Radius),
                            ImGui::CalcTextSize(object->GetName().c_str()).y * 0.5f),
                        3), IM_COL32_WHITE, object->GetName().c_str());
            }
        }
        return this;
    }

    void VisualNode::HandleSelection(NodeEditor* editor)
    {
        if (ImGui::IsWindowHovered())
        {
            HighLight();
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                editor->SetSelectedNode(this);
            if (editor->IsCreatingLink() && !IsSelected() && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
            {
                if (const auto otherNode = editor->GetSelectedNode(); otherNode && otherNode != this)
                {
                    switch (otherNode->GetType())
                    {
                        case NodeType::State:
                            {
                                if (m_Type != NodeType::Transition)
                                    return;
                                const auto trigger = editor->GetCurrentFsm()->GetTrigger(m_Id);
                                if (!trigger || trigger->GetCurrentState())
                                    return;
                                if (const auto state = editor->GetCurrentFsm()->GetState(otherNode->GetId()))
                                    state->AddTrigger(trigger);
                                break;
                            }
                        case NodeType::Transition:
                            {
                                if (m_Type != NodeType::State)
                                    return;
                                if (!editor->GetCurrentFsm()->GetState(m_Id))
                                    return;
                                if (const auto fsmTrigger = editor->GetCurrentFsm()->GetTrigger(otherNode->GetId()))
                                    fsmTrigger->SetNextState(m_Id);
                                break;
                            }
                    }
                }
                editor->SetCreatingLink(false);
            }
        }
        else if (!IsSelected())
            UnHighLight();
    }

    ImVec2 VisualNode::GetTextPos(const char* text)
    {
        const ImVec2 textSize = ImGui::CalcTextSize(text);
        return {GetDrawPos().x - textSize.x / 2, GetDrawPos().y - textSize.y / 2};
    }
}
