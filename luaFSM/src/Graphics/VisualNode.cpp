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
    
    VisualNode* VisualNode::Draw2(const DrawableObject* object)
    {
        const auto editor = NodeEditor::Get();
        if (m_WindowLabel == 0)
            m_WindowLabel = ++WINDOW_COUNT;
        const std::string label = "##Window" + std::to_string(m_WindowLabel);
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
                    drawList->AddEllipse(m_LastPosition, m_EllipseRadius, IM_COL32(0, 255, 0, 255));
                else
                    drawList->AddEllipse(m_LastPosition, m_EllipseRadius, GetBorderColor());
                drawList->AddText(GetTextPos(object->GetName().c_str()), IM_COL32_WHITE, object->GetName().c_str());
            }
            else if (m_Shape == NodeShape::Circle)
            {
                drawList->AddCircleFilled(m_LastPosition, m_Radius, GetCurrentColor());
                drawList->AddCircle(m_LastPosition, m_Radius, GetBorderColor());
            }
            ImGui::EndChild();
            ImGui::SetWindowFontScale(editor->GetScale());
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
            ImGui::SetWindowFontScale(1.0f);
        }
        return this;
    }

    VisualNode* VisualNode::Draw(const DrawableObject* object)
    {
        const auto editor = NodeEditor::Get();
        const float scale = editor->GetScale();
        ImGui::SetWindowFontScale(editor->GetScale());
        ImGui::SetNextWindowSize(InitSizes(object->GetName()), ImGuiCond_Always);
        if (m_TargetPosition.x < 0)
            m_TargetPosition = editor->GetNextNodePos(this);
        const ImVec2 adjustedPos = GetPosition();
        if (adjustedPos.x < 0)
        {
            SetInvisible(true);
            return nullptr;
        }
        //adjustedPos = Math::AddVec2(editor->GetCanvasPos(), adjustedPos);
        SetInvisible(false);
        if (!Math::ImVec2Equal(m_LastPosition, adjustedPos))
            ImGui::SetNextWindowPos(adjustedPos);
        m_LastPosition = adjustedPos;
        if (m_WindowLabel == 0)
            m_WindowLabel = ++WINDOW_COUNT;
        const std::string label = "##Window" + std::to_string(m_WindowLabel);
        const ImGuiWindow* existingWindow = ImGui::FindWindowByName(label.c_str());
        if (const ImGuiWindow* canvas = ImGui::FindWindowByName("Canvas"); existingWindow && canvas)
        {
            bool needsClampToScreen = false;
            ImVec2 targetPos = existingWindow->Pos;
            if (existingWindow->Pos.x < canvas->Pos.x + ImGui::GetWindowContentRegionMin().x)
            {
                needsClampToScreen = true;
                targetPos.x = canvas->Pos.x + ImGui::GetWindowContentRegionMin().x;
            }
            else if (existingWindow->Size.x + existingWindow->Pos.x > canvas->Size.x + canvas->Pos.x)
            {
                needsClampToScreen = true;
                targetPos.x = canvas->Pos.x + canvas->Size.x - existingWindow->Size.x;
            }
            if (existingWindow->Pos.y < canvas->Pos.y + ImGui::GetWindowContentRegionMin().y)
            {
                needsClampToScreen = true;
                targetPos.y = canvas->Pos.y + ImGui::GetWindowContentRegionMin().y;
            }
            else if (existingWindow->Size.y + existingWindow->Pos.y > canvas->Size.y + canvas->Pos.y)
            {
                needsClampToScreen = true;
                targetPos.y = canvas->Pos.y + canvas->Size.y - existingWindow->Size.y;
            }
            if (needsClampToScreen)
                ImGui::SetNextWindowPos(targetPos, ImGuiCond_Always);
        }
        ImGui::Begin(label.c_str(), nullptr, NodeEditor::nodeWindowFlags);
        {
            ImGui::SetWindowFontScale(editor->GetScale());
            auto drawList = ImGui::GetWindowDrawList();
            HandleSelection(editor);
            ImVec2 right = {};
            if (m_Shape == NodeShape::Ellipse)
            {
                const auto ellipseRadius = Math::MultiplyVec2(m_EllipseRadius, scale);
                drawList->AddEllipseFilled(GetDrawPos(),  m_EllipseRadius, GetCurrentColor());
                if (NodeEditor::Get()->GetCurrentFsm()->GetInitialStateId() == m_Id)
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
                const auto radius = m_Radius * scale;
                drawList->AddCircleFilled(GetDrawPos(), m_Radius, GetCurrentColor());
                drawList->AddCircle(GetDrawPos(), m_Radius, GetBorderColor());
            }
            const auto windowPos = ImGui::GetWindowPos();
            auto movedAmount =
                Math::SubtractVec2(windowPos, Math::AddVec2(adjustedPos, {ImGui::GetScrollX() / 5, ImGui::GetScrollY() / 5}));
            const auto invertedScale = ImVec2(1.0f / NodeEditor::Get()->GetScale(), 1.0f / NodeEditor::Get()->GetScale());
            movedAmount = Math::MultiplyVec2(movedAmount, invertedScale);
            auto newPos = Math::AddVec2(GetTargetPosition(), movedAmount);
            newPos.x = std::max(editor->GetCanvasPos().x + 1, newPos.x);
            newPos.y = std::max(editor->GetCanvasPos().y + 1, newPos.y);
            SetTargetPosition(newPos);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::End();
            ImGui::SetWindowFontScale(editor->GetScale());
            if (m_Type == NodeType::Transition)
            {
                drawList = ImGui::GetWindowDrawList();
                auto rightBound = Math::AddVec2X(GetLastDrawPos(), m_Radius);
                auto leftBound = Math::SubtractVec2X(GetLastDrawPos(), m_Radius);
                auto trigger = editor->GetCurrentFsm()->GetTrigger(m_Id);
                ImVec2 currentStateArrowPos = {};
                ImVec2 nextStateArrowPos = {};

                if (trigger->GetCurrentState())
                    currentStateArrowPos = GetFromPoint(trigger->GetCurrentState()->GetNode());

                if (trigger->GetNextState())
                    nextStateArrowPos = GetFromPoint(trigger->GetNextState()->GetNode());

                auto textSize = ImGui::CalcTextSize(object->GetName().c_str());
                ImVec2 position = {};
    
                // Determine arrow directions and adjust text position
                bool arrowFromLeft = (currentStateArrowPos.x < GetLastDrawPos().x
                    && abs(currentStateArrowPos.y - GetLastDrawPos().y) < m_Radius * 0.5)
                || (nextStateArrowPos.x < GetLastDrawPos().x
                    && abs(nextStateArrowPos.y - GetLastDrawPos().y) < m_Radius * 0.5);
                bool arrowFromRight = (currentStateArrowPos.x > GetLastDrawPos().x
                    && abs(currentStateArrowPos.y - GetLastDrawPos().y) < m_Radius * 0.5)
                || (nextStateArrowPos.x > GetLastDrawPos().x
                    && abs(nextStateArrowPos.y - GetLastDrawPos().y) < m_Radius * 0.5);

                if (arrowFromRight)
                {
                    // Move text to the left
                    auto xPos = Math::SubtractVec2X(leftBound, 3.0f + textSize.x);
                    position = Math::SubtractVec2Y(xPos, textSize.y * 0.5f);
                }
                else if (arrowFromLeft)
                {
                    // Move text to the right
                    auto xPos = Math::AddVec2X(rightBound, 3.0f);
                    position = Math::SubtractVec2Y(xPos, textSize.y * 0.5f);
                }
                else
                {
                    // Default position
                    auto xPos = Math::AddVec2X(rightBound, 3.0f);
                    position = Math::SubtractVec2Y(xPos, textSize.y * 0.5f);
                }

                drawList->AddText(position, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]), object->GetName().c_str());
            }
            ImGui::SetWindowFontScale(1.0f);
        }
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
                editor->SetDragging(true);
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
