#pragma once
#include <json.hpp>
#include <spdlog/fmt/bundled/format.h>

#include "imgui.h"
#include "ImGuiNotify.hpp"
#include "data/Fsm.h"
#include "Graphics/VisualNode.h"

namespace LuaFsm
{

    struct FsmRegex
    {
        
        static std::regex ClassStringRegex(const std::string &id, const std::string &fieldName)
        {
            const auto string =  fmt::format(R"({0}\.{1}\s*=\s*\"((?:.|\s)*?)\"$)", id, fieldName);
            std::regex regex(string);
            return regex;
        }
        
        static std::regex InvalidIdRegex()
        {
            std::string string = "(";
            string += R"(?:^[0-9]|\s|,|\.|\/|\\|\(|\)|\[|\]|\{|\}|\*|\+|\?|\^|\$|\||\!|\@|\#|\%|\&|\=|\:|\;|\"|\'|\<|\>|\`|\~|\-)";
            for (const std::array<std::string, 23> luaKeywords {
                     "and", "break", "do", "else", "elseif", "end",
                     "false", "for", "function", "goto", "if", "in",
                     "local", "nil", "not", "or", "repeat", "require",
                     "return", "then", "true", "until", "while"};
                     const auto& keyword : luaKeywords)
            {
                string += "|\\b" + keyword + "\\b";
            }
            string += ")";
            std::regex regex(string);
            return regex;
        }

        static std::regex ClassIntegerRegex(const std::string &id, const std::string &fieldName)
        {
            const auto string =  fmt::format(R"({0}\.{1}\s*=\s*([0-9]+))", id, fieldName);
            std::regex regex(string);
            return regex;
        }

        static std::regex ClassFloatRegex(const std::string &id, const std::string &fieldName)
        {
            const auto string =  fmt::format(R"({0}\.{1}\s*=\s*(\-*[\.0-9]+))", id, fieldName);
            std::regex regex(string);
            return regex;
        }

        static std::regex ClassBoolRegex(const std::string &id, const std::string &fieldName)
        {
            const auto string =  fmt::format(R"({0}\.{1}\s*=\s*((?:true|false)))", id, fieldName);
            std::regex regex(string);
            return regex;
        }

        static std::regex ClassTableRegex(const std::string &id, const std::string &fieldName)
        {
            const auto prefix =  fmt::format(R"({0}\.{1})", id, fieldName);
            const auto string = prefix + R"(\s*=\s*\{(?:\s*)((?:.|\s)+?)(?:\s*)\})";
            std::regex regex(string);
            return regex;
        }

        static std::regex ClassTableElementsRegex()
        {
            const auto string = R"((\-*[\.0-9]+)(?:,|$))";
            std::regex regex(string);
            return regex;
        }

        static std::regex IdRegex(const std::string &classType)
        {
            const auto string = fmt::format("---@{0}\\s+([a-zA-Z0-9_-]+)", classType);
            std::regex regex(string);
            return regex;
        }

        static std::regex IdRegexClass(const std::string &classType, const std::string &className)
        {
            const auto string = fmt::format("---@{0}\\s+({1})", classType, className);
            std::regex regex(string);
            return regex;
        }

        static std::regex IdRegexClassFull(const std::string &classType, const std::string &className)
        {
            const auto string = fmt::format("(---@{0}\\s+{1})", classType, className);
            std::regex regex(string);
            return regex;
        }

        static std::regex IdRegexClassAnnotation(const std::string &classType, const std::string &className)
        {
            const auto string = fmt::format(R"((class\s+\b{0}\b\s*\:\s*\b{1}\b))", className, classType);
            std::regex regex(string);
            return regex;
        }

        static std::regex IdRegexClassDeclaration(const std::string &classType, const std::string &className)
        {
            const auto string = fmt::format(R"((\b{0}\b\s*\=\s*\b{1}\b\:new))", className, classType);
            std::regex regex(string);
            return regex;
        }
        
        static std::string FunctionNameString(const std::string &id, const std::string &funcName)
        {
            return fmt::format("{0}:{1}\\(.*\\)", id, funcName);
        }
        
        static std::regex FunctionBody(const std::string &id, const std::string &funcName)
        {
            const auto functionBodyString = R"(\s*([\s\S]*?)\s*end---@endFunc)";
            std::regex regex(FunctionNameString(id, funcName) + functionBodyString);
            return regex;
        }
        
        static std::regex FunctionBodyReplace(const std::string &id, const std::string &funcName)
        {
            const auto functionBodyString = R"(\s*([\s\S]*?)\s*(end---@endFunc))";
            std::regex regex("(" + FunctionNameString(id, funcName) + ")" + functionBodyString);
            return regex;
        }
    };

    enum class IdValidityError
    {
        Valid,
        EmptyId,
        DuplicateIdState,
        DuplicateIdTrigger,
        InvalidId
    };
    
    class VisualNode; //forward declaration
    class Fsm; //forward declaration
    
    class NodeEditor
    {
    public:
        NodeEditor();
        ~NodeEditor();

        //Canvas position
        [[nodiscard]] ImVec2 GetCanvasPos()
        {
            if (const auto canvas = GetCanvasWindow())
                m_CanvasPos = canvas->Pos;
            return m_CanvasPos;
        }
        void SetCanvasPos(const ImVec2& pos) { m_CanvasPos = pos; }

