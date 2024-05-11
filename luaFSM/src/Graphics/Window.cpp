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
#include "imgui/ImGuiNotify.hpp"
#include "imgui/IconsFontAwesome6.h"

//callback for window close glfw
bool SHUTDOWN = false;
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

    //static data initializers
    TextEditor::Palette Window::m_Palette = {};
    std::unordered_map<std::string, ImFont*> Window::m_Fonts = {};
    std::unordered_map<std::string, Window::ImGuiColorTheme> Window::m_Themes = {};
    std::string Window::m_ActiveTheme;

    bool MAIN_DOCK_OPENED = true;
    bool ADD_STATE_POPUP = false;
    bool ADD_FSM_POPUP = false;
    bool LOAD_FILE = false;
    bool SELECT_FILE = false;
    bool OPEN_THEME_EDITOR = false;
    
    bool DOCK_SPACE_SET = false;
    
    ImVec2 CURSOR_POS = {0, 0};
    bool ADD_NEW_STATE_AT_CURSOR = false;
    bool ADD_NEW_TRIGGER_AT_CURSOR = false;
    bool PASTE_NEW_STATE_AT_CURSOR = false;
    bool PASTE_NEW_TRIGGER_AT_CURSOR = false;
    
    std::string LAST_SAVE_PATH;
    std::string LAST_OPEN_PATH;
    std::string LAST_PATH;
    
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
        Application::Get()->Close();
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
        AddFont("Cascadia_16",
            ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCode.ttf", 16));
        AddFont("Cascadia_14",
            ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCode.ttf", 14));
        AddFont("Cascadia_12",
            ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCode.ttf", 12));
        AddFont("Cascadia_10",
        ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCode.ttf", 10));
        AddFont("Cascadia_18",
            ImGui::GetIO().Fonts->AddFontFromFileTTF("assets/fonts/CascadiaCode.ttf", 18));
        io.FontDefault = FONT = GetFont("Cascadia_18");
        NodeEditor::Get()->SetFont(GetFont("Cascadia_18"));
        constexpr float baseFontSize = 18.0f; // Default font size
        constexpr float iconFontSize = baseFontSize * 2.0f / 3.0f; // FontAwesome fonts need to have their sizes reduced by 2.0f/3.0f in order to align correctly

        // Check if FONT_ICON_FILE_NAME_FAS is a valid path

        if (const std::ifstream fontAwesomeFile(FONT_ICON_FILE_NAME_FAS); !fontAwesomeFile.good())
        {
            // If it's not good, then we can't find the font and should abort
            std::cerr << "Could not find the FontAwesome font file." << '\n';
            abort();
        }

        static constexpr ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
        ImFontConfig iconsConfig;
        iconsConfig.MergeMode = true;
        iconsConfig.PixelSnapH = true;
        iconsConfig.GlyphMinAdvanceX = iconFontSize;
        AddFont("notificationFont",
            ImGui::GetIO().Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, iconFontSize, &iconsConfig, icons_ranges));

        

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
        SetTheme("Green");
    }
    
    void Window::BeginImGui()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::PushFont(FONT);
    }

    
    void Window::OnImGuiRender()
    {
        RenderNotifications();
        MainMenu();
        MainDockSpace();
        Canvas();
        Properties();
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
    
    void Window::RenderNotifications()
    {
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.10f, 1.00f)); // Background color
        ImGui::PushFont(m_Fonts["notificationFont"]);
        ImGui::RenderNotifications();
        ImGui::PopStyleColor(1);
        ImGui::PopFont();
    }

    void Window::MainMenu()
    {
        const auto nodeEditor = NodeEditor::Get();
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New FSM"))
                {
                    ADD_FSM_POPUP = true;
                }
                if (auto fsm = nodeEditor->GetCurrentFsm(); fsm && fsm->GetLinkedFile().empty() && ImGui::MenuItem("Create Lua File"))
                {
                    SELECT_FILE = true;
                }
                if (ImGui::MenuItem("Load"))
                {
                    LOAD_FILE = true;
                }
                if (auto fsm = nodeEditor->GetCurrentFsm(); fsm && !fsm->GetLinkedFile().empty() && ImGui::MenuItem("Save"))
                {
                    nodeEditor->GetCurrentFsm()->UpdateToFile();
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
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 0.0f));
            if (ImGui::Button("Edit Theme"))
                OPEN_THEME_EDITOR = !OPEN_THEME_EDITOR;
            ImGui::PopStyleColor();
            if (auto fsm = NodeEditor::Get()->GetCurrentFsm())
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 0.0f));
                if (ImGui::Button("FSM Properties"))
                    NodeEditor::Get()->DeselectAllNodes();
                ImGui::PopStyleColor();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 1.0f));
                if (ImGui::Button("Add State"))
                    ADD_STATE_POPUP = true;
                ImGui::PopStyleColor();
                ImGui::Text("Current FSM: ");
                ImGui::Text(fsm->GetName().c_str());
            }
            /*
            * 
            ImGui::Text("Scale: ");
            ImGui::SameLine();
            ImGui::Text("%.2f", nodeEditor->GetScale());
             */
            ImGui::EndMainMenuBar();
        }
        
        if (OPEN_THEME_EDITOR)
        {
            ImGui::OpenPopup("Theme Editor");
            OPEN_THEME_EDITOR = false;
        }
        
        if (ImGui::BeginPopup("Theme Editor"))
        {
            if (!m_ActiveTheme.empty())
            {
                static std::string name = m_ActiveTheme;
                static std::string lastActiveTheme = m_ActiveTheme;
                static auto [standard, hovered, active, text, background, windowBg, themeName] = GetTheme(m_ActiveTheme);
                if (m_ActiveTheme != lastActiveTheme)
                {
                    lastActiveTheme = m_ActiveTheme;
                    standard = GetTheme(m_ActiveTheme).standard;
                    hovered = GetTheme(m_ActiveTheme).hovered;
                    active = GetTheme(m_ActiveTheme).active;
                    text = GetTheme(m_ActiveTheme).text;
                    background = GetTheme(m_ActiveTheme).background;
                    windowBg = GetTheme(m_ActiveTheme).windowBg;
                    name = m_ActiveTheme;
                }
                ImGui::ColorEdit4("standard", reinterpret_cast<float*>(&standard));
                ImGui::ColorEdit4("hovered", reinterpret_cast<float*>(&hovered));
                ImGui::ColorEdit4("active", reinterpret_cast<float*>(&active));
                ImGui::ColorEdit4("text", reinterpret_cast<float*>(&text));
                ImGui::ColorEdit4("background", reinterpret_cast<float*>(&background));
                ImGui::ColorEdit4("windowBg", reinterpret_cast<float*>(&windowBg));
                SetTheme({standard, hovered, active, text, background, windowBg, ""});
                ImGui::InputText("Theme Name", &name);
                if (!name.empty())
                {
                    if (ImGui::Button("Save"))
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
                    ImGui::SameLine();
                }
                else
                    ImGui::Text("Input a name to save the theme.");
                if (ImGui::Button("Close"))
                {
                    SetTheme(m_ActiveTheme);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        
        if (ADD_FSM_POPUP)
        {
            ImGui::OpenPopup("Add FSM");
            ADD_FSM_POPUP = false;
        }
        
        if (ImGui::BeginPopup("Add FSM"))
        {
            ImGui::Text("Add FSM");
            static std::string fsmId;
            ImGui::InputText("FSM ID", &fsmId, 0);
            ImGui::SetItemTooltip("Unique identifier for the FSM. This is used as the global variable name in Lua.");
            static std::string fsmName;
            ImGui::InputText("FSM Name", &fsmName, 0);
            ImGui::SetItemTooltip("Just for visual purposes in this editor.");
            if (auto invalid = std::regex_search(fsmId, FsmRegex::InvalidIdRegex());
                !fsmId.empty()
                && !fsmName.empty()
                && !invalid)
            {
                if (ImGui::Button("Add"))
                {
                    const auto fsm = std::make_shared<Fsm>(fsmId);
                    fsm->SetName(fsmName);
                    NodeEditor::Get()->SetCurrentFsm(fsm);
                    NodeEditor::Get()->DeselectAllNodes();
                    ImGui::CloseCurrentPopup();
                    SELECT_FILE = true;
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
            else if (invalid)
            {
                ImGui::Text("Invalid FSM ID");
            }
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        
        if (LOAD_FILE)
        {
            ImGui::OpenPopup("Open File");
            LOAD_FILE = false;
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
                config.flags |= ImGuiWindowFlags_NoCollapse;
                ImGuiFileDialog::Instance()->OpenDialog("loadLuaFile", "Choose File", ".lua", config);
                if (ImGuiFileDialog::Instance()->Display("loadLuaFile", ImGuiWindowFlags_NoCollapse, ImVec2({600, 500}) )) {
                    if (ImGuiFileDialog::Instance()->IsOk()) { // action if 
                        folder = ImGuiFileDialog::Instance()->GetCurrentPath();
                        filePath = folder + "/";
                        filePath += ImGuiFileDialog::Instance()->GetCurrentFileName();
                    }
                    // close
                    ImGuiFileDialog::Instance()->Close();
                    ImGui::CloseCurrentPopup();
                }
                if (!filePath.empty())
                {
                    //deserialize file
                    const auto fsm = Fsm::CreateFromFile(filePath);
                    NodeEditor::Get()->DeselectAllNodes();
                    LAST_OPEN_PATH = filePath;
                    LAST_PATH = folder;
                    folder = "";
                    filePath = "";
                }
            }
            ImGui::EndPopup();
        }
        
        if (SELECT_FILE)
        {
            ImGui::OpenPopup("SelectFile");
            SELECT_FILE = false;
        }
        
        if (ImGui::BeginPopup("SelectFile"))
        {
            static std::string filePath;
            static std::string folder;
            if (!nodeEditor->GetCurrentFsm())
            {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            if (filePath.empty())
            {
                IGFD::FileDialogConfig config;
                if (!LAST_PATH.empty())
                    config.path = LAST_PATH;
                else
                    config.path = ".";
                config.fileName = nodeEditor->GetCurrentFsm()->GetId();
                ImGuiFileDialog::Instance()->OpenDialog("ChooseLuaFile", "Save lua file", ".lua", config);
                if (ImGuiFileDialog::Instance()->Display("ChooseLuaFile", ImGuiWindowFlags_NoCollapse, ImVec2({600, 500}) )) {
                    if (ImGuiFileDialog::Instance()->IsOk()) { // action if 
                        folder = ImGuiFileDialog::Instance()->GetCurrentPath();
                        filePath = folder + "/";
                        filePath += ImGuiFileDialog::Instance()->GetCurrentFileName();
                    }
                    // close
                    ImGuiFileDialog::Instance()->Close();
                    ImGui::CloseCurrentPopup();
                }
                if (!filePath.empty())
                {
                    const auto code = nodeEditor->GetCurrentFsm()->GetLuaCode();
                    std::ofstream file(filePath);
                    if (!file.is_open())
                    {
                        ImGui::InsertNotification({ImGuiToastType::Error, 3000, "Failed to export file at: %s", filePath.c_str()});
                        return;
                    }
                    file << code;
                    file.close();
                    nodeEditor->GetCurrentFsm()->SetLinkedFile(filePath);
                    LAST_SAVE_PATH = filePath;
                    LAST_PATH = folder;
                    folder = "";
                    filePath = "";
                }
            }
            ImGui::EndPopup();
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
            ImGui::SetItemTooltip("Used for internal identification and must be unique. Changing this requires refactor.");
            static std::string stateName;
            ImGui::InputText("State Name", &stateName, 0);
            if (auto invalid = std::regex_search(stateId, FsmRegex::InvalidIdRegex());
                !nodeEditor->GetCurrentFsm()->GetState(stateId)
                && !nodeEditor->GetCurrentFsm()->GetTrigger(stateId)
                && !stateId.empty()
                && !stateName.empty()
                && !invalid)
            {
                if (ImGui::Button("Add"))
                {
                    const auto state = std::make_shared<FsmState>(stateId);
                    state->SetName(stateName);
                    nodeEditor->GetCurrentFsm()->AddState(state);
                    ImGui::SetClipboardText(state->GetLuaCode().c_str());
                    state->AppendToFile();
                    ImGui::InsertNotification({ImGuiToastType::Success, 3000, "State added: %s, code copied to clipboard", stateId.c_str()});
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
            }
            else if (nodeEditor->GetCurrentFsm()->GetState(stateId) || nodeEditor->GetCurrentFsm()->GetTrigger(stateId))
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
            else if (invalid)
            {
                ImGui::Text("Invalid State ID");
            }
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    ImVec2 CANVAS_POS{};
    ImVec2 CANVAS_SIZE{};
    ImVec2 CANVAS_CONTENT_MIN{};
    ImVec2 CANVAS_CONTENT_MAX{};

    void Window::MainDockSpace()
    {
        
        const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        // Set window position and size
        ImGui::SetNextWindowPos(ImVec2(mainViewport->WorkPos.x, mainViewport->WorkPos.y + 30), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(mainViewport->WorkSize.x, mainViewport->WorkSize.y - 30), ImGuiCond_Once);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        // Main window encompassing all elements
        ImGui::Begin("Node Editor", &MAIN_DOCK_OPENED,
            ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_MenuBar
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoResize);
        {
            ImGui::PopStyleVar();
            /* 
            if (ImGui::BeginMenuBar())
            {
                const auto nodeEditor = NodeEditor::Get();
                ImGui::Text("Canvas pos: %.1f, %.1f", nodeEditor->GetCanvasPos().x, nodeEditor->GetCanvasPos().y);
                ImGui::Text("Canvas size: %.1f, %.1f", nodeEditor->GetCanvasSize().x, nodeEditor->GetCanvasSize().y);
                ImGui::Text("Canvas min: %.1f, %.1f", CANVAS_CONTENT_MIN.x, CANVAS_CONTENT_MIN.y);
                ImGui::Text("Canvas max: %.1f, %.1f", CANVAS_CONTENT_MAX.x, CANVAS_CONTENT_MAX.y);
                ImGui::Text("mouse pos: %.1f, %.1f", ImGui::GetMousePos().x, ImGui::GetMousePos().y);
                ImGui::EndMenuBar();
            }*/
            
            // Dock space setup
            ImGuiID dockspaceId = ImGui::GetID("NodeEditorDockspace");
            ImGui::DockSpace(dockspaceId);
            if (!DOCK_SPACE_SET)
            {
                ImGui::DockBuilderRemoveNode(dockspaceId); // Clear out existing layout
                ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_None); // Add a new node
                ImGui::DockBuilderSetNodeSize(dockspaceId, mainViewport->Size);

                const ImGuiID dockIdLeft = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.75f, nullptr, &dockspaceId);
                const ImGuiID dockIdRight = dockspaceId;

                ImGui::DockBuilderDockWindow("Canvas", dockIdLeft);
                ImGui::DockBuilderDockWindow("Properties", dockIdRight);

                ImGui::DockBuilderFinish(dockspaceId);
                DOCK_SPACE_SET = true;
            }
            ImGui::End();
        }
    }

    float MOUSE_SCROLL = 0;
    bool firstOpen = true;
    ImVec2 lastScroll = {0, 0};
    
    void Window::Canvas()
    {
        const auto nodeEditor = NodeEditor::Get();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        // Canvas window
        ImGui::SetNextWindowContentSize({4096, 4096});
        ImGui::PushFont(nodeEditor->GetFont());
        ImGui::Begin("Canvas", nullptr,
            ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoScrollWithMouse
            | ImGuiWindowFlags_NoMove
            );
        if (!NodeEditor::Get()->GetCurrentFsm())
        {
            ImGui::PopStyleVar();
            ImGui::End();
        }
        else
        {
            if (firstOpen)
            {
                ImGui::SetScrollX(1024);
                ImGui::SetScrollY(1024);
                //lastScroll = ImVec2(1024, 1024);
                firstOpen = false;
            }
            if (ImGui::IsWindowHovered() || ImGui::IsWindowFocused())
            {
                if (ImGui::IsKeyPressed(ImGuiKey_S)
                && ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
                {
                    if (!nodeEditor->GetCurrentFsm()->GetLinkedFile().empty())
                        nodeEditor->GetCurrentFsm()->UpdateToFile();
                }
                if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
                {
                    MOUSE_SCROLL = ImGui::GetIO().MouseWheel * 0.025;
                    nodeEditor->SetScale(std::min(1.5f, std::max(nodeEditor->GetScale() + MOUSE_SCROLL, 0.5f)));
                }
                else
                {
                    MOUSE_SCROLL = 0;
                }
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
                {
                    const ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
                    ImGui::SetScrollX(ImGui::GetScrollX() - delta.x);
                    ImGui::SetScrollY(ImGui::GetScrollY() - delta.y);
                    ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
                }
                if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                {
                    if (auto existingCurveNode = nodeEditor->GetCurveNode(); !existingCurveNode)
                    {
                        const auto fsm = nodeEditor->GetCurrentFsm();
                        const auto pressPos = ImGui::GetMousePos();
                        float smallestDist = 40;
                        VisualNode* curveNode = nullptr;
                        bool isInLine = false;
                        for (const auto& [key, condition] : fsm->GetTriggers())
                        {
                            if (Math::Distance(pressPos, condition->GetNode()->GetInLineMidPoint()) < smallestDist)
                            {
                                smallestDist = Math::Distance(pressPos, condition->GetNode()->GetInLineMidPoint());
                                curveNode = condition->GetNode();
                                isInLine = true;
                            }
                            if (Math::Distance(pressPos, condition->GetNode()->GetOutLineMidPoint()) < smallestDist)
                            {
                                smallestDist = Math::Distance(pressPos, condition->GetNode()->GetOutLineMidPoint());
                                curveNode = condition->GetNode();
                                isInLine = false;
                            }
                        }
                        if (curveNode)
                        {
                            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
                            nodeEditor->SetCurveNode(curveNode);
                            if (isInLine)
                            {
                                nodeEditor->SetSettingInCurve(true);
                                nodeEditor->SetSettingOutCurve(false);
                            }
                            else
                            {
                                nodeEditor->SetSettingInCurve(false);
                                nodeEditor->SetSettingOutCurve(true);
                            }
                        }
                    }
                    else if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                    {
                        //Literally trial and error no clue why this works
                        if (const auto trigger = nodeEditor->GetCurrentFsm()->GetTrigger(existingCurveNode->GetId()))
                        {
                            if (nodeEditor->IsSettingInCurve())
                            {
                                auto newCurve = existingCurveNode->GetInArrowCurve();
                                if (const auto toState = trigger->GetCurrentState())
                                {
                                    const auto statePos = toState->GetNode()->GetGridPos();
                                    if ( statePos.y < existingCurveNode->GetGridPos().y)
                                    {
                                        if (statePos.x > existingCurveNode->GetGridPos().x)
                                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / 350;
                                        else
                                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / 350;
                                    }
                                    else
                                    {
                                        if (statePos.x < existingCurveNode->GetGridPos().x)
                                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / 350;
                                        else
                                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / 350;
                                    }
                                    if (statePos.x > existingCurveNode->GetGridPos().x)
                                    {
                                        if (statePos.y < existingCurveNode->GetGridPos().y)
                                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / 350;
                                        else
                                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / 350;
                                    }
                                    else
                                    {
                                        if (statePos.y > existingCurveNode->GetGridPos().y)
                                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / 350;
                                        else
                                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / 350;
                                    }
                                    newCurve = std::max(-1.0f, std::min(1.0f, newCurve));
                                    existingCurveNode->SetInArrowCurve(newCurve);
                                }
                            }
                            if (nodeEditor->IsSettingOutCurve())
                            {
                                auto newCurve = existingCurveNode->GetOutArrowCurve();
                                if (const auto fromState = trigger->GetNextState())
                                {
                                    const auto statePos = fromState->GetNode()->GetGridPos();
                                    if ( statePos.y < existingCurveNode->GetGridPos().y)
                                    {
                                        if (statePos.x < existingCurveNode->GetGridPos().x)
                                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / 350;
                                        else
                                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / 350;
                                    }
                                    else
                                    {
                                        if (statePos.x > existingCurveNode->GetGridPos().x)
                                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / 350;
                                        else
                                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / 350;
                                    }
                                    if (statePos.x > existingCurveNode->GetGridPos().x)
                                    {
                                        if (statePos.y > existingCurveNode->GetGridPos().y)
                                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / 350;
                                        else
                                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / 350;
                                    }
                                    else
                                    {
                                        if (statePos.y < existingCurveNode->GetGridPos().y)
                                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / 350;
                                        else
                                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / 350;
                                    }
                                    newCurve = std::max(-1.0f, std::min(1.0f, newCurve));
                                    existingCurveNode->SetOutArrowCurve(newCurve);
                                }
                            }
                            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
                        }
                    }
                }
                else
                {
                    nodeEditor->SetCurveNode(nullptr);
                    nodeEditor->SetSettingInCurve(false);
                    nodeEditor->SetSettingOutCurve(false);
                }
            }
            else
            {
                nodeEditor->SetCurveNode(nullptr);
                nodeEditor->SetSettingInCurve(false);
                nodeEditor->SetSettingOutCurve(false);
            }
            if (ImGui::IsKeyPressed(ImGuiKey_F4))
            {
                if (!nodeEditor->GetCurrentFsm()->GetLinkedFile().empty())
                    nodeEditor->GetCurrentFsm()->UpdateFromFile(nodeEditor->GetCurrentFsm()->GetLinkedFile());
            }
            if (ImGui::IsKeyPressed(ImGuiKey_F5))
            {
                if (!nodeEditor->GetCurrentFsm()->GetLinkedFile().empty())
                    nodeEditor->GetCurrentFsm()->UpdateToFile();
            }
            nodeEditor->SetCanvasPos(Math::AddVec2(ImGui::GetWindowPos(), ImGui::GetWindowContentRegionMin()));
            nodeEditor->SetCanvasSize(ImGui::GetWindowSize());
            CANVAS_POS = ImGui::GetWindowPos();
            CANVAS_SIZE = ImGui::GetWindowSize();
            CANVAS_CONTENT_MIN = ImGui::GetWindowContentRegionMin();
            CANVAS_CONTENT_MAX = ImGui::GetWindowContentRegionMax();
            ImGui::SetWindowFontScale(nodeEditor->GetScale());
            for (const auto& [key, state] : nodeEditor->GetCurrentFsm()->GetStates())
                state->DrawNode();
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)
                //&& (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) || ImGui::IsWindowFocused())
                && nodeEditor->IsDragging())
            {
                if (const auto selectedNode = nodeEditor->GetSelectedNode(); selectedNode)
                {
                    auto newPos = ImGui::GetMousePos() - selectedNode->GetSize() / 2;
                    newPos.x = std::max(CANVAS_POS.x, newPos.x);
                    newPos.y = std::max(CANVAS_POS.y, newPos.y);
                    newPos.x = std::min(CANVAS_POS.x + CANVAS_SIZE.x - selectedNode->GetSize().x, newPos.x);
                    newPos.y = std::min(CANVAS_POS.y + CANVAS_SIZE.y - selectedNode->GetSize().y, newPos.y);
                    newPos = (newPos - CANVAS_POS + ImVec2{ImGui::GetScrollX(), ImGui::GetScrollY()}) / nodeEditor->GetScale();
                    selectedNode->SetGridPos(newPos);
                }
            }
            else
            {
                nodeEditor->SetDragging(false);
            }
            for (const auto& [key, trigger] : nodeEditor->GetCurrentFsm()->GetTriggers())
            {
                trigger->DrawNode();
                if (trigger->GetCurrentState())
                {
                    if (const auto currentState = trigger->GetCurrentState()->GetNode())
                    {
                        NodeEditor::DrawConnection(currentState, trigger->GetNode());
                    }
                }
                if (trigger->GetNextState())
                {
                    if (const auto nextState = trigger->GetNextState()->GetNode())
                    {
                        NodeEditor::DrawConnection(trigger->GetNode(), nextState);
                    }
                }
            }
            if (nodeEditor->GetCopiedNode() && ImGui::IsKeyPressed(ImGuiKey_V) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
            {
                CURSOR_POS = ImGui::GetMousePos() - CANVAS_POS + ImVec2{ImGui::GetScrollX(), ImGui::GetScrollY()} / nodeEditor->GetScale();
                switch (nodeEditor->GetCopiedNode()->GetType())
                {
                    case NodeType::State:
                    {
                        PASTE_NEW_STATE_AT_CURSOR = true;
                        break;
                    }
                    case NodeType::Transition:
                    {
                        PASTE_NEW_TRIGGER_AT_CURSOR = true;
                        break;
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
                    CURSOR_POS = ImGui::GetMousePos() - CANVAS_POS + ImVec2{ImGui::GetScrollX(), ImGui::GetScrollY()} / nodeEditor->GetScale();
                    switch (otherNode->GetType())
                    {
                        case NodeType::State:
                        {
                                ADD_NEW_TRIGGER_AT_CURSOR = true;
                                break;
                        }
                        case NodeType::Transition:
                        {
                                ADD_NEW_STATE_AT_CURSOR = true;
                            break;
                        }
                    }
                }
                NodeEditor::Get()->SetCreatingLink(false);
            }
            else if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
                NodeEditor::Get()->SetCreatingLink(false);
            ImGui::PopStyleVar();
            
            ImGui::SetWindowFontScale(1.0f);
            
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
            
            if (PASTE_NEW_STATE_AT_CURSOR)
            {
                ImGui::OpenPopup("Paste State At Cursor");
                PASTE_NEW_STATE_AT_CURSOR = false;
            }

            if (PASTE_NEW_TRIGGER_AT_CURSOR)
            {
                ImGui::OpenPopup("Paste Trigger At Cursor");
                PASTE_NEW_TRIGGER_AT_CURSOR = false;
            }

            if (ImGui::BeginPopup("Add State At Cursor"))
            {
                if (const auto fromNode = NodeEditor::Get()->GetSelectedNode(); fromNode && fromNode->GetType() == NodeType::Transition)
                {
                    static std::string id;
                    ImGui::InputText("New State ID", &id);
                    ImGui::SetItemTooltip("Used for internal identification and must be unique. Changing this requires refactor.");
                    static std::string name;
                    ImGui::InputText("New State Name", &name);
                    if (const auto invalid = std::regex_search(id, FsmRegex::InvalidIdRegex());
                        !invalid &&
                        !id.empty() && !name.empty()
                        && !NodeEditor::Get()->GetCurrentFsm()->GetTrigger(id)
                        && !NodeEditor::Get()->GetCurrentFsm()->GetState(id) && ImGui::Button("Add State"))
                    {
                        const auto state = NodeEditor::Get()->GetCurrentFsm()->AddState(id);
                        state->GetNode()->SetGridPos(CURSOR_POS);
                        state->SetName(name);
                        const auto trigger = NodeEditor::Get()->GetCurrentFsm()->GetTrigger(fromNode->GetId());
                        trigger->SetNextState(state->GetId());
                        ImGui::SetClipboardText(state->GetLuaCode().c_str());
                        state->AppendToFile();
                        ImGui::InsertNotification({ImGuiToastType::Success, 3000, "State added: %s, code copied to clipboard", id.c_str()});
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
                    else if (invalid)
                    {
                        ImGui::Text("Invalid State ID");
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel"))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("Paste State At Cursor"))
            {
                if (const auto copiedNode = NodeEditor::Get()->GetCopiedNode();
                    copiedNode && copiedNode->GetType() == NodeType::State
                    && NodeEditor::Get()->GetCurrentFsm()->GetState(copiedNode->GetId())
                    )
                {
                    static std::string id;
                    ImGui::InputText("New State ID", &id);
                    ImGui::SetItemTooltip("Used for internal identification and must be unique. Changing this requires refactor.");
                    if (const auto invalid = std::regex_search(id, FsmRegex::InvalidIdRegex());
                        !invalid &&!id.empty()
                        && !NodeEditor::Get()->GetCurrentFsm()->GetTrigger(id)
                        && !NodeEditor::Get()->GetCurrentFsm()->GetState(id)
                        && ImGui::Button("Add State"))
                    {
                        const auto originalState = NodeEditor::Get()->GetCurrentFsm()->GetState(copiedNode->GetId());
                        const auto state = NodeEditor::Get()->GetCurrentFsm()->AddState(id);
                        state->SetName(originalState->GetName());
                        state->SetOnEnter(originalState->GetOnEnter());
                        state->SetOnUpdate(originalState->GetOnUpdate());
                        state->SetOnExit(originalState->GetOnExit());
                        state->SetDescription(originalState->GetDescription());
                        state->GetNode()->SetGridPos(CURSOR_POS);
                        ImGui::SetClipboardText(state->GetLuaCode().c_str());
                        state->AppendToFile();
                        ImGui::InsertNotification({ImGuiToastType::Success, 3000, "State added: %s, code copied to clipboard", id.c_str()});
                        id = "";
                        ImGui::CloseCurrentPopup();
                    }
                    else if (id.empty())
                    {
                        ImGui::Text("ID can not be empty.");
                    }
                    else if (NodeEditor::Get()->GetCurrentFsm()->GetTrigger(id) || NodeEditor::Get()->GetCurrentFsm()->GetState(id))
                    {
                        ImGui::Text("State with ID already exists.");
                    }
                    else if (invalid)
                    {
                        ImGui::Text("Invalid State ID");
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel"))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::EndPopup();
            }

            if (ImGui::BeginPopup("Paste Trigger At Cursor"))
            {
                if (const auto copiedNode = NodeEditor::Get()->GetCopiedNode();
                    copiedNode && copiedNode->GetType() == NodeType::Transition
                    && NodeEditor::Get()->GetCurrentFsm()->GetTrigger(copiedNode->GetId())
                    )
                {
                    static std::string id;
                    ImGui::InputText("New Trigger ID", &id);
                    ImGui::SetItemTooltip("Used for internal identification and must be unique. Changing this requires refactor.");
                    if (const auto invalid = std::regex_search(id, FsmRegex::InvalidIdRegex());
                        !invalid && !id.empty()
                        && !NodeEditor::Get()->GetCurrentFsm()->GetTrigger(id)
                        && !NodeEditor::Get()->GetCurrentFsm()->GetState(id)
                        && ImGui::Button("Add Condition"))
                    {
                        const auto originalCondition = NodeEditor::Get()->GetCurrentFsm()->GetTrigger(copiedNode->GetId());
                        const auto condition = std::make_shared<FsmTrigger>(id);
                        NodeEditor::Get()->GetCurrentFsm()->AddTrigger(condition);
                        condition->SetName(originalCondition->GetName());
                        condition->SetDescription(originalCondition->GetDescription());
                        condition->SetCondition(originalCondition->GetCondition());
                        condition->SetAction(originalCondition->GetAction());
                        condition->GetNode()->SetGridPos(CURSOR_POS);
                        ImGui::SetClipboardText(condition->GetLuaCode().c_str());
                        condition->AppendToFile();
                        ImGui::InsertNotification({ImGuiToastType::Success, 3000, "State added: %s, code copied to clipboard", id.c_str()});
                        id = "";
                        ImGui::CloseCurrentPopup();
                    }
                    else if (id.empty())
                    {
                        ImGui::Text("ID can not be empty.");
                    }
                    else if (NodeEditor::Get()->GetCurrentFsm()->GetTrigger(id) || NodeEditor::Get()->GetCurrentFsm()->GetState(id))
                    {
                        ImGui::Text("Condition with ID already exists.");
                    }
                    else if (invalid)
                    {
                        ImGui::Text("Invalid Condition ID");
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
                    ImGui::InputText("New Trigger ID", &id);
                    ImGui::SetItemTooltip("Used for internal identification and must be unique. Be careful with choosing if you change it later you will need to refactor your lua code to maintain loading ability.");
                    static std::string name;
                    ImGui::InputText("New Trigger Name", &name);
                    if (const auto invalid = std::regex_search(id, FsmRegex::InvalidIdRegex());
                        !invalid &&
                        !id.empty() && !name.empty()
                        && !NodeEditor::Get()->GetCurrentFsm()->GetTrigger(id)
                        && !NodeEditor::Get()->GetCurrentFsm()->GetState(id)
                        && ImGui::Button("Add Trigger"))
                    {
                        const auto state = NodeEditor::Get()->GetCurrentFsm()->GetState(fromNode->GetId());
                        const auto trigger = state->AddTrigger(id);
                        nodeEditor->GetCurrentFsm()->AddTrigger(trigger);
                        trigger->GetNode()->SetGridPos(CURSOR_POS);
                        ImGui::SetClipboardText(trigger->GetLuaCode().c_str());
                        ImGui::InsertNotification({ImGuiToastType::Success, 3000, "Condition added: %s, code copied to clipboard", id.c_str()});
                        trigger->SetName(name);
                        trigger->AppendToFile();
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
                    else if (invalid)
                    {
                        ImGui::Text("Invalid Trigger ID");
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
    }

    void Window::Properties()
    {
        const auto nodeEditor = NodeEditor::Get();

        // Code editor window
        ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoMove);
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
    
    void Window::TrimTrailingNewlines(std::string& str)
    {
        while (!str.empty() && (str.back() == '\n' || str.back() == '\r'))
        {
            str.pop_back();
        }
    }


    
}
