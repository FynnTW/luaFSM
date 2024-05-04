#pragma once
#include "imgui.h"
#include "Graphics/VisualNode.h"

namespace LuaFsm
{
    class VisualNode;
    class NodeEditor
    {
    public:
        int AddNode(const std::string& name);
        void AddNode(VisualNode* node);
        [[nodiscard]] ImVec2 GetNodeSize() const { return m_NodeSize; }
        void SetNodeSize(const ImVec2& size) { m_NodeSize = size; }
        [[nodiscard]] ImVec2 GetLastNodePos() const { return m_LastNodePos; }
        void SetLastNodePos(const ImVec2& pos) { m_LastNodePos = pos; }
        [[nodiscard]] int GetLinkCount() const { return m_LinkCount; }
        void IncrementLinkCount() { m_LinkCount++; }
        void DecrementLinkCount() { m_LinkCount--; }
        int AddLink() { return m_LinkCount++; }
        void ClearLinkCount() { m_LinkCount = 0; }
        [[nodiscard]] std::string GetLastNodeName() const { return m_LastNodeName; }
        void SetLastNodeName(const std::string& name) { m_LastNodeName = name; }
        [[nodiscard]] float GetNodeRadius() const { return m_NodeRadius; }
        inline static uint32_t NodeWindowFlags = ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoTitleBar;
        ImVec2 GetNextNodePos();
        void DrawConnection(VisualNode* fromNode, VisualNode* toNode);
        void DeselectAllNodes();
        void ClearNodes() { m_Nodes.clear(); }
        [[nodiscard]] std::unordered_map<std::string, VisualNode*> GetNodes() const { return m_Nodes; }
        VisualNode* GetNode(const std::string& id) { return m_Nodes[id]; }
        void RemoveNode(const std::string& id) { m_Nodes.erase(id); }
        [[nodiscard]] VisualNode* GetSelectedNode() const { return m_SelectedNode; }
        void SetSelectedNode(VisualNode* node);
        [[nodiscard]] ImVec2 GetCanvasPos() const { return m_CanvasPos; }
        void SetCanvasPos(const ImVec2& pos) { m_CanvasPos = pos; }
        [[nodiscard]] ImVec2 GetCanvasSize() const { return m_CanvasSize; }
        void SetCanvasSize(const ImVec2& size) { m_CanvasSize = size; }
        

    private:
        ImVec2 m_NodeSize{100, 100};
        float m_NodeRadius = 60.0f;
        ImVec2 m_LastNodePos{0, 0};
        int m_LinkCount = 0;
        ImVec2 m_CanvasPos{0, 0};
        ImVec2 m_CanvasSize{0, 0};
        VisualNode* m_SelectedNode = nullptr;
        std::string m_LastNodeName;
        std::unordered_map<std::string, VisualNode*> m_Nodes;
        
    };
    
}