        //Canvas size
        [[nodiscard]] ImVec2 GetCanvasSize()
        {
            if (const auto canvas = GetCanvasWindow())
                m_CanvasSize = canvas->Size;
            return m_CanvasSize;
        }
        void SetCanvasSize(const ImVec2& size) { m_CanvasSize = size; }

        [[nodiscard]] ImVec2 GetCanvasScroll() const
        {
            if (const auto canvas = GetCanvasWindow())
                return canvas->Scroll;
            return {0, 0};
        }

        void SetCanvasScroll(const ImVec2& scroll) const
        {
            if (const auto canvas = GetCanvasWindow())
                canvas->Scroll = scroll;
        }

        //Font
        [[nodiscard]] ImFont* GetFont() const { return m_Font; }
        void SetFont(ImFont* font) { m_Font = font; }

        //Selected nodes
        [[nodiscard]] VisualNode* GetSelectedNode() const { return m_SelectedNode; }
        void SetSelectedNode(VisualNode* node);
        void DeselectAllNodes();

        //Get node by id
        VisualNode* GetNode(const std::string& id, NodeType type = NodeType::State) const;

        //Draw lines
        static void DrawLine(const ImVec2 fromPos, const ImVec2 toPos, ImU32 color = IM_COL32_WHITE, const float thickness = 2.0f, float
                             arrowHeadWidth = 10.0f,
                             float arrowHeadLength = 15.0f, const float curve = 0.0f, VisualNode* fromNode = nullptr, VisualNode* targetNode = nullptr);
        static void DrawConnection(VisualNode* fromNode, VisualNode* toNode);
        void ExportLua(const std::string& filePath) const;

        void SetCurrentFsm(const FsmPtr& fsm) { m_Fsm = fsm; DeselectAllNodes(); }
        [[nodiscard]] FsmPtr GetCurrentFsm() const { return m_Fsm; }

        static NodeEditor* Get() { return m_Instance; }

        [[nodiscard]] bool IsCreatingLink() const { return m_IsCreatingLink; }
        void SetCreatingLink(const bool creatingLink) { m_IsCreatingLink = creatingLink; }

        [[nodiscard]] bool ShowFsmProps() const { return m_ShowFsmProps; }
        void SetShowFsmProps(const bool showFsmProps) { m_ShowFsmProps = showFsmProps; }

        void CopyNode(VisualNode* node) { m_CopiedNode = node; }
        [[nodiscard]] VisualNode* GetCopiedNode() const { return m_CopiedNode; }

        nlohmann::json SerializeSettings() const;
        void SaveSettings() const;
        void LoadSettings();
        void DeserializeSettings(const nlohmann::json& settings);
        
        float GetScale() const { return m_Scale; }
        void SetScale(const float scale) { m_Scale = scale; }

        bool IsDragging() const { return m_IsDragging; }
        void SetDragging(const bool dragging) { m_IsDragging = dragging; }

        bool IsSettingInCurve() const { return m_IsSettingInCurve; }
        void SetSettingInCurve(const bool settingInCurve) { m_IsSettingInCurve = settingInCurve; }

        bool IsSettingOutCurve() const { return m_IsSettingOutCurve; }
        void SetSettingOutCurve(const bool settingOutCurve) { m_IsSettingOutCurve = settingOutCurve; }

        IdValidityError CheckIdValidity(const std::string& id) const;
        static void InformValidityError(IdValidityError error);
        void MoveToNode(const std::string& id, const NodeType type);

        void SetCurveNode(VisualNode* node) { m_CurveNode = node; }
        [[nodiscard]] VisualNode* GetCurveNode() const { return m_CurveNode; }

        bool AppendStates() const { return m_AppendStates; }
        void SetAppendStates(const bool appendStates) { m_AppendStates = appendStates; }

        bool ShowPriority() const { return m_ShowPriority; }
        void SetShowPriority(const bool show) { m_ShowPriority = show; }

        ImVec2 GetDragOffset() const { return m_DragOffset; }
        void SetDragOffset(const ImVec2& offset) { m_DragOffset = offset; }
        
        bool ShowNodeContext() const { return m_ShowNodeContext; }
        void SetShowNodeContext(const bool show) { m_ShowNodeContext = show; }

        bool IsPanning() const { return m_IsPanning; }
        void SetPanning(const bool panning) { m_IsPanning = panning; }

        [[nodiscard]] ImGuiWindow* GetCanvasWindow() const { return ImGui::FindWindowByName(canvasName.c_str()); }
        
    public:
        inline static uint32_t nodeWindowFlags = ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoFocusOnAppearing
            | ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoTitleBar;
        std::string canvasName = "Canvas";
    
    private:
        ImVec2 m_CanvasPos{0, 0};
        ImVec2 m_CanvasSize{0, 0};
        ImVec2 m_DragOffset{0, 0};
        VisualNode* m_SelectedNode = nullptr;
        bool m_ShowFsmProps = false;
        bool m_IsDragging = false;
        bool m_IsSettingInCurve = false;
        bool m_IsSettingOutCurve = false;
        bool m_ShowNodeContext = false;
        bool m_IsPanning = false;
        VisualNode* m_CurveNode = nullptr;
        VisualNode* m_CopiedNode = nullptr;
        ImFont* m_Font = nullptr;
        bool m_IsCreatingLink = false;
        FsmPtr m_Fsm = nullptr;
        static NodeEditor* m_Instance;
        float m_Scale = 1.0f;
        bool m_AppendStates = true;
        bool m_ShowPriority = false;
    };
    
}
