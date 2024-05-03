#include "pch.h"
#include "Window.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "Log.h"
#include "imgui/ImGuiImpl.h"
#include "luaFsm.h"
#include "data/FSM.h"
#include "imgui/TextEditor.h"
#include "imgui/imnodes/imnodes.h"

bool shutdown = false;
void onWindowClose(GLFWwindow* window)
{
    shutdown = true;
}

namespace LuaFsm
{
    static bool S_GLFW_INITIALIZED = false;

    static ImFont* FONT = nullptr;

    std::shared_ptr<Fsm> startExampleFsm()
    {
        auto fsm = std::make_shared<Fsm>("dunland_fsm");
        fsm->SetName("Dunland FSM");
        
        auto dunland_init_state = std::make_shared<FsmState>("dunland_init_state");
        dunland_init_state->SetName("Initial State");
        dunland_init_state->SetDescription("The initial state for the Dunland FSM");
        dunland_init_state->AddEvent("onCharacterSelected");
        dunland_init_state->SetOnUpdate(R"(
            log("Updating state 1")
            self.data.characterName = eventData.character.shortName
        )");
        dunland_init_state->SetOnEnter(R"(
            log(self.name .. " Entered")
        )");
        dunland_init_state->SetOnExit(R"(
            log(self.name .. " Exited")
        )");

        auto gadridoc_selected = std::make_shared<FsmTrigger>("gadridoc_selected");
        gadridoc_selected->SetName("Gadridoc Selected");
        gadridoc_selected->SetDescription("Check if the character selected is Gadridoc");
        gadridoc_selected->SetCondition(R"(
            return this.arguments.characterName == this.arguments.checkedName
        )");
        gadridoc_selected->SetOnTrue(R"(print("Gadridoc selected"))");
        gadridoc_selected->SetOnFalse(R"(print("Gadridoc not selected"))");
        gadridoc_selected->AddArgument("characterName", "");
        gadridoc_selected->AddArgument("checkedName", "Gadridoc");
        gadridoc_selected->SetNextStateId("gadridoc_selected_state");
        
        dunland_init_state->AddTrigger(gadridoc_selected);
        fsm->AddState(dunland_init_state);
        auto luaCode = gadridoc_selected->GetLuaCode();
        return fsm;
    }

    Window::Window(const std::string& title, const unsigned int width, const unsigned int height)
    {
        const WindowProps props(title, width, height);
        Init(props);
        m_Data.window = std::shared_ptr<Window>(this);
    }

    Window::~Window()
    {
    }


    void Window::Init(const WindowProps& props)
    {
        m_Data.title = props.title;
        m_Data.width = props.width;
        m_Data.height = props.height;
        
        if (!S_GLFW_INITIALIZED)
        {
            const int success = glfwInit();
            S_GLFW_INITIALIZED = true;
        }
        
        m_Window = glfwCreateWindow(static_cast<int>(props.width), static_cast<int>(props.height), props.title.c_str(), nullptr, nullptr);
        
        glfwSetWindowUserPointer(m_Window, &m_Data);
        SetVSync(true);
        glfwMakeContextCurrent(m_Window);
        glfwSetWindowCloseCallback(m_Window, onWindowClose);
        glfwMaximizeWindow(m_Window);
    }

    void Window::Shutdown()
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImNodes::DestroyContext();
        ImGui::DestroyContext();
        glfwDestroyWindow(m_Window);
        const auto app = Application::Get();
        app->Close();
    }

    TextEditor editor;
    void Window::InitImGui()
    {
        ImGui::CreateContext();
        ImNodes::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
        io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
        

        
        ImGui_ImplOpenGL3_Init("#version 410");
        ImGui_ImplGlfw_InitForOpenGL(GetNativeWindow(), true);
        m_Fsm = startExampleFsm();
        m_NodeEditor = std::make_shared<NodeEditor>();
        
        FONT = ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCode.ttf", 20);
        editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        auto state = m_Fsm->GetState("dunland_init_state");
        auto trigger = state->GetTrigger("gadridoc_selected");
        auto luaCode = trigger->GetLuaCode();
        editor.SetText(luaCode);
    }

    

    void Window::BeginImGui()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::PushFont(FONT);
    }

    bool opened = true;
    bool opened2 = true;
        
    std::vector<std::pair<int, int>> links;
    

    void Window::OnImGuiRender()
{
    const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                shutdown = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // Set window position and size
    ImGui::SetNextWindowPos(ImVec2(mainViewport->WorkPos.x, mainViewport->WorkPos.y + 30), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(mainViewport->WorkSize.x, mainViewport->WorkSize.y - 30), ImGuiCond_Once);

    // Main window encompassing all elements
    ImGui::Begin("Node Editor", &opened, ImGuiWindowFlags_MenuBar
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoResize);

    // Second-level menu bar
    if (ImGui::BeginMenuBar())
    {
        ImGui::Button("Add Node");
        ImGui::EndMenuBar();
    }

    // Dock space setup
    ImGuiID dockspace_id = ImGui::GetID("NodeEditorDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_None);

    ImGui::End();

    // Initialize Docking Layout (only run once)
    if (!ImGui::DockBuilderGetNode(dockspace_id))
    {
        ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_None); // Add a new node

        ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.5f, nullptr, &dockspace_id);
        ImGuiID dock_id_right = dockspace_id;

        ImGui::DockBuilderDockWindow("Canvas", dock_id_left);
        ImGui::DockBuilderDockWindow("Code", dock_id_right);

        ImGui::DockBuilderFinish(dockspace_id);
    }

    // Canvas window
    ImGui::Begin("Canvas");
    {
        ImNodes::BeginNodeEditor();
        ImNodes::EndNodeEditor();
        ImGui::End();
    }

    // Code editor window
    ImGui::Begin("Code", &opened2);
    {
        editor.Render("Lua Editor");
        auto state = m_Fsm->GetState("dunland_init_state");
        auto trigger = state->GetTrigger("gadridoc_selected");
        auto luaCode = trigger->GetLuaCode();
        if (ImGui::Button("Copy"))
            ImGui::SetClipboardText(luaCode.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Close"))
            opened2 = false;
        ImGui::End();
    }
}

    
    void Window::EndImGui()
    {
        // Rendering
        ImGui::PopFont();
        constexpr auto clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        const ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui::Render();
        GLFWwindow* window = glfwGetCurrentContext();
        int displayW, displayH;
        glfwGetFramebufferSize(window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backupCurrentContext);
        }
        glfwSwapBuffers(window);
        if (shutdown)
            Shutdown();
    }

    void Window::SetVSync(const bool enabled)
    {
        m_Data.vSync = enabled;
        if (m_Data.vSync)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);
    }
    
    void Window::OnUpdate()
    {
        glfwPollEvents();
        BeginImGui();
        OnImGuiRender();
        EndImGui();
    }

    
}
