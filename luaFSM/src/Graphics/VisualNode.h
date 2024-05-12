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
        Transition,
        Fsm
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
        void Deselect()
        {
            m_Selected = false;
            if (!m_IsHighlighted)
                UnHighLight();
        }
        [[nodiscard]] ImColor GetCurrentColor() const { return m_CurrentColor; }
        void SetCurrentColor(const ImColor currentColor) { m_CurrentColor = currentColor; }
        void SetHighlightColor(const ImColor color) { m_HighLightColor = color; }
        void SetHighlightColorSelected(const ImColor color) { m_HighLightColorSelected = color; }
        [[nodiscard]] ImColor GetHighlightColor() const
        {
            return {
                std::min(1.0f, m_Color.Value.x * 1.75f),
                std::min(1.0f, m_Color.Value.y * 1.75f),
                std::min(1.0f, m_Color.Value.z * 1.75f),
                std::min(1.0f, m_Color.Value.w * 1.75f)
            };
        }
        [[nodiscard]] ImColor GetHighlightColorSelected() const
        {
            return {
               std::min(1.0f, m_Color.Value.x * 1.5f),
               std::min(1.0f, m_Color.Value.y * 1.5f),
               std::min(1.0f, m_Color.Value.z * 1.5f),
               std::min(1.0f, m_Color.Value.w * 1.5f)
            };
        }
        void HandleSelection(NodeEditor* editor);
        ImVec2 InitSizes(const std::string& name);
        ImVec2 GetTextPos (const char* text);
        [[nodiscard]] ImVec2 GetDrawPos()
        {
            m_Position = ImGui::GetWindowPos();
            return Math::AddVec2(m_Position, GetCenter());
        }
        [[nodiscard]] ImVec2 GetLastDrawPos2() const { return Math::AddVec2(m_Position, GetCenter()); }
        [[nodiscard]] ImVec2 GetLastDrawPos() const { return m_LastPosition; }

        ImVec2 GetGridPos() const { return m_GridPos; }
        void SetGridPos(const ImVec2& gridPos) { m_GridPos = gridPos; }
        ImVec2 GetEllipseRadius() const { return m_EllipseRadius; }
        void SetEllipseRadius(const ImVec2& ellipseRadius) { m_EllipseRadius = ellipseRadius; }
        void SetInArrowCurve(const float inArrowCurve) { m_InArrowCurve = inArrowCurve; }
        void SetOutArrowCurve(const float outArrowCurve) { m_OutArrowCurve = outArrowCurve; }
        float GetInArrowCurve() const { return m_InArrowCurve; }
        float GetOutArrowCurve() const { return m_OutArrowCurve; }
        void SetLastConnectionPoint(const ImVec2& lastConnectionPoint) { m_LastConnectionPoint = lastConnectionPoint; }
        ImVec2 GetLastConnectionPoint() const { return m_LastConnectionPoint; }
        void SetInLineMidPoint(const ImVec2& lineMidPoint) { m_InLineMidPoint = lineMidPoint; }
        ImVec2 GetInLineMidPoint() const { return m_InLineMidPoint; }
        void SetOutLineMidPoint(const ImVec2& lineMidPoint) { m_OutLineMidPoint = lineMidPoint; }
        ImVec2 GetOutLineMidPoint() const { return m_OutLineMidPoint; }
        void SetIsHighlighted(const bool isHighlighted)
        {
            m_IsHighlighted = isHighlighted;
            if (isHighlighted)
                HighLight();
            else if (!m_Selected)
                UnHighLight();
        }
        [[nodiscard]] bool IsHighlighted() const { return m_IsHighlighted; }

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
        bool m_IsHighlighted = false;
        float m_InArrowCurve = 0.0f;
        float m_OutArrowCurve = 0.0f;
        bool m_TextLeft = false;
        int m_WindowLabel = 0;
        ImVec2 m_LastConnectionPoint = {0, 0};
        NodeShape m_Shape = NodeShape::Circle;
        NodeType m_Type = NodeType::State;
        ImVec2 m_GridPos = {1048,1032};
        ImVec2 m_InLineMidPoint = {};
        ImVec2 m_OutLineMidPoint = {};
        ImColor m_Color = IM_COL32(0, 80, 80, 150);
        ImColor m_HighLightColor = IM_COL32(25, 125, 125, 175);
        ImColor m_HighLightColorSelected = IM_COL32(50, 150, 150, 200);
        ImColor m_CurrentColor = IM_COL32(0, 100, 100, 150);
        ImColor m_BorderColor = IM_COL32(255, 255, 255, 255);
    };
    
}
