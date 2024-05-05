#pragma once
#include "Graphics/Window.h"

namespace LuaFsm
{
    class Application
    {
    public:
        Application();
        virtual ~Application();

        void Run() const;
        static Application* Get() { return m_SInstance; }
        Window* GetWindow() const { return m_Window.get(); }
        void Close() { m_Running = false; }

    private:
        static Application* m_SInstance;
        bool m_Running = true;
        std::shared_ptr<Window> m_Window;
    };
}
