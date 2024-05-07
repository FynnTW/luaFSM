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

        struct ImGuiColorTheme
        {
            ImVec4 standard{};
            ImVec4 hovered{};
            ImVec4 active{};
            ImVec4 text{};
            ImVec4 background{};
            ImVec4 windowBg{};
            std::string name;
            nlohmann::json Serialize()
            {
                nlohmann::json json;
                json["standard"] = {standard.x, standard.y, standard.z, standard.w};
                json["hovered"] = {hovered.x, hovered.y, hovered.z, hovered.w};
                json["active"] = {active.x, active.y, active.z, active.w};
                json["text"] = {text.x, text.y, text.z, text.w};
                json["background"] = {background.x, background.y, background.z, background.w};
                json["windowBg"] = {windowBg.x, windowBg.y, windowBg.z, windowBg.w};
                json["name"] = name;
                return json;
            }
            static ImGuiColorTheme Deserialize(const nlohmann::json& json)
            {
                ImGuiColorTheme theme;
                theme.name = json["name"];
                theme.standard = ImVec4(json["standard"][0], json["standard"][1], json["standard"][2], json["standard"][3]);
                theme.hovered = ImVec4(json["hovered"][0], json["hovered"][1], json["hovered"][2], json["hovered"][3]);
                theme.active = ImVec4(json["active"][0], json["active"][1], json["active"][2], json["active"][3]);
                theme.text = ImVec4(json["text"][0], json["text"][1], json["text"][2], json["text"][3]);
                theme.background = ImVec4(json["background"][0], json["background"][1], json["background"][2], json["background"][3]);
                theme.windowBg = ImVec4(json["windowBg"][0], json["windowBg"][1], json["windowBg"][2], json["windowBg"][3]);
                return theme;
            }
            
        };
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
        static void InitThemes();
        static void AddTheme(const std::string& name, const ImGuiColorTheme& theme);
        static ImGuiColorTheme GetTheme(const std::string& name);
        static void SetTheme(const std::string& name);
        static void SetTheme(const ImGuiColorTheme& theme);
        static void RemoveTheme(const std::string& name);
        

    private:
        void Init(const WindowProps& props);
        GLFWwindow* m_Window;
        std::shared_ptr<TextEditor> m_TextEditor;
        static TextEditor::Palette m_Palette;
        static std::unordered_map<std::string, ImGuiColorTheme> m_Themes;
        static std::string m_ActiveTheme;
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
