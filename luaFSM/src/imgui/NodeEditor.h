#pragma once
#include "imgui.h"
#include "data/Fsm.h"
#include "Graphics/VisualNode.h"

namespace LuaFsm
{
    class VisualNode; //forward declaration
    class Fsm; //forward declaration
    class NodeEditor
    {
    public:
        NodeEditor();
        ~NodeEditor();

        //Canvas position
        [[nodiscard]] ImVec2 GetCanvasPos() const { return m_CanvasPos; }
        void SetCanvasPos(const ImVec2& pos) { m_CanvasPos = pos; }

        //Canvas size
        [[nodiscard]] ImVec2 GetCanvasSize() const { return m_CanvasSize; }
        void SetCanvasSize(const ImVec2& size) { m_CanvasSize = size; }

        //Font
        [[nodiscard]] ImFont* GetFont() const { return m_Font; }
        void SetFont(ImFont* font) { m_Font = font; }

        //Last node position
        [[nodiscard]] ImVec2 GetLastNodePos() const { return m_LastNodePos; }
        void SetLastNodePos(const ImVec2& pos) { m_LastNodePos = pos; }
        ImVec2 GetNextNodePos(VisualNode* node);

        //Selected nodes
        [[nodiscard]] VisualNode* GetSelectedNode() const { return m_SelectedNode; }
        void SetSelectedNode(VisualNode* node);
        void DeselectAllNodes();

        //Get node by id
        VisualNode* GetNode(const std::string& id, NodeType type = NodeType::State) const;

        //Draw lines
        static void DrawLine(ImVec2 fromPos, ImVec2 toPos, ImU32 color = IM_COL32_WHITE, float thickness = 2.0f, float arrowHeadWidth= 10.0f,
                             float arrowHeadLength= 15.0f);
        static void DrawConnection(const VisualNode* fromNode, const VisualNode* toNode);
        void ExportLua(const std::string& filePath) const;

        void SetCurrentFsm(const FsmPtr& fsm) { m_Fsm = fsm; }
        [[nodiscard]] FsmPtr GetCurrentFsm() const { return m_Fsm; }

        static NodeEditor* Get() { return m_Instance; }

        [[nodiscard]] bool IsCreatingLink() const { return m_IsCreatingLink; }
        void SetCreatingLink(const bool creatingLink) { m_IsCreatingLink = creatingLink; }

        [[nodiscard]] bool ShowFsmProps() const { return m_ShowFsmProps; }
        void SetShowFsmProps(const bool showFsmProps) { m_ShowFsmProps = showFsmProps; }

        void SaveFsm(const std::string& filePath) const;
        
    public:
        inline static uint32_t nodeWindowFlags = ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoFocusOnAppearing
            | ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoTitleBar;
        

    private:
        ImVec2 m_LastNodePos{0, 0};
        ImVec2 m_CanvasPos{0, 0};
        ImVec2 m_CanvasSize{0, 0};
        VisualNode* m_SelectedNode = nullptr;
        VisualNode* m_LastNode = nullptr;
        bool m_ShowFsmProps = false;
        ImFont* m_Font = nullptr;
        bool m_IsCreatingLink = false;
        FsmPtr m_Fsm = nullptr;
        static NodeEditor* m_Instance;
        
    };
    
}
