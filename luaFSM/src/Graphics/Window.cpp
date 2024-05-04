#include "pch.h"
#include "Window.h"

#include <complex.h>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui/imgui_stdlib.h"
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

    FsmPtr Window::m_Fsm = nullptr;

    std::shared_ptr<Fsm> startExampleFsm()
    {
        Window::SetCurrentFsm(std::make_shared<Fsm>("dunland_state"));
        auto fsm = Window::GetCurrentFsm();
        fsm->SetName("Dunland FSM");

        const auto dunlandInitState = fsm->AddState("dunlandInitState");
        dunlandInitState->SetName("Initial State");
        dunlandInitState->SetDescription("The initial state for the Dunland FSM");
        dunlandInitState->AddEvent("onCharacterSelected");
        dunlandInitState->SetOnUpdate( "self.data.characterName = eventData.character.shortName");
        dunlandInitState->SetOnEnter("log(self.name .. \" Entered\")");
        dunlandInitState->SetOnExit("log(self.name .. \" Exited\")");

        const auto gadridocSelected = dunlandInitState->AddTrigger("gadridocSelected");
        gadridocSelected->SetCurrentState(dunlandInitState->GetId());
        gadridocSelected->SetName("Gadridoc Selected");
        gadridocSelected->SetDescription("Check if the character selected is Gadridoc");
        gadridocSelected->SetCondition("return self.arguments.characterName == self.arguments.checkedName");
        gadridocSelected->SetOnTrue(R"(print("Gadridoc selected"))");
        gadridocSelected->SetOnFalse(R"(print("Gadridoc not selected"))");
        gadridocSelected->AddArgument("characterName", "");
        gadridocSelected->AddArgument("checkedName", "Gadridoc");
        gadridocSelected->SetNextState("gadridoc_selected_state");
        
        fsm->AddState(dunlandInitState);
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
        editor.SetText(m_Fsm->GetLuaCode());
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
    
    // FSM node structure
    struct FSMNode {
        const char* name;
        ImVec2 position;
        float radius;
        bool is_dragging = false;
        ImVec2 drag_offset;
    };

    std::vector<FSMNode> nodes = {
        {"State 1", ImVec2(0, 0), 300,false, ImVec2(0, 0)},
        {"State 2", ImVec2(0, 100), 300,false, ImVec2(0, 0)},
        {"State 3", ImVec2(0, 200), 300,false, ImVec2(0, 0)},
    };

    ImVec2 GetCirclePos(const ImVec2& center, float radius, float angle)
    {
        return ImVec2(center.x + cos(angle) * radius, center.y + sin(angle) * radius);
    }

    ImVec2 closestPointOnCircle(ImVec2 center, float radius, ImVec2 point)
    {
        // Calculate the direction vector from the center to the point
        float dx = point.x - center.x;
        float dy = point.y - center.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        // Handle the edge case where the point is at the center of the circle
        if (distance == 0.0f)
        {
            // In this case, we'll just return a point directly to the right of the center
            return ImVec2(center.x + radius, center.y);
        }

        // Normalize the direction vector
        float ux = dx / distance;
        float uy = dy / distance;

        // Calculate the closest point on the circle's boundary
        ImVec2 closestPoint;
        closestPoint.x = center.x + ux * radius;
        closestPoint.y = center.y + uy * radius;

        return closestPoint;
    }
    
    bool addStatePopup = false;
    void TrimTrailingNewlines(std::string& str)
    {
        while (!str.empty() && (str.back() == '\n' || str.back() == '\r'))
        {
            str.pop_back();
        }
    }
    
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
            | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoResize);

        // Second-level menu bar
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::Button("Add State"))
            {
                addStatePopup = true;
            }
            ImGui::EndMenuBar();
        }

        if (addStatePopup)
        {
            ImGui::OpenPopup("Add State");
            addStatePopup = false;
        }

        if (ImGui::BeginPopup("Add State"))
        {
            ImGui::Text("Add State");
            static std::string stateId;
            ImGui::InputText("State ID", &stateId, 0);
            static std::string stateName;
            ImGui::InputText("State Name", &stateName, 0);
            if (!m_Fsm->GetState(stateId) && !stateId.empty() && !stateName.empty())
            {
                if (ImGui::Button("Add"))
                {
                    auto state = std::make_shared<FsmState>(stateId);
                    state->SetName(stateName);
                    m_Fsm->AddState(state);
                    ImGui::CloseCurrentPopup();
                }
            }
            else if (m_Fsm->GetState(stateId))
            {
                ImGui::Text("State ID already exists");
            }
            else if (stateId.empty())
            {
                ImGui::Text("State ID cannot be empty");
            }
            else if (stateName.empty())
            {
                ImGui::Text("State Name cannot be empty");
            }
            else
            {
                if (ImGui::Button("Add"))
                {
                    const auto newState =std::make_shared<FsmState>(stateId);
                    newState->SetName(stateName);
                    m_Fsm->AddState(newState);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        // Dock space setup
        ImGuiID dockspace_id = ImGui::GetID("NodeEditorDockspace");
        ImGui::DockSpace(dockspace_id, ImVec2(0, 0), ImGuiDockNodeFlags_None);

        // Initialize Docking Layout (only run once)
        if (!ImGui::DockBuilderGetNode(dockspace_id))
        {
            ImGui::DockBuilderRemoveNode(dockspace_id); // Clear out existing layout
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_None); // Add a new node
            ImGui::DockBuilderSetNodeSize(dockspace_id, mainViewport->Size);

            ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.5f, nullptr, &dockspace_id);
            ImGuiID dock_id_right = dockspace_id;

            ImGui::DockBuilderDockWindow("Canvas", dock_id_left);
            ImGui::DockBuilderDockWindow("Code", dock_id_right);

            ImGui::DockBuilderFinish(dockspace_id);
        }
        ImGui::End();// Define a struct to represent a node

        uint32_t nodeWindowFlags = ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoTitleBar;

        // Canvas window
        ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus);
        {
            m_NodeEditor->SetCanvasPos(ImGui::GetWindowPos());
            m_NodeEditor->SetCanvasSize(ImGui::GetWindowSize());
            for (const auto& [key, state] : m_Fsm->GetStates())
            {
                m_NodeEditor->AddNode(state->DrawNode(m_NodeEditor.get()));
            }
            for (const auto& [key, trigger] : m_Fsm->GetTriggers())
            {
                m_NodeEditor->AddNode(trigger->DrawNode(m_NodeEditor.get()));
                if (trigger->GetCurrentState())
                    if (const auto node = m_NodeEditor->GetNode(trigger->GetCurrentState()->GetId()))
                        m_NodeEditor->DrawConnection(node, trigger->GetNode());
                if (trigger->GetNextState())
                    if (const auto node = m_NodeEditor->GetNode(trigger->GetNextState()->GetId()))
                        m_NodeEditor->DrawConnection(node, trigger->GetNode());
            }
            ImGui::End();
        }

        // Code editor window
        ImGui::Begin("Code", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus);
        {
            const auto node = m_NodeEditor->GetSelectedNode();
            if (node)
            {
                switch (node->GetType())
                {
                    case NodeType::State:
                    {
                        const FsmStatePtr state = m_Fsm->GetState(node->GetId());
                        if (state)
                            DrawStateProperties(m_Fsm->GetState(node->GetId()).get());
                        break;
                    }
                    case NodeType::Transition:
                    {
                        auto trigger = m_Fsm->GetTrigger(node->GetId());
                        if (trigger)
                            DrawTriggerProperties(m_Fsm->GetTrigger(node->GetId()));
                        break;
                    }
                }
            }
            ImGui::End();
        }
    }

    void Window::DrawStateProperties(FsmState* state)
    {
        if (ImGui::Button("Refresh"))
        {
            if (state)
                editor.SetText(state->GetLuaCode());
        }
        if (ImGui::BeginTabBar("Node Properties"))
        {
            if (ImGui::BeginTabItem("Properties"))
            {
                if (state)
                {
                    ImGui::Text("Name");
                    std::string name = state->GetName();
                    ImGui::InputText("Name", &name);
                    state->SetName(name);
                    ImGui::Text("Description");
                    std::string description = state->GetDescription();
                    ImGui::InputText("Description", &description);
                    state->SetDescription(description);
                    ImGui::Text("Events");
                    if (ImGui::BeginListBox("Events"))
                    {
                        for (const auto& event : state->GetEvents())
                        {
                            ImGui::Text(event.c_str());
                        }
                        ImGui::EndListBox();
                    }
                    ImGui::Text("Data");
                    if (ImGui::BeginListBox("Data"))
                    {
                        for (const auto& [key, value] : state->GetData())
                        {
                            ImGui::Text(key.c_str());
                        }
                        ImGui::EndListBox();
                    }
                    ImGui::Text("Triggers");
                    if (ImGui::BeginListBox("Triggers"))
                    {
                        for (const auto& [key, value] : state->GetTriggers())
                        {
                            ImGui::Text(key.c_str());
                        }
                        ImGui::EndListBox();
                    }
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("OnUpdate"))
            {
                if (state)
                {
                    auto oldText = state->GetOnUpdate();
                    TrimTrailingNewlines(oldText);
                    if (state->GetOnUpdateEditor()->GetText() != state->GetOnUpdate())
                        state->GetOnUpdateEditor()->SetText(state->GetOnUpdate());
                    state->GetOnUpdateEditor()->Render("OnUpdate");
                    if (state->GetOnUpdateEditor()->IsTextChanged())
                        state->SetOnUpdate(state->GetOnUpdateEditor()->GetText());
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("OnEnter"))
            {
                if (state)
                {
                    auto oldText = state->GetOnEnter();
                    TrimTrailingNewlines(oldText);
                    if (state->GetOnEnterEditor()->GetText() != state->GetOnEnter())
                        state->GetOnEnterEditor()->SetText(state->GetOnEnter());
                    state->GetOnEnterEditor()->Render("OnEnter");
                    if (state->GetOnEnterEditor()->IsTextChanged())
                        state->SetOnEnter(state->GetOnEnterEditor()->GetText());
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("OnExit"))
            {
                if (state)
                {
                    auto oldText = state->GetOnExit();
                    TrimTrailingNewlines(oldText);
                    if (state->GetOnExitEditor()->GetText() != state->GetOnExit())
                        state->GetOnExitEditor()->SetText(state->GetOnExit());
                    state->GetOnExitEditor()->Render("OnExit");
                    if (state->GetOnExitEditor()->IsTextChanged())
                        state->SetOnExit(state->GetOnExitEditor()->GetText());
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("OnInit"))
            {
                if (state)
                {
                    auto oldText = state->GetOnInit();
                    TrimTrailingNewlines(oldText);
                    if (state->GetOnInitEditor()->GetText() != state->GetOnInit())
                        state->GetOnInitEditor()->SetText(state->GetOnInit());
                    state->GetOnInitEditor()->Render("OnInit");
                    if (state->GetOnInitEditor()->IsTextChanged())
                        state->SetOnInit(state->GetOnInitEditor()->GetText());
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Lua Code"))
            {
                if (state)
                    editor.Render("Lua Code");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    
    void Window::DrawTriggerProperties(FsmTrigger* trigger)
    {
        if (ImGui::Button("Refresh"))
        {
            if (trigger)
                editor.SetText(trigger->GetLuaCode());
        }
        if (ImGui::BeginTabBar("Node Properties"))
        {
            if (ImGui::BeginTabItem("Properties"))
            {
                if (trigger)
                {
                    ImGui::Text("Name");
                    std::string name = trigger->GetName();
                    ImGui::InputText("Name", &name);
                    trigger->SetName(name);
                    ImGui::Text("Description");
                    std::string description = trigger->GetDescription();
                    ImGui::InputText("Description", &description);
                    trigger->SetDescription(description);
                    ImGui::Text("arguments");
                    if (ImGui::BeginTable("arguments", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg))
                    {
                        ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 200.0f);
                        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                        int index = 0;
                        for (auto& [key, value] : trigger->GetArguments())
                        {
                            ImGui::TableNextRow();
                            std::string keyId = "##Key" + std::to_string(index);
                            std::string valueId = "##Value" + std::to_string(index);
                            std::string keyX = key;
                            std::string valueX = value;
            
                            // Key column
                            ImGui::TableSetColumnIndex(0);
                            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                            ImGui::InputText(keyId.c_str(), &keyX);

                            // Value column
                            ImGui::TableSetColumnIndex(1);
                            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                            ImGui::InputText(valueId.c_str(), &valueX);
                            index++;
                        }
                        ImGui::EndTable();
                    }
                    ImGui::NewLine();
                    ImGui::Text("Condition:");
                    ImGui::Separator();
                    TextEditor* conditionEditor = trigger->GetConditionEditor();
                    auto pallete = conditionEditor->GetPalette();
                    pallete.at(static_cast<int>(TextEditor::PaletteIndex::Background)) =
                        ImGui::ColorConvertFloat4ToU32({0.12f, 0.12f, 0.12f, 1.f});
                    pallete.at(static_cast<int>(TextEditor::PaletteIndex::Default)) =
                        ImGui::ColorConvertFloat4ToU32({0.61f, 0.83f, 0.99f, 1.f});
                    pallete.at(static_cast<int>(TextEditor::PaletteIndex::Punctuation)) =
                        ImGui::ColorConvertFloat4ToU32({1.f, 1.f, 1.f, 1.f});
                    pallete.at(static_cast<int>(TextEditor::PaletteIndex::Identifier)) =
                        ImGui::ColorConvertFloat4ToU32({0.66f, 0.89f, 1.f, 1.f});
                    conditionEditor->SetPalette(pallete);
                    auto oldText = trigger->GetCondition();
                    TrimTrailingNewlines(oldText);
                    if (conditionEditor->GetText() != trigger->GetCondition())
                        conditionEditor->SetText(trigger->GetCondition());
                    conditionEditor->Render("Condition");
                    if (conditionEditor->IsTextChanged())
                        trigger->SetCondition(conditionEditor->GetText());
                    ImGui::Separator();
                    
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("OnTrue"))
            {
                if (trigger)
                {
                    auto oldText = trigger->GetOnTrue();
                    TrimTrailingNewlines(oldText);
                    if (trigger->GetOnTrueEditor()->GetText() != trigger->GetOnTrue())
                        trigger->GetOnTrueEditor()->SetText(trigger->GetOnTrue());
                    trigger->GetOnTrueEditor()->Render("OnTrue");
                    if (trigger->GetOnTrueEditor()->IsTextChanged())
                        trigger->SetOnTrue(trigger->GetOnTrueEditor()->GetText());
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("OnFalse"))
            {
                if (trigger)
                {
                    auto oldText = trigger->GetOnFalse();
                    TrimTrailingNewlines(oldText);
                    if (trigger->GetOnFalseEditor()->GetText() != trigger->GetOnFalse())
                        trigger->GetOnFalseEditor()->SetText(trigger->GetOnFalse());
                    trigger->GetOnFalseEditor()->Render("OnFalse");
                    if (trigger->GetOnFalseEditor()->IsTextChanged())
                        trigger->SetOnFalse(trigger->GetOnFalseEditor()->GetText());
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Lua Code"))
            {
                if (trigger)
                    editor.Render("Lua Code");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
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
