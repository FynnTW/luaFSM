#include "pch.h"
#include "Window.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui/imgui_stdlib.h"
#include "Log.h"
#include "imgui/ImGuiImpl.h"
#include "luaFsm.h"
#include "data/FSM.h"
#include "imgui/TextEditor.h"
#include "imgui/NodeEditor.h"
#include "imgui/ImFileDialog.h"
#include "IO/FileReader.h"
#include "IO/LuaParser.h"

bool SHUTDOWN = false;
bool SHOW_METRICS = false;
void onWindowClose(GLFWwindow* window)
{
    SHUTDOWN = true;
}

namespace LuaFsm
{
    namespace {
        bool S_GLFW_INITIALIZED = false;
        ImFont* FONT = nullptr;
    }

    TextEditor::Palette Window::m_Palette = {};
    std::unordered_map<std::string, ImFont*> Window::m_Fonts = {};

    void startExampleFsm()
    {
        NodeEditor::Get()->SetCurrentFsm(std::make_shared<Fsm>("dunland_state"));
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
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
    }

    Window::Window(const std::string& title, const unsigned int width, const unsigned int height)
    {
        const WindowProps props(title, width, height);
        Init(props);
        m_Data.window = this;
    }

    Window::~Window() = default;


    void Window::Init(const WindowProps& props)
    {
        m_Data.title = props.title;
        m_Data.width = props.width;
        m_Data.height = props.height;
        
        if (!S_GLFW_INITIALIZED)
        {
            [[maybe_unused]] const int success = glfwInit();
            S_GLFW_INITIALIZED = true;
        }
        
        m_Window = glfwCreateWindow(static_cast<int>(props.width), static_cast<int>(props.height), props.title.c_str(), nullptr, nullptr);
        
        glfwSetWindowUserPointer(m_Window, &m_Data);
        SetVSync(true);
        glfwMakeContextCurrent(m_Window);
        glfwSetWindowCloseCallback(m_Window, onWindowClose);
        glfwMaximizeWindow(m_Window);
    }

