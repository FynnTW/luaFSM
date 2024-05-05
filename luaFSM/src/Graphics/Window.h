#pragma once
#include "data/FSM.h"
#include "GLFW/glfw3.h"

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
        ~Window();
        [[nodiscard]] unsigned int GetWidth() const {return m_Data.width;}
        [[nodiscard]] unsigned int GetHeight() const {return m_Data.height;}
        void SetVSync(bool enabled);
        void OnUpdate() const;
        [[nodiscard]] bool IsVSync() const {return m_Data.vSync;}
        void Shutdown() const;
        void InitImGui();
        static TextEditor::Palette GetPalette() {return m_Palette;}
        static void BeginImGui();
        static void OnImGuiRender();
        static void DrawTextEditor(TextEditor& txtEditor, std::string& oldText);
        static void TrimTrailingNewlines(std::string& str);
        void EndImGui() const;
        GLFWwindow* GetNativeWindow() const {return m_Window;}
        static [[nodiscard]] ImFont* GetFont(const std::string& name)
        {
            return m_Fonts[name];
        }
        static void AddFont (const std::string& name, ImFont* font)
        {
            m_Fonts[name] = font;
        }
        

    private:
        void Init(const WindowProps& props);
        GLFWwindow* m_Window;
        std::shared_ptr<TextEditor> m_TextEditor;
        static TextEditor::Palette m_Palette;
        struct WindowData
        {
            std::string title;
            Window* window;
            unsigned int width, height;
            bool vSync;
        } m_Data;
        static std::unordered_map<std::string, ImFont*> m_Fonts;
    
    };
    
}
