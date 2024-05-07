﻿#include "pch.h"
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
    std::unordered_map<std::string, Window::ImGuiColorTheme> Window::m_Themes = {};
    std::string Window::m_ActiveTheme;

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
        InitThemes();
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
        SetTheme("Red");
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
    bool SAVE_FSM_LUA = false;
    bool SHOW_STYLE_EDITOR = false;
    bool THEME_EDITOR = false;

    bool ADD_NEW_STATE_AT_CURSOR = false;
    bool ADD_NEW_TRIGGER_AT_CURSOR = false;
    bool DOCK_SPACE_SET = false;
    ImVec2 CURSOR_POS = {0, 0};
    std::string LAST_SAVE_PATH;
    std::string LAST_EXPORT_PATH;
    std::string LAST_OPEN_PATH;
    std::string LAST_PATH;
    
    void Window::TrimTrailingNewlines(std::string& str)
    {
        while (!str.empty() && (str.back() == '\n' || str.back() == '\r'))
        {
            str.pop_back();
        }
    }
    
    void Window::OnImGuiRender()
    {
        
        //ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.33f, 0.33f, 0.38f, 1.0f));
        const auto nodeEditor = NodeEditor::Get();
        const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New FSM"))
                {
                    ADD_FSM_POPUP = true;
                }
                if (ImGui::MenuItem("Load (JSON)"))
                {
                    OPEN_FILE_DIALOG = true;
                }
                if (ImGui::MenuItem("Save (JSON)"))
                {
                    SAVE_FSM = true;
                }
                if (ImGui::MenuItem("Export (Lua)"))
                {
                    SAVE_FSM_LUA = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit"))
                {
                    SHUTDOWN = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Themes"))
            {
                for (const auto& name : m_Themes | std::views::keys)
                {
                    if (ImGui::MenuItem(name.c_str()))
                    {
                        SetTheme(name);
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::Button("Edit Theme"))
                THEME_EDITOR = !THEME_EDITOR;
            
            
            ImGui::EndMainMenuBar();
        }

        if (ADD_FSM_POPUP)
        {
            ImGui::OpenPopup("Add FSM");
            ADD_FSM_POPUP = false;
        }

        if (THEME_EDITOR)
        {
            ImGui::OpenPopup("Theme Editor");
            THEME_EDITOR = false;
        }

        if (SAVE_FSM)
        {
            ImGui::OpenPopup("Save FSM");
            SAVE_FSM = false;
        }

        if (SAVE_FSM_LUA)
        {
            ImGui::OpenPopup("Export Lua");
            SAVE_FSM_LUA = false;
        }

        if (OPEN_FILE_DIALOG)
        {
            ImGui::OpenPopup("Open File");
            OPEN_FILE_DIALOG = false;
        }

        if (ImGui::BeginPopup("Theme Editor"))
        {
            if (!m_ActiveTheme.empty())
            {
                static auto [standard, hovered, active, text, background, windowBg, themeName] = GetTheme(m_ActiveTheme);
                ImGui::ColorEdit4("standard", reinterpret_cast<float*>(&standard));
                ImGui::ColorEdit4("hovered", reinterpret_cast<float*>(&hovered));
                ImGui::ColorEdit4("active", reinterpret_cast<float*>(&active));
                ImGui::ColorEdit4("text", reinterpret_cast<float*>(&text));
                ImGui::ColorEdit4("background", reinterpret_cast<float*>(&background));
                ImGui::ColorEdit4("windowBg", reinterpret_cast<float*>(&windowBg));
                SetTheme({standard, hovered, active, text, background, windowBg, ""});
                static std::string name;
                ImGui::InputText("Theme Name", &name);
                if (!name.empty() && ImGui::Button("Save"))
                {
                    auto theme = ImGuiColorTheme{standard, hovered, active, text, background, windowBg, name};
                    AddTheme(name, theme);
                    SetTheme(name);
                    auto json = theme.Serialize();
                    std::ofstream file("assets/themes/" + name + ".json");
                    file << json.dump(4);
                    file.close();
                    name = "";
                }
                if (ImGui::Button("Close"))
                {
                    SetTheme(m_ActiveTheme);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        

        if (ImGui::BeginPopup("Save FSM"))
        {
            if (!nodeEditor->GetCurrentFsm())
                return;
            static std::string filePath;
            static std::string path;
            static std::string folder;
            if (filePath.empty())
            {
                IGFD::FileDialogConfig config;
                if (!LAST_SAVE_PATH.empty())
                    config.filePathName = LAST_SAVE_PATH;
                else if (!LAST_OPEN_PATH.empty())
                    config.filePathName = LAST_OPEN_PATH;
                else
                {
                    if(!LAST_PATH.empty())
                        config.path = LAST_PATH;
                    else
                        config.path = ".";
                    config.fileName = nodeEditor->GetCurrentFsm()->GetId() + ".json";
                }
                ImGuiFileDialog::Instance()->OpenDialog("SaveFile", "Save File", ".json", config);
                if (ImGuiFileDialog::Instance()->Display("SaveFile")) {
                    if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
                        filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                        path = ImGuiFileDialog::Instance()->GetFilePathName();
                        folder = path.substr(0, path.find_last_of("/\\"));
                        if (filePath.find(".json") == std::string::npos)
                            filePath += ".json";
                    }
                    // close
                    ImGuiFileDialog::Instance()->Close();
                    ImGui::CloseCurrentPopup();
                }
                if (!filePath.empty())
                {
                    nlohmann::json j;
                    if (nodeEditor->GetCurrentFsm())
                        j = nodeEditor->GetCurrentFsm()->Serialize();
                    //save file
                    std::ofstream file(filePath);
                    file << j.dump(4);
                    file.close();
                    LAST_SAVE_PATH = path;
                    LAST_PATH = folder;
                    folder = "";
                    filePath = "";
                    path = "";
                }
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Export Lua"))
        {
            if (!nodeEditor->GetCurrentFsm())
                return;
            static std::string filePath;
            static std::string folder;
            if (filePath.empty())
            {
                IGFD::FileDialogConfig config;
                if (!LAST_EXPORT_PATH.empty())
                    config.filePathName = LAST_EXPORT_PATH;
                else
                {
                    if(!LAST_PATH.empty())
                        config.path = LAST_PATH;
                    else
                        config.path = ".";
                    config.fileName = nodeEditor->GetCurrentFsm()->GetId() + ".lua";
                }
                ImGuiFileDialog::Instance()->OpenDialog("exportLua", "Export LUA", ".lua", config);
                if (ImGuiFileDialog::Instance()->Display("exportLua")) {
                    if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
                        filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                        folder = filePath.substr(0, filePath.find_last_of("/\\"));
                        if (filePath.find(".lua") == std::string::npos)
                            filePath += ".lua";
                    }
                    // close
                    ImGuiFileDialog::Instance()->Close();
                    ImGui::CloseCurrentPopup();
                }
                if (!filePath.empty())
                {
                    std::ofstream file(filePath);
                    file << nodeEditor->GetCurrentFsm()->GetLuaCode();
                    file.close();
                    LAST_EXPORT_PATH = filePath;
                    LAST_PATH = folder;
                    folder = "";
                    filePath = "";
                }
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
                    NodeEditor::Get()->DeselectAllNodes();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
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
            static std::string filePath;
            static std::string folder;
            if (filePath.empty())
            {
                IGFD::FileDialogConfig config;
                if (!LAST_OPEN_PATH.empty())
                    config.filePathName = LAST_OPEN_PATH;
                else if (!LAST_SAVE_PATH.empty())
                    config.filePathName = LAST_SAVE_PATH;
                else if (!LAST_PATH.empty())
                    config.path = LAST_PATH;
                else
                    config.path = ".";
                ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".json", config);
                if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
                    if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
                        filePath = ImGuiFileDialog::Instance()->GetFilePathName();
                        folder = filePath.substr(0, filePath.find_last_of("/\\"));
                    }
                    // close
                    ImGuiFileDialog::Instance()->Close();
                    ImGui::CloseCurrentPopup();
                }
                if (!filePath.empty())
                {
                    //deserialize file
                    auto string = FileReader::ReadAllText(filePath);
                    const auto json = nlohmann::json::parse(string);
                    const auto fsm = Fsm::Deserialize(json);
                    NodeEditor::Get()->SetCurrentFsm(fsm);
                    NodeEditor::Get()->DeselectAllNodes();
                    LAST_OPEN_PATH = filePath;
                    LAST_PATH = folder;
                    folder = "";
                    filePath = "";
                }
            }
            ImGui::EndPopup();
        }

        // Set window position and size
        ImGui::SetNextWindowPos(ImVec2(mainViewport->WorkPos.x, mainViewport->WorkPos.y + 30), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(mainViewport->WorkSize.x, mainViewport->WorkSize.y - 30), ImGuiCond_Once);

        // Main window encompassing all elements
        ImGui::Begin("Node Editor", &OPENED,
            ImGuiWindowFlags_MenuBar
            | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoResize);

        // Second-level menu bar
        if (ImGui::BeginMenuBar())
        {
            if (auto fsm = NodeEditor::Get()->GetCurrentFsm())
            {
                ImGui::Text(fsm->GetName().c_str());
                if (ImGui::Button("FSM Properties"))
                    NodeEditor::Get()->DeselectAllNodes();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 1.0f));
                if (ImGui::Button("Add State"))
                    ADD_STATE_POPUP = true;
                ImGui::PopStyleColor();
            }
            //if (ImGui::Button("Style Editor"))
            //    SHOW_STYLE_EDITOR = !SHOW_STYLE_EDITOR;
            ImGui::EndMenuBar();
        }

        if (SHOW_METRICS)
        {
            ImGui::ShowMetricsWindow(&SHOW_METRICS);
        }

        if (SHOW_STYLE_EDITOR)
        {
            ImGui::ShowStyleEditor();
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
                ImGui::SameLine();
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
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        

        // Dock space setup
        ImGuiID dockspaceId = ImGui::GetID("NodeEditorDockspace");
        ImGui::DockSpace(dockspaceId);

        //if (NodeEditor::Get()->GetCurrentFsm())
        //{
            // Initialize Docking Layout (only run once)
            if (!DOCK_SPACE_SET)
            {
                ImGui::DockBuilderRemoveNode(dockspaceId); // Clear out existing layout
                ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_None); // Add a new node
                ImGui::DockBuilderSetNodeSize(dockspaceId, mainViewport->Size);

                const ImGuiID dockIdLeft = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.75f, nullptr, &dockspaceId);
                const ImGuiID dockIdRight = dockspaceId;

                ImGui::DockBuilderDockWindow("Canvas", dockIdLeft);
                ImGui::DockBuilderDockWindow("Code", dockIdRight);

                ImGui::DockBuilderFinish(dockspaceId);
                DOCK_SPACE_SET = true;
            }
        //}
        ImGui::End();// Define a struct to represent a node

        // Canvas window
        ImGui::SetNextWindowContentSize({4096, 4096});
        ImGui::PushFont(nodeEditor->GetFont());
        ImGui::Begin("Canvas", nullptr,
            ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoScrollbar);
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
            else if (ImGui::IsWindowHovered(ImGuiHoveredFlags_Stationary) && !ImGui::IsMouseDown(ImGuiMouseButton_Right) && NodeEditor::Get()->IsCreatingLink())
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
            else if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
                NodeEditor::Get()->SetCreatingLink(false);

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
            if (const auto node = nodeEditor->GetSelectedNode(); node)
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

        //ImGui::PopStyleColor();
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

    void Window::InitThemes()
    {
        for (const auto path = "assets/themes/"; const auto& entry : std::filesystem::directory_iterator(path))
        {
            if (entry.path().extension() == ".json")
            {
                auto string = FileReader::ReadAllText(entry.path().string());
                const auto json = nlohmann::json::parse(string);
                auto theme = ImGuiColorTheme::Deserialize(json);
                AddTheme(theme.name, theme);
            }
        }
    }

    void Window::AddTheme(const std::string& name, const ImGuiColorTheme& theme)
    {
        m_Themes[name] = theme;
    }

    Window::ImGuiColorTheme Window::GetTheme(const std::string& name)
    {
        if (m_Themes.contains(name))
            return m_Themes[name];
        return {};
    }

    void Window::SetTheme(const std::string& name)
    {
        if (m_Themes.contains(name))
        {
            SetTheme(m_Themes[name]);
            m_ActiveTheme = name;
        }
    }

    void Window::SetTheme(const ImGuiColorTheme& theme)
    {
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text]                   = theme.text;
        colors[ImGuiCol_WindowBg]               = theme.windowBg;
        colors[ImGuiCol_PopupBg]                = ImVec4(theme.background.x / 2, theme.background.y / 2, theme.background.z / 2, theme.background.w);
        colors[ImGuiCol_Border]                 = theme.active;
        colors[ImGuiCol_FrameBg]                = theme.background;
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(theme.hovered.x / 2, theme.hovered.y / 2, theme.hovered.z / 2, theme.hovered.w);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(theme.active.x / 2, theme.active.y / 2, theme.active.z / 2, theme.active.w);
        colors[ImGuiCol_MenuBarBg]              = theme.background;
        colors[ImGuiCol_TitleBg]                = ImVec4(theme.background.x / 2, theme.background.y / 2, theme.background.z / 2, theme.background.w);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(theme.active.x / 2, theme.active.y / 2, theme.active.z / 2, theme.active.w);
        colors[ImGuiCol_ScrollbarGrab]          = theme.standard;
        colors[ImGuiCol_ScrollbarGrabHovered]   = theme.hovered;
        colors[ImGuiCol_ScrollbarGrabActive]    = theme.active;
        colors[ImGuiCol_CheckMark]              = theme.standard;
        colors[ImGuiCol_SliderGrab]             = theme.standard;
        colors[ImGuiCol_SliderGrabActive]       = theme.active;
        colors[ImGuiCol_Button]                 = theme.standard;
        colors[ImGuiCol_ButtonHovered]          = theme.hovered;
        colors[ImGuiCol_ButtonActive]           = theme.active;
        colors[ImGuiCol_Header]                 = theme.standard;
        colors[ImGuiCol_HeaderHovered]          = theme.hovered;
        colors[ImGuiCol_HeaderActive]           = theme.active;
        colors[ImGuiCol_Separator]              = theme.standard;
        colors[ImGuiCol_SeparatorHovered]       = theme.hovered;
        colors[ImGuiCol_SeparatorActive]        = theme.active;
        colors[ImGuiCol_ResizeGrip]             = theme.standard;
        colors[ImGuiCol_ResizeGripHovered]      = theme.hovered;
        colors[ImGuiCol_ResizeGripActive]       = theme.active;
        colors[ImGuiCol_Tab]                    = theme.standard;
        colors[ImGuiCol_TabHovered]             = theme.hovered;
        colors[ImGuiCol_TabActive]              = theme.active;
        colors[ImGuiCol_TabUnfocused]           = theme.standard;
        colors[ImGuiCol_TabUnfocusedActive]     = theme.active;
        colors[ImGuiCol_DockingPreview]         = theme.standard;
    }

    void Window::RemoveTheme(const std::string& name)
    {
        if (m_Themes.contains(name))
            m_Themes.erase(name);
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