    void Window::Shutdown() const
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(m_Window);
        const auto app = Application::Get();
        app->Close();
    }
    
    void Window::InitImGui()
    {
        ImGui::CreateContext();
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
        //startExampleFsm();
        
        AddFont("Cascadia_20",
            ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCode.ttf", 20));
        AddFont("Cascadia_18",
            ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCode.ttf", 18));
        AddFont("Cascadia_16",
            ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCode.ttf", 16));
        AddFont("Cascadia_14",
            ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCode.ttf", 14));
        AddFont("Cascadia_12",
            ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCode.ttf", 12));
        AddFont("Cascadia_10",
            ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCode.ttf", 10));
        FONT = GetFont("Cascadia_18");
        NodeEditor::Get()->SetFont(GetFont("Cascadia_18"));

        m_TextEditor = std::make_shared<TextEditor>();
        auto palette = m_TextEditor->GetPalette();
        palette.at(static_cast<int>(TextEditor::PaletteIndex::Background)) =
            ImGui::ColorConvertFloat4ToU32({0.12f, 0.12f, 0.12f, 1.f});
        palette.at(static_cast<int>(TextEditor::PaletteIndex::Default)) =
            ImGui::ColorConvertFloat4ToU32({0.61f, 0.83f, 0.99f, 1.f});
        palette.at(static_cast<int>(TextEditor::PaletteIndex::Punctuation)) =
            ImGui::ColorConvertFloat4ToU32({1.f, 1.f, 1.f, 1.f});
        palette.at(static_cast<int>(TextEditor::PaletteIndex::Identifier)) =
            ImGui::ColorConvertFloat4ToU32({0.66f, 0.89f, 1.f, 1.f});
        m_TextEditor->SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        m_Palette = palette;
        m_TextEditor->SetPalette(m_Palette);
        //m_TextEditor->SetText(NodeEditor::Get()->GetCurrentFsm()->GetLuaCode());
    }

    

    void Window::BeginImGui()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::PushFont(FONT);
    }

    bool OPENED = true;
    bool OPENED2 = true;
    bool ADD_STATE_POPUP = false;
    bool ADD_FSM_POPUP = false;
    bool OPEN_FILE_DIALOG = false;
    bool SAVE_FSM = false;

    bool ADD_NEW_STATE_AT_CURSOR = false;
    bool ADD_NEW_TRIGGER_AT_CURSOR = false;
    ImVec2 CURSOR_POS = {0, 0};
    
    void Window::TrimTrailingNewlines(std::string& str)
    {
        while (!str.empty() && (str.back() == '\n' || str.back() == '\r'))
        {
            str.pop_back();
        }
    }
    
    void Window::OnImGuiRender()
    {
        const auto nodeEditor = NodeEditor::Get();
        const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New..."))
                {
                    ADD_FSM_POPUP = true;
                }
                if (ImGui::MenuItem("Load..."))
                {
                    OPEN_FILE_DIALOG = true;
                }
                if (ImGui::MenuItem("Save..."))
                {
                    SAVE_FSM = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                {
                    SHUTDOWN = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (ADD_FSM_POPUP)
        {
            ImGui::OpenPopup("Add FSM");
            ADD_FSM_POPUP = false;
        }

        if (SAVE_FSM)
        {
            ImGui::OpenPopup("Save FSM");
            SAVE_FSM = false;
        }

        if (OPEN_FILE_DIALOG)
        {
            ImGui::OpenPopup("Open File");
            OPEN_FILE_DIALOG = false;
        }

        if (ImGui::BeginPopup("Save FSM"))
        {
            static std::string filePath;
            if (filePath.empty())
            {
                IGFD::FileDialogConfig config;
                config.path = ".";
                ImGuiFileDialog::Instance()->OpenDialog("SaveFile", "Save File", ".json", config);
                if (ImGuiFileDialog::Instance()->Display("SaveFile")) {
                    if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
                        filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                        if (filePath.find(".json") == std::string::npos)
                            filePath += ".json";
                    }
                    // close
                    ImGuiFileDialog::Instance()->Close();
                }
                nlohmann::json j;
                if (nodeEditor->GetCurrentFsm())
                    j = nodeEditor->GetCurrentFsm()->Serialize();
                //save file
                std::ofstream file(filePath);
                file << j.dump(4);
                file.close();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Add FSM"))
        {
            ImGui::Text("Add FSM");
            static std::string fsmId;
            ImGui::InputText("FSM ID", &fsmId, 0);
            static std::string fsmName;
            ImGui::InputText("FSM Name", &fsmName, 0);
            if (!fsmId.empty() && !fsmName.empty())
            {
                if (ImGui::Button("Add"))
                {
                    const auto fsm = std::make_shared<Fsm>(fsmId);
                    fsm->SetName(fsmName);
                    NodeEditor::Get()->SetCurrentFsm(fsm);
                    ImGui::CloseCurrentPopup();
                }
            }
            else if (fsmId.empty())
            {
                ImGui::Text("FSM ID cannot be empty");
            }
            else if (fsmName.empty())
            {
                ImGui::Text("FSM Name cannot be empty");
            }
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Open File"))
        {
            ImGui::Text("Open File");
            static std::string filePath;
            if (filePath.empty())
            {
                IGFD::FileDialogConfig config;
                config.path = ".";
                ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".json", config);
                if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
                    if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
                        filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                    }
                    // close
                    ImGuiFileDialog::Instance()->Close();
                }
                if (!filePath.empty())
                {
                    //deserialize file
                    auto string = FileReader::ReadAllText(filePath);
                    const auto json = nlohmann::json::parse(string);
                    const auto fsm = Fsm::Deserialize(json);
                    NodeEditor::Get()->SetCurrentFsm(fsm);
                }
            }
            ImGui::EndPopup();
        }

        // Set window position and size
        ImGui::SetNextWindowPos(ImVec2(mainViewport->WorkPos.x, mainViewport->WorkPos.y + 30), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(mainViewport->WorkSize.x, mainViewport->WorkSize.y - 30), ImGuiCond_Once);

        // Main window encompassing all elements
        ImGui::Begin("Node Editor", &OPENED, ImGuiWindowFlags_MenuBar
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoResize);

        // Second-level menu bar
        if (ImGui::BeginMenuBar())
        {
            if (NodeEditor::Get()->GetCurrentFsm() && ImGui::Button("FSM"))
            {
                NodeEditor::Get()->DeselectAllNodes();
            }
            if (NodeEditor::Get()->GetCurrentFsm() && ImGui::Button("Add State"))
            {
                ADD_STATE_POPUP = true;
            }
            ImGui::EndMenuBar();
        }

        if (SHOW_METRICS)
        {
            ImGui::ShowMetricsWindow(&SHOW_METRICS);
        }

        if (ADD_STATE_POPUP)
        {
            ImGui::OpenPopup("Add State");
            ADD_STATE_POPUP = false;
        }

        if (ImGui::BeginPopup("Add State"))
        {
            ImGui::Text("Add State");
            static std::string stateId;
            ImGui::InputText("State ID", &stateId, 0);
            static std::string stateName;
            ImGui::InputText("State Name", &stateName, 0);
            if (!nodeEditor->GetCurrentFsm()->GetState(stateId) && !stateId.empty() && !stateName.empty())
            {
                if (ImGui::Button("Add"))
                {
                    const auto state = std::make_shared<FsmState>(stateId);
                    state->SetName(stateName);
                    nodeEditor->GetCurrentFsm()->AddState(state);
                    ImGui::CloseCurrentPopup();
                }
            }
            else if (nodeEditor->GetCurrentFsm()->GetState(stateId))
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
                    nodeEditor->GetCurrentFsm()->AddState(newState);
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndPopup();
        }

        

        // Dock space setup
        ImGuiID dockspaceId = ImGui::GetID("NodeEditorDockspace");
        ImGui::DockSpace(dockspaceId, ImVec2(0, 0), ImGuiDockNodeFlags_None);

        // Initialize Docking Layout (only run once)
        if (!ImGui::DockBuilderGetNode(dockspaceId))
        {
            ImGui::DockBuilderRemoveNode(dockspaceId); // Clear out existing layout
            ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_None); // Add a new node
            ImGui::DockBuilderSetNodeSize(dockspaceId, mainViewport->Size);

            const ImGuiID dockIdLeft = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.5f, nullptr, &dockspaceId);
            const ImGuiID dockIdRight = dockspaceId;

            ImGui::DockBuilderDockWindow("Canvas", dockIdLeft);
            ImGui::DockBuilderDockWindow("Code", dockIdRight);

            ImGui::DockBuilderFinish(dockspaceId);
        }
        ImGui::End();// Define a struct to represent a node

        // Canvas window
        ImGui::SetNextWindowContentSize({4096, 4096});
        ImGui::PushFont(nodeEditor->GetFont());
        ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus| ImGuiWindowFlags_NoScrollbar);
        if (!NodeEditor::Get()->GetCurrentFsm())
            ImGui::End();
        else
        {

            nodeEditor->SetCanvasPos(ImGui::GetWindowPos());
            nodeEditor->SetCanvasSize(ImGui::GetWindowSize());
            for (const auto& [key, state] : nodeEditor->GetCurrentFsm()->GetStates())
                state->DrawNode();
            for (const auto& [key, trigger] : nodeEditor->GetCurrentFsm()->GetTriggers())
            {
                trigger->DrawNode();
                if (trigger->GetCurrentState())
                {
                    if (const auto node = trigger->GetCurrentState()->GetNode())
                    {
                        if (!node->IsInvisible() && !trigger->GetNode()->IsInvisible())
                            NodeEditor::DrawConnection(node, trigger->GetNode());
                    }
                }
                if (trigger->GetNextState())
                {
                    if (const auto nextNode = trigger->GetNextState()->GetNode())
                    {
                        if (!nextNode->IsInvisible() && !trigger->GetNode()->IsInvisible())
                            NodeEditor::DrawConnection(trigger->GetNode(), nextNode);
                    }
                }
            }
            if (const auto selectedNode = nodeEditor->GetSelectedNode();
                selectedNode && ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right))
            {
                NodeEditor::DrawLine(selectedNode->GetFromPoint(ImGui::GetMousePos()), ImGui::GetMousePos());
                NodeEditor::Get()->SetCreatingLink(true);
            }
            else if (ImGui::IsWindowHovered() && !ImGui::IsMouseDown(ImGuiMouseButton_Right) && NodeEditor::Get()->IsCreatingLink())
            {
                
                if (const auto otherNode = nodeEditor->GetSelectedNode(); otherNode)
                {
                    switch (otherNode->GetType())
                    {
                        case NodeType::State:
                        {
                                CURSOR_POS = ImGui::GetMousePos();
                                ADD_NEW_TRIGGER_AT_CURSOR = true;
                                break;
                        }
                        case NodeType::Transition:
                        {
                                CURSOR_POS = ImGui::GetMousePos();
                                ADD_NEW_STATE_AT_CURSOR = true;
                            break;
                        }
                    }
                }
                NodeEditor::Get()->SetCreatingLink(false);
            }

            if (ADD_NEW_STATE_AT_CURSOR)
            {
                ImGui::OpenPopup("Add State At Cursor");
                ADD_NEW_STATE_AT_CURSOR = false;
            }

            if (ADD_NEW_TRIGGER_AT_CURSOR)
            {
                ImGui::OpenPopup("Add Trigger At Cursor");
                ADD_NEW_TRIGGER_AT_CURSOR = false;
            }

            if (ImGui::BeginPopup("Add State At Cursor"))
            {
                if (const auto fromNode = NodeEditor::Get()->GetSelectedNode(); fromNode && fromNode->GetType() == NodeType::Transition)
                {
                    static std::string id;
                    ImGui::InputText("id", &id);
                    static std::string name;
                    ImGui::InputText("name", &name);
                    if (!id.empty() && !name.empty() && !NodeEditor::Get()->GetCurrentFsm()->GetState(id) && ImGui::Button("Add State"))
                    {
                        const auto state = NodeEditor::Get()->GetCurrentFsm()->AddState(id);
                        state->GetNode()->SetTargetPosition(Math::SubtractVec2(CURSOR_POS, {25, 50}));
                        state->SetName(name);
                        const auto trigger = NodeEditor::Get()->GetCurrentFsm()->GetTrigger(fromNode->GetId());
                        trigger->SetNextState(state->GetId());
                        name = "";
                        id = "";
                        ImGui::CloseCurrentPopup();
                    }
                    else if (id.empty())
                    {
                        ImGui::Text("ID can not be empty.");
                    }
                    else if (name.empty())
                    {
                        ImGui::Text("Name can not be empty.");
                    }
                    else if (NodeEditor::Get()->GetCurrentFsm()->GetState(id))
                    {
                        ImGui::Text("State with ID already exists.");
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel"))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("Add Trigger At Cursor"))
            {
                if (const auto fromNode = NodeEditor::Get()->GetSelectedNode(); fromNode && fromNode->GetType() == NodeType::State)
                {
                    static std::string id;
                    ImGui::InputText("id", &id);
                    static std::string name;
                    ImGui::InputText("name", &name);
                    if (!id.empty() && !name.empty() && !NodeEditor::Get()->GetCurrentFsm()->GetTrigger(id) && ImGui::Button("Add Trigger"))
                    {
                        const auto state = NodeEditor::Get()->GetCurrentFsm()->GetState(fromNode->GetId());
                        const auto trigger = state->AddTrigger(id);
                        trigger->GetNode()->SetTargetPosition(Math::SubtractVec2(CURSOR_POS, {25, 50}));
                        trigger->SetName(name);
                        name = "";
                        id = "";
                        ImGui::CloseCurrentPopup();
                    }
                    else if (id.empty())
                    {
                        ImGui::Text("ID can not be empty.");
                    }
                    else if (name.empty())
                    {
                        ImGui::Text("Name can not be empty.");
                    }
                    else if (NodeEditor::Get()->GetCurrentFsm()->GetTrigger(id))
                    {
                        ImGui::Text("Trigger with ID already exists.");
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel"))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }
            ImGui::End();
        }
        ImGui::PopFont();

        // Code editor window
        ImGui::Begin("Code", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus);
        {
            if (const auto node = nodeEditor->GetSelectedNode())
            {
                switch (node->GetType())
                {
                    case NodeType::State:
                    {
                        if (const auto state = nodeEditor->GetCurrentFsm()->GetState(node->GetId()))
                            state->DrawProperties();
                        break;
                    }
                    case NodeType::Transition:
                    {
                        if (const auto trigger = nodeEditor->GetCurrentFsm()->GetTrigger(node->GetId()))
                            trigger->DrawProperties();
                        break;
                    }
                }
            }
            if (nodeEditor->GetCurrentFsm() && !nodeEditor->GetSelectedNode())
                NodeEditor::Get()->SetShowFsmProps(true);
            if (nodeEditor->GetCurrentFsm() && nodeEditor->ShowFsmProps())
                nodeEditor->GetCurrentFsm()->DrawProperties();
            ImGui::End();
        }
    }

    void Window::DrawTextEditor(TextEditor& txtEditor, std::string& oldText)
    {
        txtEditor.SetPalette(m_Palette);
        if (strcmp(txtEditor.GetText().c_str(), oldText.c_str()) != 0)
            txtEditor.SetText(oldText);
        //TrimTrailingNewlines(oldText);
        txtEditor.Render("Code");
        if (txtEditor.IsTextChanged())
            oldText = txtEditor.GetText();
    }
    
    void Window::EndImGui() const
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
        if (SHUTDOWN)
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
    
    void Window::OnUpdate() const
    {
        glfwPollEvents();
        BeginImGui();
        OnImGuiRender();
        EndImGui();
    }

    
}
