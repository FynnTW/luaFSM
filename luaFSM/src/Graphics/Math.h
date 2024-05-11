#pragma once
#include "imgui.h"

namespace LuaFsm
{
/*
    inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs)
    {
        return {lhs.x - rhs.x, lhs.y - rhs.y};
    }
    inline ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs)
    {
        return {lhs.x * rhs.x, lhs.y * rhs.y};
    }
    
    inline ImVec2 operator-(const ImVec2& lhs, const float& value)
    {
        return {lhs.x - value, lhs.y - value};
    }
    inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)
    {
        return {lhs.x + rhs.x, lhs.y + rhs.y};
    }
    inline ImVec2 operator/(const ImVec2& lhs, const float& value)
    {
        return {lhs.x / value, lhs.y / value};
    }
    inline ImVec2 operator*(const ImVec2& lhs, const float& value)
    {
        return {lhs.x * value, lhs.y * value};
    }
    inline ImVec2 operator+(const ImVec2& lhs, const float& value)
    {
        return {lhs.x + value, lhs.y + value};
    }*/
    class Math
    {
    public:
        static ImVec2 GetCirclePos(const ImVec2& center, float radius, float angle);
        static ImVec2 ClosestPointOnCircle(ImVec2 center, float radius, ImVec2 point);
        static ImVec2 ClosestPointOnDiamond(ImVec2 center, float width, float height, ImVec2 point);
        static ImVec2 ClosestPointOnEllipse(ImVec2 center, float a, float b, ImVec2 point);
        
        static float Distance(const ImVec2& a, const ImVec2& b)
        {
            return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
        }
        static float DirectionVectorX(const ImVec2& a, const ImVec2& b)
        {
            return SubtractVec2X(b, a.x).x;
        }
        static float DirectionVectorY(const ImVec2& a, const ImVec2& b)
        {
            return SubtractVec2Y(b, a.y).y;
        }
        static float GetCircleWindowSideLength(const float radius)
        {
            return radius * 2;
        }
        static ImVec2 AddVec2(const ImVec2& a, const ImVec2& b)
        {
            return {a.x + b.x, a.y + b.y};
        }
        static ImVec2 SubtractVec2(const ImVec2& a, const ImVec2& b)
        {
            return {a.x - b.x, a.y - b.y};
        }
        static ImVec2 MultiplyVec2(const ImVec2& a, const ImVec2& b)
        {
            return {a.x * b.x, a.y * b.y};
        }
        static ImVec2 MultiplyVec2(const ImVec2& a, const float value)
        {
            return {a.x * value, a.y * value};
        }
        static ImVec2 AddVec2X(const ImVec2& a, const float x)
        {
            return {a.x + x, a.y};
        }
        static ImVec2 AddVec2Y(const ImVec2& a, const float y)
        {
            return {a.x, a.y + y};
        }
        static ImVec2 SubtractVec2X(const ImVec2& a, const float x)
        {
            return {a.x - x, a.y};
        }
        static ImVec2 SubtractVec2Y(const ImVec2& a, const float y)
        {
            return {a.x, a.y - y};
        }
        static bool ImVec2Equal(const ImVec2& a, const ImVec2& b)
        {
            return a.x == b.x && a.y == b.y;
        }
    private:
    
    };
    
}
