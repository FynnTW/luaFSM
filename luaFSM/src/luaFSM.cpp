#include "pch.h"

#include "luaFsm.h"
#include "imgui/NodeEditor.h"

int main(int argc, char* argv[])
{
    const auto app = new LuaFsm::Application();
    const auto editor = new LuaFsm::NodeEditor();
    app->GetWindow()->InitImGui();
    app->Run();
    delete editor;
    delete app;
    return 0;
}

namespace LuaFsm
{
    Application* Application::m_SInstance = nullptr;
    
    Application::Application()
    {
        m_SInstance = this;
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
        m_Window = std::make_shared<Window>("LuaFSM", 1280, 720);
    }

    Application::~Application()
    = default;

    void Application::Run() const
    {
        while (m_Running)
        {
            glClearColor(0.1f, 0.1f, 0.1f, 1);
            glClear(GL_COLOR_BUFFER_BIT);
            m_Window->OnUpdate();
        }
    }
}
