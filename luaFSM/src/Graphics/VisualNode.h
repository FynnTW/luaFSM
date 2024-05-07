#pragma once
#include <json.hpp>

#include "imgui.h"
#include "Math.h"
#include "data/DrawableObject.h"

namespace LuaFsm
{
    class NodeEditor;
    enum class NodeShape
    {
        Circle,
        Square,
        Diamond,
        Ellipse,
        Triangle
    };
    
    enum class NodeType
    {
        State,
        Transition
    };

    struct StateData
    {
        std::string name;
        std::string value = "nil";
        std::string type = "any";
        std::string comment;
        nlohmann::json Serialize()
        {
            nlohmann::json json;
            json["name"] = name;
            json["value"] = value;
            json["type"] = type;
            json["comment"] = comment;
            return json;
        }
        static StateData Deserialize(const nlohmann::json& json)
        {
            StateData data;
            data.name = json["name"];
            data.value = json["value"];
            data.type = json["type"];
            data.comment = json["comment"];
            return data;
        }
    };
    
    class VisualNode
    {
    public:
        [[nodiscard]] ImVec2 GetPosition();
        VisualNode* Draw(const DrawableObject* object);
        [[nodiscard]] ImVec2 GetTargetPosition() const { return m_TargetPosition; }
        void SetTargetPosition(const ImVec2& targetPosition) { m_TargetPosition = targetPosition; }
        [[nodiscard]] ImVec2 GetLastPosition() const { return m_LastPosition; }
        void SetLastPosition(const ImVec2& lastPosition) { m_LastPosition = lastPosition; }
        [[nodiscard]] ImVec2 GetSize() const { return m_Size; }
        void SetSize(const ImVec2& size) { m_Size = size; }
        [[nodiscard]] float GetRadius() const { return m_Radius; }
        [[nodiscard]] NodeType GetType() const { return m_Type; }
        void SetType(const NodeType type) { m_Type = type; }
        [[nodiscard]] bool IsInvisible() const { return m_IsVisible; }
        void SetInvisible(const bool invisible) { m_IsVisible = invisible; }
        void SetRadius(const float radius) { m_Radius = radius; }
        [[nodiscard]] ImVec2 GetCenter() const { return {m_Size.x / 2, m_Size.y / 2}; }
        void SetCenter(const ImVec2& center) { m_Center = center; }
        [[nodiscard]] NodeShape GetShape() const { return m_Shape; }
        void SetShape(const NodeShape shape) { m_Shape = shape; }
        [[nodiscard]] ImColor GetColor() const { return m_Color; }
        void SetColor(const ImColor color) { m_Color = color; }
        [[nodiscard]] ImColor GetBorderColor() const { return m_BorderColor; }
        void SetBorderColor(const ImColor borderColor) { m_BorderColor = borderColor; }
        [[nodiscard]] ImVec2 GetFromPoint(const ImVec2 toPoint) const
        {
            if (m_Shape == NodeShape::Ellipse)
                return Math::ClosestPointOnEllipse(GetLastDrawPos(), m_EllipseRadius.x, m_EllipseRadius.y, toPoint);
            if (m_Shape == NodeShape::Circle)
                return Math::ClosestPointOnCircle(GetLastDrawPos(), m_Radius, toPoint);
            return Math::ClosestPointOnDiamond(GetLastDrawPos(), m_Size.x, m_Size.y, toPoint);
            
        }
        [[nodiscard]] ImVec2 GetFromPoint(const VisualNode* toNode) const
        {
            if (m_Shape == NodeShape::Ellipse)
                return Math::ClosestPointOnEllipse(GetLastDrawPos(), m_EllipseRadius.x, m_EllipseRadius.y,  toNode->GetLastDrawPos());
            if (m_Shape == NodeShape::Circle)
                return Math::ClosestPointOnCircle(GetLastDrawPos(), m_Radius, toNode->GetLastDrawPos());
            return Math::ClosestPointOnDiamond(GetLastDrawPos(), m_Size.x, m_Size.y, toNode->GetLastDrawPos());
        }
        [[nodiscard]] std::string GetId() const { return m_Id; }
        void SetId(const std::string& id) { m_Id = id; }
        [[nodiscard]] bool IsSelected() const { return m_Selected; }
        void Select() { m_Selected = true; HighLightSelected(); }
        void HighLight() { m_CurrentColor = GetHighlightColor();}
        void HighLightSelected() { m_CurrentColor = GetHighlightColorSelected();}
        void UnHighLight() { m_CurrentColor = m_Color; }
        void Deselect() { m_Selected = false; UnHighLight(); }
        [[nodiscard]] ImColor GetCurrentColor() const { return m_CurrentColor; }
        void SetCurrentColor(const ImColor currentColor) { m_CurrentColor = currentColor; }
        void SetHighlightColor(const ImColor color) { m_HighLightColor = color; }
        void SetHighlightColorSelected(const ImColor color) { m_HighLightColorSelected = color; }
        [[nodiscard]] ImColor GetHighlightColor() const
        {
            return {
                std::min(1.0f, m_Color.Value.x * 1.5f),
                std::min(1.0f, m_Color.Value.y * 1.5f),
                std::min(1.0f, m_Color.Value.z * 1.5f),
                std::min(1.0f, m_Color.Value.w * 1.5f)
            };
        }
        [[nodiscard]] ImColor GetHighlightColorSelected() const
        {
            return {
               std::min(1.0f, m_Color.Value.x * 2.0f),
               std::min(1.0f, m_Color.Value.y * 2.0f),
               std::min(1.0f, m_Color.Value.z * 2.0f),
               std::min(1.0f, m_Color.Value.w * 2.0f)
            };
        }
        void HandleSelection(NodeEditor* editor);
        ImVec2 InitSizes()
        {
            auto sideLength = Math::GetCircleWindowSideLength(m_Radius);
            m_Size = {sideLength, sideLength};
            m_Center = {sideLength / 2, sideLength / 2};
            m_Shape = NodeShape::Circle;
            return m_Size;
        }
        ImVec2 InitSizes2()
        {
            m_Radius = 12.f;
            auto sideLength = Math::GetCircleWindowSideLength(m_Radius);
            m_Size = {sideLength, sideLength};
            m_Center = {sideLength / 2, sideLength / 2};
            m_Shape = NodeShape::Circle;
            return m_Size;
        }
        ImVec2 InitSizesEllipse(const std::string& name)
        {
            const float width = ImGui::CalcTextSize(name.c_str()).x + 50.0f;
            const float height = ImGui::CalcTextSize(name.c_str()).y + 30.0f;
            m_Size = {width + 10, height + 3};
            m_EllipseRadius = {width / 2, height / 2};
            m_Center = {width / 2, height / 2};
            m_Shape = NodeShape::Ellipse;
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
        ImVec2 InitSizesDiamond2()
        {
            m_Size = {65, 60};
            m_Center = {m_Size.x / 2.f, m_Size.y / 2.f};
            m_Shape = NodeShape::Diamond;
            return Math::AddVec2(m_Size, {2,2});
        }
        ImVec2 GetTextPos (const char* text);
        [[nodiscard]] ImVec2 GetDrawPos()
        {
            m_Position = ImGui::GetWindowPos();
            return Math::AddVec2(m_Position, GetCenter());
        }
        [[nodiscard]] ImVec2 GetLastDrawPos() const { return Math::AddVec2(m_Position, GetCenter()); }

    private:
        ImVec2 m_Position{-1.0f, -1.0f};
        ImVec2 m_TargetPosition{-1.0f, -1.0f};
        ImVec2 m_LastPosition{-1.0f, -1.0f};
        ImVec2 m_Size{};
        float m_Radius = 50.0f;
        ImVec2 m_EllipseRadius = {m_Radius, m_Radius};
        ImVec2 m_Center;
        std::string m_Id;
        bool m_Selected = false;
        bool m_IsVisible = false;
        int m_WindowLabel = 0;
        NodeShape m_Shape = NodeShape::Circle;
        NodeType m_Type = NodeType::State;
        ImColor m_Color = IM_COL32(0, 80, 80, 150);
        ImColor m_HighLightColor = IM_COL32(25, 125, 125, 175);
        ImColor m_HighLightColorSelected = IM_COL32(50, 150, 150, 200);
        ImColor m_CurrentColor = IM_COL32(0, 100, 100, 150);
        ImColor m_BorderColor = IM_COL32(255, 255, 255, 255);
    };
    
}
