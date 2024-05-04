#pragma once
#include "data/FSM.h"
#include "GLFW/glfw3.h"
#include "imgui/NodeEditor.h"

namespace LuaFsm
{
    class Window;

    struct WindowData
    {
        std::string title;
        std::shared_ptr<Window> window;
        unsigned int width, height;
        bool vSync;
    };
    
    struct WindowProps
    {
        std::string title;
        unsigned int width;
        unsigned int height;
        explicit WindowProps(std::string title = "",
                             const unsigned int width = 1920,
                             const unsigned int height = 1080)
            : title(std::move(title)), width(width), height(height)
        {
        }
    };
    
    class Window
    {
    public:
        Window(const std::string& title, unsigned int width, unsigned int height);
        Window() = default;
        virtual ~Window();
        [[nodiscard]] unsigned int GetWidth() const {return m_Data.width;}
        [[nodiscard]] unsigned int GetHeight() const {return m_Data.height;}
        void SetVSync(bool enabled);
        void OnUpdate();
        [[nodiscard]] bool IsVSync() const {return m_Data.vSync;}
        void Shutdown();
        void InitImGui();
        void BeginImGui();
        void OnImGuiRender();
        static void DrawStateProperties(FsmState* state);
        static void DrawTriggerProperties(FsmTrigger* trigger);
        void EndImGui();
        static FsmPtr GetCurrentFsm() {return m_Fsm;}
        static void SetCurrentFsm(const FsmPtr& fsm) { m_Fsm = fsm; }
        GLFWwindow* GetNativeWindow() const {return m_Window;}
        

    private:
        void Init(const WindowProps& props);
        GLFWwindow* m_Window;
        struct WindowData
        {
            std::string title;
            std::shared_ptr<Window> window;
            unsigned int width, height;
            bool vSync;
        } m_Data;
        static std::shared_ptr<Fsm> m_Fsm;
        std::shared_ptr<NodeEditor> m_NodeEditor;
    
    };
    
}
