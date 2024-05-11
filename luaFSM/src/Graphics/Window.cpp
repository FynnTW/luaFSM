#include "pch.h"
#include "Window.h"
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
    
    ImVec2 CURSOR_POS = {0, 0};
    bool ADD_NEW_STATE_AT_CURSOR = false;
    bool ADD_NEW_TRIGGER_AT_CURSOR = false;
    bool PASTE_NEW_STATE_AT_CURSOR = false;
    bool PASTE_NEW_TRIGGER_AT_CURSOR = false;
    bool PROPERTIES_FIRST_TIME = true;
    bool FIRST_OPEN = true;
    
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
        //Popups
        InitializePopups();
        
    }
    IM_MSVC_RUNTIME_CHECKS_RESTORE

    void Window::InitializePopups()
    {
        m_PopupManager.AddPopup(static_cast<int>(WindowPopups::AddFsmPopup), std::make_shared<AddFsmPopup>());
        m_PopupManager.AddPopup(static_cast<int>(WindowPopups::ThemeEditor), std::make_shared<ThemeEditor>());
        m_PopupManager.AddPopup(static_cast<int>(WindowPopups::LoadFile), std::make_shared<LoadFile>());
        m_PopupManager.AddPopup(static_cast<int>(WindowPopups::CreateFilePopup), std::make_shared<CreateFilePopup>());
        m_PopupManager.AddPopup(static_cast<int>(WindowPopups::AddStateMain), std::make_shared<AddStatePopup>("AddStateMain"));
        
        const auto addStateCursor = std::make_shared<AddStatePopup>("AddStateCursor");
        addStateCursor->isDrawn = true;
        m_PopupManager.AddPopup(static_cast<int>(WindowPopups::AddStateCursor), addStateCursor);
        
        const auto pasteState = std::make_shared<AddStatePopup>("PasteState");
        pasteState->isCopy = true;
        m_PopupManager.AddPopup(static_cast<int>(WindowPopups::PasteState), pasteState);
        
        const auto addTriggerCursor = std::make_shared<AddTriggerPopup>("AddTriggerCursor");
        addTriggerCursor->isDrawn = true;
        m_PopupManager.AddPopup(static_cast<int>(WindowPopups::AddTriggerCursor), addTriggerCursor);
        
        const auto pasteTrigger = std::make_shared<AddTriggerPopup>("PasteTrigger");
        pasteTrigger->isCopy = true;
        m_PopupManager.AddPopup(static_cast<int>(WindowPopups::PasteTrigger), pasteTrigger);
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
        MainMenu();
        MainDockSpace();
        Canvas();
        Properties();
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

    void Window::MainMenu()
    {
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        const auto popupManager = GetPopupManager();
        if (ImGui::BeginMainMenuBar())
        {
            /*-----------------------------*\
                        File menu
            \*-----------------------------*/
            if (ImGui::BeginMenu("File"))
            {
                //New FSM
                if (ImGui::MenuItem("New FSM"))
                {
                    popupManager->OpenPopup(static_cast<int>(WindowPopups::AddFsmPopup));
                    PROPERTIES_FIRST_TIME = true;
                    FIRST_OPEN = true;
                }

                //Load FSM from Lua file
                if (ImGui::MenuItem("Load"))
                {
                    std::string current;
                    if (fsm)
                        current = fsm->GetId();
                    popupManager->OpenPopup(static_cast<int>(WindowPopups::LoadFile));
                    if (!current.empty() && NodeEditor::Get()->GetCurrentFsm() && current != NodeEditor::Get()->GetCurrentFsm()->GetId())
                        FIRST_OPEN = true;
                }

                if (fsm)
                {
                    const std::string linkedFile = fsm->GetLinkedFile();
                    //Create Lua File
                    if (linkedFile.empty() && ImGui::MenuItem("Create Lua File"))
                        popupManager->OpenPopup(static_cast<int>(WindowPopups::CreateFilePopup));
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
                    if (ImGui::MenuItem(name.c_str()))
                        SetTheme(name);
                ImGui::EndMenu();
            }
            //Edit theme
            ImGui::PushStyleColor(ImGuiCol_Button, INVISIBLE_COLOR);
            if (ImGui::Button("Edit Theme"))
                popupManager->OpenPopup(static_cast<int>(WindowPopups::ThemeEditor));
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

                //Add a new state
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.2f, 1.0f)); //Green button
                if (ImGui::Button("Add State"))
                    popupManager->OpenPopup(static_cast<int>(WindowPopups::AddStateMain));
                ImGui::PopStyleColor(); //Green button

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
        ImGui::SetNextWindowPos(ImVec2(mainViewport->WorkPos.x, mainViewport->WorkPos.y + 30), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(mainViewport->WorkSize.x, mainViewport->WorkSize.y - 30), ImGuiCond_Once);

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

                ImGui::DockBuilderDockWindow("Canvas", dockIdLeft);
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
    uint32_t CANVAS_FLAGS =
        ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoScrollbar
        | ImGuiWindowFlags_NoScrollWithMouse
        | ImGuiWindowFlags_NoMove;

    ImVec2 CANVAS_FULL_SIZE = {4096, 4096};
    
    // Canvas window
    void Window::Canvas()
    {
        const auto nodeEditor = NodeEditor::Get();
        const auto fsm = nodeEditor->GetCurrentFsm();
        if (!fsm)
            return;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        
        //Size of the canvas
        ImGui::SetNextWindowContentSize(CANVAS_FULL_SIZE);
        ImGui::PushFont(nodeEditor->GetFont()); //Node font
        ImGui::Begin("Canvas", &CANVAS_OPEN, CANVAS_FLAGS);
        {
            //Handle initial canvas position
            if (FIRST_OPEN)
            {
                //Center canvas on nodes when loading
                if (fsm->GetInitialState())
                    nodeEditor->MoveToNode(fsm->GetInitialStateId());
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
                    selectedNode->SetGridPos(newPos);
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
                        const auto popup = dynamic_cast<AddStatePopup*>(GetPopupManager()->GetPopup(static_cast<int>(WindowPopups::PasteState)).get());
                        popup->position = CURSOR_POS;
                        popup->Open();
                        break;
                    }
                    case NodeType::Transition:
                    {
                        const auto popup = dynamic_cast<AddTriggerPopup*>(GetPopupManager()->GetPopup(static_cast<int>(WindowPopups::PasteTrigger)).get());
                        popup->position = CURSOR_POS;
                        popup->Open();
                        break;
                    }
                }
            }

            //Draw lines for creating links
            if (const auto selectedNode = nodeEditor->GetSelectedNode();
                selectedNode && ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Right))
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
            if (NodeEditor::Get()->IsCreatingLink() && ImGui::IsWindowHovered() && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
            {
                if (const auto parentNode = nodeEditor->GetSelectedNode(); parentNode)
                {
                    CURSOR_POS = ImGui::GetMousePos() - CANVAS_POS + ImVec2{ImGui::GetScrollX(), ImGui::GetScrollY()} / nodeEditor->GetScale();
                    switch (parentNode->GetType())
                    {
                        case NodeType::State:
                        {
                            const auto popup = dynamic_cast<AddTriggerPopup*>(GetPopupManager()->GetPopup(static_cast<int>(WindowPopups::AddTriggerCursor)).get());
                            popup->position = CURSOR_POS;
                            popup->Open();
                            break;
                        }
                        case NodeType::Transition:
                        {
                            const auto popup = dynamic_cast<AddStatePopup*>(GetPopupManager()->GetPopup(static_cast<int>(WindowPopups::AddStateCursor)).get());
                            popup->position = CURSOR_POS;
                            popup->Open();
                            break;
                        }
                    }
                }
                nodeEditor->SetCreatingLink(false);
            }
            
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Right))
                nodeEditor->SetCreatingLink(false);
            
            ImGui::PopStyleVar(); //ImGuiStyleVar_WindowPadding
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
            const ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Middle);
            ImGui::SetScrollX(ImGui::GetScrollX() - delta.x);
            ImGui::SetScrollY(ImGui::GetScrollY() - delta.y);
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Middle);
        }

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

    //Properties on the right side of the screen
    void Window::Properties()
    {
        const auto nodeEditor = NodeEditor::Get();
        const auto fsm = nodeEditor->GetCurrentFsm();
        if (!fsm)
            return;
        
        ImGui::Begin("Properties", &PROPERTIES_OPEN, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove);
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
            if (const auto node = nodeEditor->GetSelectedNode(); node)
            {
                if (const auto type = node->GetType(); type == NodeType::State)
                {
                    if (const auto state = fsm->GetState(node->GetId()))
                        state->DrawProperties();
                }
                else if (type == NodeType::Transition)
                {
                    if (const auto trigger = fsm->GetTrigger(node->GetId()))
                        trigger->DrawProperties();
                }
            }
            else //if no selected nodes show fsm properties
                nodeEditor->SetShowFsmProps(true);
            
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
