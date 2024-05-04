#pragma once
#include "imgui.h"
#include "MathHelpers.h"
#include "imgui/NodeEditor.h"

namespace LuaFsm
{
    class NodeEditor;
    enum class NodeShape
    {
        Circle,
        Square,
        Diamond,
        Triangle
    };
    
    enum class NodeType
    {
        State,
        Transition
    };

    
    class VisualNode
    {
    public:
        [[nodiscard]] ImVec2 GetPosition() const { return m_Position; }
        void SetPosition(const ImVec2& position) { m_Position = position; }
        [[nodiscard]] ImVec2 GetSize() const { return m_Size; }
        void SetSize(const ImVec2& size) { m_Size = size; }
        [[nodiscard]] float GetRadius() const { return m_Radius; }
        [[nodiscard]] NodeType GetType() const { return m_Type; }
        void SetType(const NodeType type) { m_Type = type; }
        void SetRadius(const float radius) { m_Radius = radius; }
        [[nodiscard]] ImVec2 GetCenter() const { return {m_Size.x / 2, m_Size.y / 2}; }
        void SetCenter(const ImVec2& center) { m_Center = center; }
        [[nodiscard]] NodeShape GetShape() const { return m_Shape; }
        void SetShape(const NodeShape shape) { m_Shape = shape; }
        [[nodiscard]] ImColor GetColor() const { return m_Color; }
        void SetColor(const ImColor color) { m_Color = color; }
        [[nodiscard]] ImColor GetBorderColor() const { return m_BorderColor; }
        void SetBorderColor(const ImColor borderColor) { m_BorderColor = borderColor; }
        [[nodiscard]] ImVec2 GetFromPoint(const ImVec2 toPoint)
        {
            if (m_Shape == NodeShape::Circle)
                return MathHelpers::ClosestPointOnCircle(GetDrawPos(), m_Radius, toPoint);
            else
                return MathHelpers::ClosestPointOnDiamond(GetDrawPos(), m_Size.x, m_Size.y, toPoint);
            
        }
        [[nodiscard]] ImVec2 GetFromPoint(const VisualNode* toNode)
        {
            if (m_Shape == NodeShape::Circle)
                return MathHelpers::ClosestPointOnCircle(GetLastDrawPos(), m_Radius, toNode->GetLastDrawPos());
            else
                return MathHelpers::ClosestPointOnDiamond(GetLastDrawPos(), m_Size.x, m_Size.y, toNode->GetLastDrawPos());
        }
        [[nodiscard]] std::string GetId() const { return m_Id; }
        void SetId(const std::string& id) { m_Id = id; }
        [[nodiscard]] bool IsSelected() const { return m_Selected; }
        void Select() { m_Selected = true; HighLightSelected(); }
        void HighLight() { m_CurrentColor = m_HighLightColor;}
        void HighLightSelected() { m_CurrentColor = m_HighLightColorSelected;}
        void UnHighLight() { m_CurrentColor = m_Color; }
        void Deselect() { m_Selected = false; UnHighLight(); }
        [[nodiscard]] ImColor GetCurrentColor() const { return m_CurrentColor; }
        void SetCurrentColor(const ImColor currentColor) { m_CurrentColor = currentColor; }
        void HandleSelection(NodeEditor* editor);
        ImVec2 InitSizes()
        {
            auto sideLength = MathHelpers::GetCircleWindowSideLength(m_Radius);
            m_Size = {sideLength, sideLength};
            m_Center = {sideLength / 2, sideLength / 2};
            m_Shape = NodeShape::Circle;
            return m_Size;
        }
        ImVec2 InitSizesDiamond(const std::string& name)
        {
            float width = ImGui::CalcTextSize(name.c_str()).x + 50.0f;
            float height = ImGui::CalcTextSize(name.c_str()).y + 80.0f;
            m_Size = {width, height};
            m_Center = {width / 2, height / 2};
            m_Shape = NodeShape::Diamond;
            return m_Size;
        }
        ImVec2 GetTextPos (const char* text);
        [[nodiscard]] ImVec2 GetDrawPos()
        {
            m_Position = ImGui::GetWindowPos();
            return MathHelpers::AddVec2(m_Position, GetCenter());
        }
        [[nodiscard]] ImVec2 GetLastDrawPos() const { return MathHelpers::AddVec2(m_Position, GetCenter()); }

    private:
        ImVec2 m_Position{-1.0f, -1.0f};
        ImVec2 m_Size{};
        float m_Radius = 50.0f;
        ImVec2 m_Center;
        std::string m_Id;
        bool m_Selected = false;
        NodeShape m_Shape = NodeShape::Circle;
        NodeType m_Type = NodeType::State;
        ImColor m_Color = IM_COL32(0, 100, 100, 150);
        ImColor m_HighLightColor = IM_COL32(25, 125, 125, 175);
        ImColor m_HighLightColorSelected = IM_COL32(50, 150, 150, 200);
        ImColor m_CurrentColor = IM_COL32(0, 100, 100, 150);
        ImColor m_BorderColor = IM_COL32(255, 255, 255, 255);
    };
    
}
