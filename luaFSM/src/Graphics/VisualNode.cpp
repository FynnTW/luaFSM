#include "pch.h"
#include "VisualNode.h"

#include "imgui_internal.h"
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
        adjustedPos = adjustedPos * NodeEditor::Get()->GetScale();
        // Check if the node is off-screen
        if (adjustedPos.x < canvasPos.x || nodeEnd.x > canvasEnd.x * (1 / NodeEditor::Get()->GetScale()) ||
            adjustedPos.y < canvasPos.y || nodeEnd.y > canvasEnd.y * (1 / NodeEditor::Get()->GetScale()))
        {
            return {-1, -1}; // Node is off-screen, don't draw
        }
        return adjustedPos;
    }

    
    ImVec2 VisualNode::InitSizes(const std::string& name)
    {
        const float scale = NodeEditor::Get()->GetScale();
        switch (m_Shape)
        {
            case NodeShape::Circle:
                {
                    m_Radius = 12.f * scale;
                    const auto sideLength = Math::GetCircleWindowSideLength(m_Radius);
                    m_Size = {sideLength + 1.0f, sideLength + 1.0f};
                    m_Center = m_Size / 2;
                    return m_Size;
                }
            case NodeShape::Ellipse:
                {
                    const float width = ImGui::CalcTextSize(name.c_str()).x + 50.0f;
                    const float height = ImGui::CalcTextSize(name.c_str()).y + 30.0f;
                    m_Size = {width + 10, height + 5};
                    m_EllipseRadius = {width / 2, height / 2};
                    m_Center = {m_Size.x / 2, m_Size.y / 2};
                    return m_Size;
                }
            case NodeShape::Diamond:
                {
                    m_Size = {65, 60};
                    m_Center = {m_Size.x / 2.f, m_Size.y / 2.f};
                    return m_Size;
                }
            case NodeShape::Square:
                {
                    return {0, 0};
                }
            case NodeShape::Triangle:
                {
                    return {0, 0};
                }
        }
        return {};
    }

    int WINDOW_COUNT = 0;
    
    VisualNode* VisualNode::Draw(const DrawableObject* object)
    {
        const auto editor = NodeEditor::Get();
        if (m_WindowLabel == 0)
            m_WindowLabel = ++WINDOW_COUNT;
        const std::string label = "##Window" + std::to_string(m_WindowLabel);
        ImGui::SetWindowFontScale(editor->GetScale());
        m_Size = InitSizes(object->GetName());
        ImGui::SetCursorPos(m_GridPos * editor->GetScale());
        ImGui::BeginChild(
            label.c_str(),
            m_Size,
            ImGuiChildFlags_None,NodeEditor::nodeWindowFlags);
        {
            //const ImVec2 canvasPos = ImGui::GetWindowPos();
            const ImVec2 canvasPos = ImGui::GetItemRectMin();
            auto drawList = ImGui::GetWindowDrawList();
            m_LastPosition = (canvasPos + m_Center);
            HandleSelection(editor);
            if (m_Shape == NodeShape::Ellipse)
            {
                drawList->AddEllipseFilled(m_LastPosition,  m_EllipseRadius, GetCurrentColor());
                if (NodeEditor::Get()->GetCurrentFsm()->GetInitialStateId() == m_Id)
                    drawList->AddEllipse(m_LastPosition, m_EllipseRadius, IM_COL32(0, 255, 0, 255), 0, 0, 2.f);
                else if (const auto state = NodeEditor::Get()->GetCurrentFsm()->GetState(m_Id); state && state->IsExitState())
                    drawList->AddEllipse(m_LastPosition, m_EllipseRadius, IM_COL32(255, 0, 0, 255), 0,0, 2.f);
                else
                    drawList->AddEllipse(m_LastPosition, m_EllipseRadius, GetBorderColor(), 0,0, 2.f);
                const auto textColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]);
                drawList->AddText(GetTextPos(object->GetName().c_str()), textColor, object->GetName().c_str());
            }
            else if (m_Shape == NodeShape::Circle)
            {
                drawList->AddCircleFilled(m_LastPosition, m_Radius, GetCurrentColor());
                if (NodeEditor::Get()->GetCurrentFsm()->GetInitialStateId() == m_Id)
                    drawList->AddCircle(m_LastPosition, m_Radius, IM_COL32(0, 255, 0, 255), 0, 2.f);
                else if (const auto state = NodeEditor::Get()->GetCurrentFsm()->GetState(m_Id); state && state->IsExitState())
                    drawList->AddCircle(m_LastPosition, m_Radius, IM_COL32(255, 0, 0, 255), 0, 2.f);
                else
                    drawList->AddCircle(m_LastPosition, m_Radius, GetBorderColor(), 0, 2.f);
            }
            ImGui::EndChild();
            if (m_Type == NodeType::Transition)
            {
                drawList = ImGui::GetWindowDrawList();
                const auto rightBound = Math::AddVec2X(GetLastDrawPos(), m_Radius);
                const auto leftBound = Math::SubtractVec2X(GetLastDrawPos(), m_Radius);
                const auto trigger = editor->GetCurrentFsm()->GetTrigger(m_Id);

                const auto textSize = ImGui::CalcTextSize(object->GetName().c_str());
                ImVec2 position;
                if ((m_LastConnectionPoint.x > GetLastDrawPos().x && abs(m_LastConnectionPoint.y - GetLastDrawPos().y) < m_Radius * 0.7))
                {
                    // Move text to the left
                    const auto xPos = Math::SubtractVec2X(leftBound, 3.0f + textSize.x);
                    position = Math::SubtractVec2Y(xPos, textSize.y * 0.5f);
                }
                else
                {
                    // Default position
                    const auto xPos = Math::AddVec2X(rightBound, 3.0f);
                    position = Math::SubtractVec2Y(xPos, textSize.y * 0.5f);
                }

                drawList->AddText(position, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]), object->GetName().c_str());
            }
        }
        ImGui::SetWindowFontScale(1.0f);
        return this;
    }

    void VisualNode::HandleSelection(NodeEditor* editor)
    {
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
        {
            HighLight();
            switch (m_Type)
            {
            case NodeType::State:
                if (const auto state = editor->GetCurrentFsm()->GetState(m_Id); state && !state->GetDescription().empty())
                {
                    if (ImGui::BeginTooltip())
                    {
                        ImGui::Text(state->GetDescription().c_str());
                        ImGui::EndTooltip();
                    }
                }
                break;
            case NodeType::Transition:
                if (const auto trigger = editor->GetCurrentFsm()->GetTrigger(m_Id); trigger && !trigger->GetDescription().empty())
                {
                    if (ImGui::BeginTooltip())
                    {
                        ImGui::Text(trigger->GetDescription().c_str());
                        ImGui::EndTooltip();
                    }
                }
                break;
            }
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
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
                editor->SetCreatingLink(false);
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && IsSelected())
            {
                editor->SetDragging(true);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_C)
                && ImGui::IsKeyDown(ImGuiKey_LeftCtrl)
                && IsSelected())
            {
                editor->CopyNode(this);
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
