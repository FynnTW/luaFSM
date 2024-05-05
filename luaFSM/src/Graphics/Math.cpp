#include "pch.h"
#include "Math.h"
#include <numbers>

namespace LuaFsm
{
    ImVec2 Math::GetCirclePos(const ImVec2& center, const float radius, const float angle)
    {
        return {center.x + cos(angle) * radius, center.y + sin(angle) * radius};
    }

    ImVec2 Math::ClosestPointOnCircle(const ImVec2 center, const float radius, const ImVec2 point)
    {
        const float distance = Distance(center, point);
        
        if (distance == 0.0f)
            return {center.x + radius, center.y};

        const float ux = DirectionVectorX(center, point) / distance;
        const float uy = DirectionVectorY(center, point) / distance;

        return {center.x + ux * radius, center.y + uy * radius};
    }
    
    float absolute(const float value) {
        return value < 0 ? -value : value;
    }

    // Calculate the distance from a point to a line segment (ax, ay) to (bx, by)
    float pointToLineDistance(float px, float py, float ax, float ay, float bx, float by) {
        float lengthSq = (bx - ax) * (bx - ax) + (by - ay) * (by - ay);
        float t = ((px - ax) * (bx - ax) + (py - ay) * (by - ay)) / lengthSq;
        t = std::max(0.0f, std::min(1.0f, t));
        float closestX = ax + t * (bx - ax);
        float closestY = ay + t * (by - ay);
        return std::hypot(px - closestX, py - closestY);
    }
    
    ImVec2 Math::ClosestPointOnDiamond(const ImVec2 center, const float width, const float height, const ImVec2 point)
    {
        float halfWidth = width / 2;
        float halfHeight = height / 2;

        // Define the four corners of the diamond
        ImVec2 corners[4] = {
            {center.x, center.y + halfHeight},   // Top
            {center.x + halfWidth, center.y},    // Right
            {center.x, center.y - halfHeight},   // Bottom
            {center.x - halfWidth, center.y}     // Left
        };

        ImVec2 closestPoint = corners[0];
        float minDistance = pointToLineDistance(point.x, point.y, corners[0].x, corners[0].y, corners[1].x, corners[1].y);

        // Check each edge of the diamond
        for (int i = 1; i < 4; ++i) {
            float distance = pointToLineDistance(point.x, point.y, corners[i].x, corners[i].y, corners[(i + 1) % 4].x, corners[(i + 1) % 4].y);
            if (distance < minDistance) {
                minDistance = distance;
                closestPoint = ImVec2((corners[i].x + corners[(i + 1) % 4].x) / 2,
                                      (corners[i].y + corners[(i + 1) % 4].y) / 2);
            }
        }

        return closestPoint;
    }

    // Utility function to calculate distance between two points
    float distance(float x1, float y1, float x2, float y2) {
        return std::hypot(x1 - x2, y1 - y2);
    }

    // Function to find the closest point on an ellipse to a given point
    ImVec2 Math::ClosestPointOnEllipse(const ImVec2 center, const float a, const float b, const ImVec2 point)
    {
        constexpr int numSamples = 1000;
        float minDistance = std::numeric_limits<float>::max();
        ImVec2 closestPoint;

        for (int i = 0; i <= numSamples; ++i) {
            const float theta = (2 * std::numbers::pi * i) / numSamples;
            const float ex = center.x + a * std::cos(theta);
            const float ey = center.y + b * std::sin(theta);
            if (const float dist = distance(point.x, point.y, ex, ey); dist < minDistance) {
                minDistance = dist;
                closestPoint = ImVec2(ex, ey);
            }
        }

        return closestPoint;
    }
}
