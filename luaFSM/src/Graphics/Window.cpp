#include "pch.h"
#include "Window.h"
#include "stb_image.h"
#include "imgui/ImGuiImpl.h"
#include "imgui.h"
#include "imgui/imgui_stdlib.h"
#include "Log.h"
#include "luaFsm.h"
#include "data/FSM.h"
#include "imgui/TextEditor.h"
#include "imgui/NodeEditor.h"
#include "IO/FileReader.h"
#include "imgui/ImGuiNotify.hpp"
#include "imgui/IconsFontAwesome6.h"
#include "imgui/popups/Popup.h"

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
        ImFont* MAIN_EDITOR_FONT = nullptr;
    }

    //static data initializers
    TextEditor::Palette Window::m_Palette = {};
    std::unordered_map<std::string, ImFont*> Window::m_Fonts = {};
    std::unordered_map<std::string, Window::ImGuiColorTheme> Window::m_Themes = {};
    std::string Window::m_ActiveTheme;
    PopupManager Window::m_PopupManager{};

    bool MAIN_DOCK_OPENED = true;
    bool DOCK_SPACE_SET = false;

    bool HELP_WINDOW_OPEN = false;
    
    ImVec2 CURSOR_POS = {0, 0};
    bool ADD_NEW_STATE_AT_CURSOR = false;
    bool ADD_NEW_TRIGGER_AT_CURSOR = false;
    bool PASTE_NEW_STATE_AT_CURSOR = false;
    bool PASTE_NEW_TRIGGER_AT_CURSOR = false;
    bool PROPERTIES_FIRST_TIME = true;
    bool FIRST_OPEN = true;
    
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

        if (FileReader::FileExists("luaFSM.png"))
        {
            int width, height, channels;
            const auto img = stbi_load("luaFSM.png", &width, &height, &channels, 4);
            GLFWimage images[1];
            images[0].width = width;
            images[0].height = height;
            images[0].pixels = img;
            glfwSetWindowIcon(m_Window, 1, images);
        }
        
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
    
    IM_MSVC_RUNTIME_CHECKS_OFF
    void Window::InitImGui()
    {
        InitThemes();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
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

        //Add different font sizes
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
        
        io.FontDefault = MAIN_EDITOR_FONT = GetFont("Cascadia_18");
        NodeEditor::Get()->SetFont(GetFont("Cascadia_18"));

        /*-----------------------------------------*\
                  ImGuiNotify Icons Stuff 
        \*-----------------------------------------*/
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
        
        /*-----------------------------------------*\
                    Set Text Editor colors
        \*-----------------------------------------*/
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

        //Default theme
        SetTheme("Green");
        NodeEditor::Get()->LoadSettings();
        //Popups
        InitializePopups();
    }
    IM_MSVC_RUNTIME_CHECKS_RESTORE

    void Window::InitializePopups()
    {
        m_PopupManager.AddPopup(WindowPopups::AddFsmPopup, std::make_shared<AddFsmPopup>());
        m_PopupManager.AddPopup(WindowPopups::ThemeEditor, std::make_shared<ThemeEditor>());
        m_PopupManager.AddPopup(WindowPopups::LoadFile, std::make_shared<LoadFile>());
        m_PopupManager.AddPopup(WindowPopups::CreateFilePopup, std::make_shared<CreateFilePopup>());
        m_PopupManager.AddPopup(WindowPopups::OptionsPopup, std::make_shared<OptionsPopUp>());
        
        const auto addStateCursor = std::make_shared<AddStatePopup>("AddStateCursor");
        addStateCursor->isDrawn = true;
        m_PopupManager.AddPopup(WindowPopups::AddStateCursor, addStateCursor);
        
        const auto pasteState = std::make_shared<AddStatePopup>("PasteState");
        pasteState->isCopy = true;
        m_PopupManager.AddPopup(WindowPopups::PasteState, pasteState);
        
        const auto addTriggerCursor = std::make_shared<AddTriggerPopup>("AddTriggerCursor");
        addTriggerCursor->isDrawn = true;
        m_PopupManager.AddPopup(WindowPopups::AddTriggerCursor, addTriggerCursor);
        
        const auto pasteTrigger = std::make_shared<AddTriggerPopup>("PasteTrigger");
        pasteTrigger->isCopy = true;
        m_PopupManager.AddPopup(WindowPopups::PasteTrigger, pasteTrigger);
    }
    
    void Window::OnUpdate() const
    {
        glfwPollEvents();
        BeginImGui();
        OnImGuiRender();
        EndImGui();
    }
    
    void Window::BeginImGui()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }
    
    void Window::OnImGuiRender()
    {
        ImGui::PushFont(MAIN_EDITOR_FONT); //Main editor font
        RenderNotifications();
        bool check = false;
        if (NodeEditor::Get()->GetCurrentFsm())
        {
            NodeEditor::Get()->GetCurrentFsm()->LastState();
            check = true;
        }
        MainMenu();
        MainDockSpace();
        Canvas();
        Properties();
        HelpWindow();
        if (NodeEditor::Get()->GetCurrentFsm() && check)
            NodeEditor::Get()->GetCurrentFsm()->CheckForChanges();
        ImGui::PopFont(); //Main editor font
    }
    
    void Window::EndImGui() const
    {
        //actual window background color
        constexpr auto clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        const ImGuiIO& io = ImGui::GetIO();
        (void)io;
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
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.10f, 1.00f)); // Notification Background color
        ImGui::PushFont(m_Fonts["notificationFont"]); // Notification Font
        ImGui::RenderNotifications();
        ImGui::PopStyleColor(); // Notification Background color
        ImGui::PopFont(); // Notification Font
    }

    ImVec4 INVISIBLE_COLOR = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    ImVec2 MENU_BAR_SIZE = {0, 0};

    void Window::MainMenu()
    {
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        const auto popupManager = GetPopupManager();
        if (ImGui::BeginMainMenuBar())
        {
            MENU_BAR_SIZE = ImGui::GetWindowSize();
            /*-----------------------------*\
                        File menu
            \*-----------------------------*/
            if (ImGui::BeginMenu("File"))
            {
                //New FSM
                if (ImGui::MenuItem("New FSM"))
                {
                    popupManager->OpenPopup(WindowPopups::AddFsmPopup);
                    PROPERTIES_FIRST_TIME = true;
                }

                //Load FSM from Lua file
                if (ImGui::MenuItem("Load"))
                    popupManager->OpenPopup(WindowPopups::LoadFile);

                if (fsm)
                {
                    const std::string linkedFile = fsm->GetLinkedFile();
                    //Create Lua File
                    if (linkedFile.empty() && ImGui::MenuItem("Create Lua File"))
                        popupManager->OpenPopup(WindowPopups::CreateFilePopup);
                    //Save to Lua File
                    if (!linkedFile.empty() && ImGui::MenuItem("Save"))
                        fsm->UpdateToFile(fsm->GetId());
                }
                ImGui::Separator();
                //Exit
                if (ImGui::MenuItem("Exit"))
                    SHUTDOWN = true;
                ImGui::EndMenu();
            }
            
            /*-----------------------------*\
                       Theme menu
            \*-----------------------------*/
            //Select theme
            if (ImGui::BeginMenu("Themes"))
            {
                for (const auto& name : m_Themes | std::views::keys)
                {
                    if (ImGui::MenuItem(name.c_str()))
                    {
                        SetTheme(name);
                        NodeEditor::Get()->SaveSettings();
                    }
                }
                ImGui::EndMenu();
            }
            //Edit theme
            ImGui::PushStyleColor(ImGuiCol_Button, INVISIBLE_COLOR);
            if (ImGui::Button("Edit Theme"))
                popupManager->OpenPopup(WindowPopups::ThemeEditor);
            ImGui::PopStyleColor(); //INVISIBLE_COLOR
            
            /*-----------------------------*\
                    Options and help
            \*-----------------------------*/
            
            ImGui::PushStyleColor(ImGuiCol_Button, INVISIBLE_COLOR);
            if (ImGui::Button("Options"))
                popupManager->OpenPopup(WindowPopups::OptionsPopup);
            ImGui::PopStyleColor(); //INVISIBLE_COLOR
            
            ImGui::PushStyleColor(ImGuiCol_Button, INVISIBLE_COLOR);
            if (ImGui::Button("Help"))
                HELP_WINDOW_OPEN = !HELP_WINDOW_OPEN;
            ImGui::PopStyleColor(); //INVISIBLE_COLOR
            
            
            /*-----------------------------*\
                      FSM Properties
            \*-----------------------------*/
            if (fsm)
            {
                //Show FSM properties
                ImGui::PushStyleColor(ImGuiCol_Button, INVISIBLE_COLOR);
                if (ImGui::Button("FSM Properties"))
                    NodeEditor::Get()->DeselectAllNodes(); //No nodes shows FSM properties
                ImGui::PopStyleColor(); //INVISIBLE_COLOR

                //FSM name
                ImGui::Text("FSM: ");
                ImGui::Text(fsm->GetName().c_str());
            }
            ImGui::EndMainMenuBar();
        }
        
        //Open popups
        popupManager->ShowOpenPopups(); 
    }
    
    uint32_t MAIN_DOCK_FLAGS =
        ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoResize;

    void Window::MainDockSpace()
    {
        const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        // Set window position and size
        ImGui::SetNextWindowPos(ImVec2(mainViewport->WorkPos.x, mainViewport->WorkPos.y + MENU_BAR_SIZE.y), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(mainViewport->WorkSize.x, mainViewport->WorkSize.y - MENU_BAR_SIZE.y), ImGuiCond_Once);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        // Main window encompassing all elements
        ImGui::Begin("Node Editor", &MAIN_DOCK_OPENED, MAIN_DOCK_FLAGS);
        {
            ImGui::PopStyleVar(); //ImGuiStyleVar_WindowPadding
            
            // Dock space setup
            ImGuiID dockspaceId = ImGui::GetID("NodeEditorDockspace");
            ImGui::DockSpace(dockspaceId);
            if (!DOCK_SPACE_SET)
            {
                ImGui::DockBuilderRemoveNode(dockspaceId); // Clear out existing layout
                ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_None);
                ImGui::DockBuilderSetNodeSize(dockspaceId, mainViewport->Size);

                const ImGuiID dockIdLeft = ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.75f, nullptr, &dockspaceId);
                const ImGuiID dockIdRight = dockspaceId;

                ImGui::DockBuilderDockWindow(NodeEditor::Get()->canvasName.c_str(), dockIdLeft);
                ImGui::DockBuilderDockWindow("Properties", dockIdRight);

                ImGui::DockBuilderFinish(dockspaceId);
                DOCK_SPACE_SET = true;
            }
            ImGui::End();
        }
    }

    
    ImVec2 CANVAS_POS{};
    ImVec2 CANVAS_SIZE{};
    float MOUSE_SCROLL = 0;
    bool CANVAS_OPEN = true;
    bool ADD_ELEMENTS_MENU = false;
    uint32_t CANVAS_FLAGS =
        ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoScrollWithMouse;

    ImVec2 CANVAS_FULL_SIZE = {4096, 4096};
    
    // Canvas window
    void Window::Canvas()
    {
        const auto nodeEditor = NodeEditor::Get();
        const auto fsm = nodeEditor->GetCurrentFsm();
        if (!fsm)
            return;

        if (fsm->IsUnSavedGlobal())
            CANVAS_FLAGS |= ImGuiWindowFlags_UnsavedDocument;
        else
            CANVAS_FLAGS &= ~ImGuiWindowFlags_UnsavedDocument;
        
        const auto popupManager = GetPopupManager();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        
        //Size of the canvas
        ImGui::SetNextWindowContentSize(CANVAS_FULL_SIZE);
        ImGui::PushFont(nodeEditor->GetFont()); //Node font
        ImGui::Begin(nodeEditor->canvasName.c_str(), nullptr, CANVAS_FLAGS);
        {
            
            ImGui::PopStyleVar(); //ImGuiStyleVar_WindowPadding
            
            //Handle initial canvas position
            if (FIRST_OPEN)
            {
                //Center canvas on nodes when loading
                if (fsm->GetInitialState())
                    nodeEditor->MoveToNode(fsm->GetInitialStateId(), NodeType::State);
                else
                {
                    ImGui::SetScrollX(1024);
                    ImGui::SetScrollY(1024);
                }
                FIRST_OPEN = false;
            }
            
            //Handle key binds
            CanvasKeyBindManager();
            
            CANVAS_POS = ImGui::GetWindowPos();
            CANVAS_SIZE = ImGui::GetWindowSize();
            nodeEditor->SetCanvasPos(CANVAS_POS);
            nodeEditor->SetCanvasSize(CANVAS_SIZE);
            /*-------------------------------------------------------------------------------------------------------*\
                                                           Draw Nodes
            \*-------------------------------------------------------------------------------------------------------*/
            ImGui::SetWindowFontScale(nodeEditor->GetScale()); //Node size is directly related to font scale
            
            //Draw states
            for (const auto& [key, state] : fsm->GetStates())
                state->DrawNode();
            
            //Draw transitions
            for (const auto& [key, trigger] : fsm->GetTriggers())
            {
                trigger->DrawNode();
                //Draw connections
                if (trigger->GetCurrentState())
                {
                    if (const auto currentState = trigger->GetCurrentState()->GetNode())
                        NodeEditor::DrawConnection(currentState, trigger->GetNode());
                }
                if (trigger->GetNextState())
                {
                    if (const auto nextState = trigger->GetNextState()->GetNode())
                        NodeEditor::DrawConnection(trigger->GetNode(), nextState);
                }
            }

            //Drag nodes
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && nodeEditor->IsDragging())
            {
                if (const auto selectedNode = nodeEditor->GetSelectedNode(); selectedNode)
                {
                    auto newPos = ImGui::GetMousePos() - selectedNode->GetSize() / 2;
                    newPos.x = std::max(CANVAS_POS.x, newPos.x);
                    newPos.y = std::max(CANVAS_POS.y, newPos.y);
                    newPos.x = std::min(CANVAS_POS.x + CANVAS_SIZE.x - selectedNode->GetSize().x, newPos.x);
                    newPos.y = std::min(CANVAS_POS.y + CANVAS_SIZE.y - selectedNode->GetSize().y, newPos.y);
                    newPos = (newPos - CANVAS_POS + ImVec2{ImGui::GetScrollX(), ImGui::GetScrollY()}) / nodeEditor->GetScale();
                    selectedNode->SetGridPos(newPos - nodeEditor->GetDragOffset());
                }
            }
            else
                nodeEditor->SetDragging(false);

            //Paste node
            if (nodeEditor->GetCopiedNode() && ImGui::IsKeyPressed(ImGuiKey_V) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
            {
                CURSOR_POS = ImGui::GetMousePos() - CANVAS_POS + ImVec2{ImGui::GetScrollX(), ImGui::GetScrollY()} / nodeEditor->GetScale();
                switch (nodeEditor->GetCopiedNode()->GetType())
                {
                    case NodeType::State:
                    {
                        popupManager->GetPopup<AddStatePopup>(WindowPopups::PasteState)->position = CURSOR_POS;
                        popupManager->OpenPopup(WindowPopups::PasteState);
                        break;
                    }
                    case NodeType::Transition:
                    {
                        popupManager->GetPopup<AddTriggerPopup>(WindowPopups::PasteTrigger)->position = CURSOR_POS;
                        popupManager->OpenPopup(WindowPopups::PasteTrigger);
                        break;
                    }
                case NodeType::Fsm:
                    break;
                }
            }

            //Draw lines for creating links
            if (ImGui::IsWindowHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
            {
                if (const auto selectedNode = nodeEditor->GetSelectedNode();
                    selectedNode && ImGui::IsMouseDragging(ImGuiMouseButton_Right))
                {
                    if ((selectedNode->GetType() == NodeType::State
                        && nodeEditor->GetCurrentFsm()->GetState(selectedNode->GetId())
                        && !nodeEditor->GetCurrentFsm()->GetState(selectedNode->GetId())->IsExitState())
                        || selectedNode->GetType() == NodeType::Transition)
                    {
                        NodeEditor::DrawLine(selectedNode->GetFromPoint(ImGui::GetMousePos()), ImGui::GetMousePos());
                        NodeEditor::Get()->SetCreatingLink(true);
                    }
                }
                //Drawing finished
                if (NodeEditor::Get()->IsCreatingLink() && !ImGui::IsMouseDown(ImGuiMouseButton_Right)
                && !popupManager->GetPopup<Popup>(WindowPopups::AddStateCursor)->isOpen
                && !popupManager->GetPopup<Popup>(WindowPopups::AddTriggerCursor)->isOpen
                )
                {
                    if (const auto parentNode = nodeEditor->GetSelectedNode(); parentNode)
                    {
                        ADD_ELEMENTS_MENU = false;
                        nodeEditor->SetShowNodeContext(false);
                        CURSOR_POS = ImGui::GetMousePos() - CANVAS_POS + ImVec2{ImGui::GetScrollX(), ImGui::GetScrollY()} / nodeEditor->GetScale();
                        switch (parentNode->GetType())
                        {
                        case NodeType::State:
                            {
                                const auto popup = popupManager->GetPopup<AddTriggerPopup>(WindowPopups::AddTriggerCursor);
                                popup->position = CURSOR_POS;
                                popup->isDrawn = true;
                                popupManager->OpenPopup(WindowPopups::AddTriggerCursor);
                                break;
                            }
                        case NodeType::Transition:
                            {
                                const auto popup = popupManager->GetPopup<AddStatePopup>(WindowPopups::AddStateCursor);
                                popup->position = CURSOR_POS;
                                popup->isDrawn = true;
                                popupManager->OpenPopup(WindowPopups::AddStateCursor);
                                break;
                            }
                        case NodeType::Fsm:
                            break;
                        }
                    }
                }
                else if (!ImGui::IsMouseDragging(ImGuiMouseButton_Right) && !nodeEditor->IsCreatingLink() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)
                && !popupManager->GetPopup<Popup>(WindowPopups::AddStateCursor)->isOpen
                && !nodeEditor->ShowNodeContext()
                && !popupManager->GetPopup<Popup>(WindowPopups::AddTriggerCursor)->isOpen)
                {
                    CURSOR_POS = ImGui::GetMousePos() - CANVAS_POS + ImVec2{ImGui::GetScrollX(), ImGui::GetScrollY()} / nodeEditor->GetScale();
                    ADD_ELEMENTS_MENU = true;
                }
            }
            
            if (nodeEditor->GetSelectedNode() && nodeEditor->ShowNodeContext())
            {
                const std::string label = "##NodeContext" + nodeEditor->GetSelectedNode()->GetId();
                ImGui::OpenPopup(label.c_str());
                if (ImGui::BeginPopup(label.c_str()))
                {
                    ImGui::Selectable("Copy");
                    if (ImGui::IsItemClicked())
                    {
                        nodeEditor->CopyNode(nodeEditor->GetSelectedNode());
                        nodeEditor->SetShowNodeContext(false);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::Selectable("Delete");
                    if (ImGui::IsItemClicked())
                    {
                        if (nodeEditor->GetSelectedNode()->GetType() == NodeType::State)
                        {
                            nodeEditor->GetCurrentFsm()->RemoveState(nodeEditor->GetSelectedNode()->GetId());
                            nodeEditor->SetShowNodeContext(false);
                        }
                        else if (nodeEditor->GetSelectedNode()->GetType() == NodeType::Transition)
                        {
                            nodeEditor->GetCurrentFsm()->RemoveTrigger(nodeEditor->GetSelectedNode()->GetId());
                            nodeEditor->SetShowNodeContext(false);
                        }
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
            
            if (ADD_ELEMENTS_MENU
                && !popupManager->GetPopup<Popup>(WindowPopups::AddStateCursor)->isOpen
                && !popupManager->GetPopup<Popup>(WindowPopups::AddTriggerCursor)->isOpen
                )
            {
                bool addState = false;
                bool addTrigger = false;
                bool pasteState = false;
                bool pasteTrigger = false;
                if (ImGui::BeginPopupContextWindow())
                {
                    if (ImGui::MenuItem("Add State"))
                    {
                        const auto popup = popupManager->GetPopup<AddStatePopup>(WindowPopups::AddStateCursor);
                        popup->position = CURSOR_POS;
                        popup->isDrawn = false;
                        ADD_ELEMENTS_MENU = false;
                        addState = true;
                    }
                    if (ImGui::MenuItem("Add Trigger"))
                    {
                        const auto popup = popupManager->GetPopup<AddTriggerPopup>(WindowPopups::AddTriggerCursor);
                        popup->position = CURSOR_POS;
                        popup->isDrawn = false;
                        ADD_ELEMENTS_MENU = false;
                        addTrigger = true;
                    }
                    if (nodeEditor->GetCopiedNode() && ImGui::MenuItem("Paste"))
                    {
                        switch (nodeEditor->GetCopiedNode()->GetType())
                        {
                        case NodeType::State:
                            {
                                popupManager->GetPopup<AddStatePopup>(WindowPopups::PasteState)->position = CURSOR_POS;
                                pasteState = true;
                                break;
                            }
                        case NodeType::Transition:
                            {
                                popupManager->GetPopup<AddTriggerPopup>(WindowPopups::PasteTrigger)->position = CURSOR_POS;
                                pasteTrigger = true;
                                break;
                            }
                        case NodeType::Fsm:
                            break;
                        }
                    }
                    ImGui::EndPopup();
                }
                if (addState)
                    popupManager->OpenPopup(WindowPopups::AddStateCursor);
                if (addTrigger)
                    popupManager->OpenPopup(WindowPopups::AddTriggerCursor);
                if (pasteState)
                    popupManager->OpenPopup(WindowPopups::PasteState);
                if (pasteTrigger)
                    popupManager->OpenPopup(WindowPopups::PasteTrigger);
            }
            
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
                nodeEditor->SetCreatingLink(false);
            
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                nodeEditor->SetShowNodeContext(false);
            
            ImGui::SetWindowFontScale(1.0f);
            
            GetPopupManager()->ShowOpenPopups();
            
            ImGui::End();
        }
        ImGui::PopFont(); //Node font
    }

    void Window::CanvasKeyBindManager()
    {
        const auto nodeEditor = NodeEditor::Get();
        if (!ImGui::IsWindowHovered() && !ImGui::IsWindowFocused())
        {
            nodeEditor->SetCurveNode(nullptr);
            nodeEditor->SetSettingInCurve(false);
            nodeEditor->SetSettingOutCurve(false);
            MOUSE_SCROLL = 0;
            return;
        }
        
        //Save Ctrl + S
        if (ImGui::IsKeyPressed(ImGuiKey_S) && ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
        {
            if (!nodeEditor->GetCurrentFsm()->GetLinkedFile().empty())
                nodeEditor->GetCurrentFsm()->UpdateToFile(NodeEditor::Get()->GetCurrentFsm()->GetId());
        }

        //Zoom Ctrl + Mouse Scroll
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
        {
            MOUSE_SCROLL = ImGui::GetIO().MouseWheel * 0.025f;
            nodeEditor->SetScale(std::min(1.5f, std::max(nodeEditor->GetScale() + MOUSE_SCROLL, 0.5f)));
        }
        else
            MOUSE_SCROLL = 0;

        //Pan Middle Mouse Drag
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
        {
            nodeEditor->SetPanning(true);
            const ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
            ImGui::SetScrollX(ImGui::GetScrollX() - delta.x);
            ImGui::SetScrollY(ImGui::GetScrollY() - delta.y);
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
        }
        else
            nodeEditor->SetPanning(false);

        //Drag arrow curves
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            if (const auto existingCurveNode = nodeEditor->GetCurveNode(); !existingCurveNode)
            {
                const auto pressPos = ImGui::GetMousePos();
                float smallestDist = 40;
                VisualNode* curveNode = nullptr;
                bool isInLine = false;
                for (const auto& [key, condition] : nodeEditor->GetCurrentFsm()->GetTriggers())
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
                DragArrows(existingCurveNode);
        }
        else //mouse left not down
        {
            nodeEditor->SetCurveNode(nullptr);
            nodeEditor->SetSettingInCurve(false);
            nodeEditor->SetSettingOutCurve(false);
        }
        
        //Load F4
        if (ImGui::IsKeyPressed(ImGuiKey_F4))
        {
            if (!nodeEditor->GetCurrentFsm()->GetLinkedFile().empty())
                nodeEditor->GetCurrentFsm()->UpdateFromFile(nodeEditor->GetCurrentFsm()->GetLinkedFile());
        }
        
        //Save F5
        if (ImGui::IsKeyPressed(ImGuiKey_F5))
        {
            if (!nodeEditor->GetCurrentFsm()->GetLinkedFile().empty())
                nodeEditor->GetCurrentFsm()->UpdateToFile(NodeEditor::Get()->GetCurrentFsm()->GetId());
        }

        //View FSM properties
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            nodeEditor->DeselectAllNodes();
    }

    void Window::DragArrows(VisualNode* existingCurveNode)
    {
        const auto nodeEditor = NodeEditor::Get();
        //Literally trial and error no clue why this works
        if (const auto trigger = nodeEditor->GetCurrentFsm()->GetTrigger(existingCurveNode->GetId()))
        {
            constexpr int dragSlowDown = 350;
            if (nodeEditor->IsSettingInCurve())
            {
                auto newCurve = existingCurveNode->GetInArrowCurve();
                if (const auto toState = trigger->GetCurrentState())
                {
                    const auto statePos = toState->GetNode()->GetGridPos();
                    if ( statePos.y < existingCurveNode->GetGridPos().y)
                    {
                        if (statePos.x > existingCurveNode->GetGridPos().x)
                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / dragSlowDown;
                        else
                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / dragSlowDown;
                    }
                    else
                    {
                        if (statePos.x < existingCurveNode->GetGridPos().x)
                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / dragSlowDown;
                        else
                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / dragSlowDown;
                    }
                    if (statePos.x > existingCurveNode->GetGridPos().x)
                    {
                        if (statePos.y < existingCurveNode->GetGridPos().y)
                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / dragSlowDown;
                        else
                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / dragSlowDown;
                    }
                    else
                    {
                        if (statePos.y > existingCurveNode->GetGridPos().y)
                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / dragSlowDown;
                        else
                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / dragSlowDown;
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
                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / dragSlowDown;
                        else
                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / dragSlowDown;
                    }
                    else
                    {
                        if (statePos.x > existingCurveNode->GetGridPos().x)
                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / dragSlowDown;
                        else
                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).y / dragSlowDown;
                    }
                    if (statePos.x > existingCurveNode->GetGridPos().x)
                    {
                        if (statePos.y > existingCurveNode->GetGridPos().y)
                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / dragSlowDown;
                        else
                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / dragSlowDown;
                    }
                    else
                    {
                        if (statePos.y < existingCurveNode->GetGridPos().y)
                            newCurve += ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / dragSlowDown;
                        else
                            newCurve -= ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x / dragSlowDown;
                    }
                    newCurve = std::max(-1.0f, std::min(1.0f, newCurve));
                    existingCurveNode->SetOutArrowCurve(newCurve);
                }
            }
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }
    }

    bool PROPERTIES_OPEN = true;
    uint32_t PROPERTIES_FLAGS =
        ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_MenuBar
        | ImGuiWindowFlags_NoCollapse;

    //Properties on the right side of the screen
    void Window::Properties()
    {
        const auto nodeEditor = NodeEditor::Get();
        const auto fsm = nodeEditor->GetCurrentFsm();
        if (!fsm)
            return;
        const auto selectedNode = nodeEditor->GetSelectedNode();
        const auto nodeType = selectedNode ? selectedNode->GetType() : NodeType::Fsm;
        if (nodeType == NodeType::State)
        {
            if (const auto state = fsm->GetState(selectedNode->GetId());
                state->IsUnSaved())
                PROPERTIES_FLAGS |= ImGuiWindowFlags_UnsavedDocument;
            else
                PROPERTIES_FLAGS &= ~ImGuiWindowFlags_UnsavedDocument;
        }
        else if (nodeType == NodeType::Transition)
        {
            if (const auto trigger = fsm->GetTrigger(selectedNode->GetId());
                trigger->IsUnSaved())
                PROPERTIES_FLAGS |= ImGuiWindowFlags_UnsavedDocument;
            else
                PROPERTIES_FLAGS &= ~ImGuiWindowFlags_UnsavedDocument;
        }
        else
        {
            if (fsm->IsUnSaved())
                PROPERTIES_FLAGS |= ImGuiWindowFlags_UnsavedDocument;
            else
                PROPERTIES_FLAGS &= ~ImGuiWindowFlags_UnsavedDocument;
        }
        ImGui::Begin("Properties", nullptr, PROPERTIES_FLAGS);
        {

            if (PROPERTIES_FIRST_TIME && fsm->GetLinkedFile().empty())
            {
                GetPopupManager()->OpenPopup(static_cast<int>(WindowPopups::CreateFilePopup));
                PROPERTIES_FIRST_TIME = false;
            }
            
            //show fsm properties
            if (nodeEditor->ShowFsmProps())
                fsm->DrawProperties();
            //show selected node properties
            else if (selectedNode)
            {
                if (nodeType == NodeType::State)
                {
                    if (const auto state = fsm->GetState(selectedNode->GetId()))
                        state->DrawProperties();
                }
                else if (nodeType == NodeType::Transition)
                {
                    if (const auto trigger = fsm->GetTrigger(selectedNode->GetId()))
                        trigger->DrawProperties();
                }
            }
            else //if no selected nodes show fsm properties
                nodeEditor->SetShowFsmProps(true);
            
            ImGui::End();
        }
    }

    void Window::HelpWindow()
    {
        if (!HELP_WINDOW_OPEN)
            return;
        const auto viewPort = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewPort->Pos + ImVec2(viewPort->Size.x * 0.2f, viewPort->Size.y * 0.2f), ImGuiCond_Once);
        ImGui::SetNextWindowSize({viewPort->Size.x * 0.6f, viewPort->Size.y * 0.6f}, ImGuiCond_Once);
        ImGui::Begin("Help", &HELP_WINDOW_OPEN,
            ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoCollapse);
        {
            ImGui::Text("Keybindings:");
            ImGui::Separator();
            ImGui::Text("F4 - Load from file");
            ImGui::Text("F5 - Save to file");
            ImGui::Separator();
            ImGui::Text("Canvas:");
            ImGui::Separator();
            ImGui::Text("Middle mouse (hold) - Pan Canvas");
            ImGui::Text("Ctrl + mouse scroll - Zoom Canvas");
            ImGui::Text("Ctrl + S - Save");
            ImGui::Text("Ctrl + C - Copy node");
            ImGui::Text("Ctrl + V - Paste node");
            ImGui::Text("Linking:");
            ImGui::Separator();
            ImGui::Text("Hold right mouse from node - Start link");
            ImGui::Text("Release on another node - Create connection");
            ImGui::Text("Release on empty canvas space - Create node");
            ImGui::Separator();
            ImGui::Text("Help:");
            ImGui::TextWrapped(R"(
A state machine works by having an initial state and a set of transitions that can be triggered. 

When a state gets first entered it executes the code inside the OnEnter block. 
You can also use the onUpdate block if your FSM has persistent states between events and does not start from 
the initial state each time you enter it. When it leaves the state onExit fires. 

Each transition has a condition that must be met for the transition to occur. If the condition is met, the code 
in the 'action' section of that condition gets executed. If there is a next state, the state machine will transition to 
that state. If there is not a next state or the next state is not an exit state and that path has been fully explored
it will evaluate the next condition until all conditions have been evaluated and none are met or it reaches an exit state. 

If it reaches an exit state the state machine will stop executing. 

The Lua code is included in FSM.lua. The FSM, states and conditions extend classes created in that file. These are quite
basic and can easily be modified for specific use cases. The program uses regex to update the file and read from it, you
can fill it with your own code otherwise as long as you keep the format the program can read for the few things that it 
reads and writes to the file. 

You declare the FSM, states and conditions with:
---@FSM (variable name of a class that extends FSM)
---@FSM_STATE (variable name of a class that extends FSM_STATE)
---@FSM_CONDITION (variable name of a class that extends FSM_CONDITION)
respectively.

If you remove those annotations from your table then the program will stop interacting with that table and will act
like those states/conditions have been deleted. To make the regex easier and more reliable key functions have to end with
---@endFunc on the same line as 'end'. Like below:

end---@endFunc

It reads the class fields that are important to the program in the variableName.field format. If it lacks those fields
it won't update them from the program. It doesn't add every function by default to not clutter the file, if you use 
a new inherited function and want your changes to sync then add it to your file manually. You can see blueprints of the 
format the program expects in the Lua Code sections of each properties page.
)");
            ImGui::End();
        }
    }

    void Window::DrawTextEditor(TextEditor& txtEditor, std::string& oldText)
    {
        txtEditor.SetPalette(m_Palette);
        if (strcmp(txtEditor.GetText().c_str(), oldText.c_str()) != 0)
            txtEditor.SetText(oldText);
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
                if (string.empty())
                    continue;
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
        colors[ImGuiCol_CheckMark]              = ImVec4(theme.standard.x * 2.f, theme.standard.y * 2.f, theme.standard.z * 2.f, theme.standard.w * 2.f);
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
    
    void Window::TrimTrailingNewlines(std::string& str)
    {
        while (!str.empty() && (str.back() == '\n' || str.back() == '\r'))
            str.pop_back();
    }


    
}
