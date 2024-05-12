#include "pch.h"
#include "Popup.h"

#include "Graphics/Window.h"
#include "imgui/ImGuiNotify.hpp"
#include "imgui/imgui_stdlib.h"
#include "imgui/NodeEditor.h"
#include "IO/FileReader.h"

namespace LuaFsm
{
    void PopupManager::ShowOpenPopups()
    {
        for (const auto& popup : m_Popups | std::views::values)
        {
            if (popup->isOpen)
            {
                popup->Open();
                popup->isOpen = false;
            }
            popup->Draw();
        }
    }

    Popup::~Popup() = default;

    bool Popup::Draw()
    {
        if (ImGui::BeginPopup(id.c_str()))
        {
            DrawFields();
            DrawButtons();
            ImGui::EndPopup();
            return true;
        }
        return false;
    }

    void Popup::Open()
    {
        ImGui::OpenPopup(id.c_str());
    }

    void Popup::Close()
    {
        isOpen = false;
        ImGui::CloseCurrentPopup();
    }

    bool Popup::OpenFileDialog(const std::string& key, const std::string& title,
                               const std::string& ext,
                               std::string* folder,
                               std::string* filePath,
                               const IGFD::FileDialogConfig& config)
    {
        ImGuiFileDialog::Instance()->OpenDialog(key, title, ext.c_str(), config);
        if (ImGuiFileDialog::Instance()->Display(key, ImGuiWindowFlags_NoCollapse, ImVec2({600, 500}) ))
        {
            if (ImGuiFileDialog::Instance()->IsOk()) { // action if 
                *folder = ImGuiFileDialog::Instance()->GetCurrentPath();
                *filePath = *folder + "\\";
                *filePath += ImGuiFileDialog::Instance()->GetCurrentFileName();
                if (ext == ".lua")
                {
                    FileReader::lastFilePath = *filePath;
                    FileReader::lastPath = *folder;
                }
            }
            // close
            ImGuiFileDialog::Instance()->Close();
            ImGui::CloseCurrentPopup();
            return true;
        }
        return true;
    }

   AddFsmPopup::AddFsmPopup()
   {
       id = "AddFSM";
       fsmId = "";
       fsmName = "";
   }

   void AddFsmPopup::DrawFields()
    {
        ImGui::Text("Add FSM");
        ImGui::InputText("FSM ID", &fsmId, 0);
        ImGui::SetItemTooltip("Unique identifier for the FSM. This is used as the global variable name in Lua.");
        ImGui::InputText("FSM Name", &fsmName, 0);
        ImGui::SetItemTooltip("Just for visual purposes in this editor.");
    }

    void AddFsmPopup::DrawButtons()
    {
        if (const auto invalid =  NodeEditor::Get()->CheckIdValidity(fsmId);
            invalid == IdValidityError::Valid && !fsmName.empty())
        {
            if (ImGui::Button("Add"))
            {
                const auto fsm = std::make_shared<Fsm>(fsmId);
                fsm->SetName(fsmName);
                NodeEditor::Get()->SetCurrentFsm(fsm);
                NodeEditor::Get()->SetCanvasScroll({1024, 1024});
                fsmId = "";
                fsmName = "";
                Close();
            }
            ImGui::SameLine();
        }
        else if (fsmName.empty())
            ImGui::Text("FSM Name cannot be empty");
        else
            NodeEditor::InformValidityError(invalid);
        if (ImGui::Button("Cancel"))
            Close();
    }

    ThemeEditor::ThemeEditor()
    {
        id = "ThemeEditor";
        name = "";
        lastActiveTheme = "";
        standard = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        hovered = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        active = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        text = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        background = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        windowBg = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    void ThemeEditor::DrawFields()
    {
        if (const auto activeTheme = Window::GetActiveTheme(); activeTheme != lastActiveTheme)
        {
            lastActiveTheme = activeTheme;
            standard = Window::GetTheme(activeTheme).standard;
            hovered = Window::GetTheme(activeTheme).hovered;
            active = Window::GetTheme(activeTheme).active;
            text = Window::GetTheme(activeTheme).text;
            background = Window::GetTheme(activeTheme).background;
            windowBg = Window::GetTheme(activeTheme).windowBg;
            name = activeTheme;
        }
        ImGui::ColorEdit4("standard", reinterpret_cast<float*>(&standard));
        ImGui::ColorEdit4("hovered", reinterpret_cast<float*>(&hovered));
        ImGui::ColorEdit4("active", reinterpret_cast<float*>(&active));
        ImGui::ColorEdit4("text", reinterpret_cast<float*>(&text));
        ImGui::ColorEdit4("background", reinterpret_cast<float*>(&background));
        ImGui::ColorEdit4("windowBg", reinterpret_cast<float*>(&windowBg));
        Window::SetTheme({standard, hovered, active, text, background, windowBg, ""});
        ImGui::InputText("Theme Name", &name);
    }

    void ThemeEditor::DrawButtons()
    {
        const auto activeTheme = Window::GetActiveTheme();
        if (!name.empty())
        {
            if (ImGui::Button("Save"))
            {
                auto theme = Window::ImGuiColorTheme{standard, hovered, active, text, background, windowBg, name};
                Window::AddTheme(name, theme);
                Window::SetTheme(name);
                const auto json = theme.Serialize();
                FileReader::SaveFile("assets/themes/" + name + ".json", json.dump(4));
                NodeEditor::Get()->SaveSettings();
                name = "";
            }
            ImGui::SameLine();
        }
        else
            ImGui::Text("Input a name to save the theme.");
        if (ImGui::Button("Close"))
        {
            Window::SetTheme(activeTheme);
            Close();
        }
    }

    void ThemeEditor::Open()
    {
        if (!Window::GetActiveTheme().empty())
            Popup::Open();
    }

    LoadFile::LoadFile()
    {
        id = "LoadFile";
        config.flags |= ImGuiWindowFlags_NoCollapse;
        config.path = ".";
    }

    void LoadFile::DrawFields()
    {
        if (filePath.empty())
        {
            OpenFileDialog("LoadFileDialog", "Load File", ".lua", &folder, &filePath, config);
            if (!filePath.empty())
            {
                Fsm::CreateFromFile(filePath);
                NodeEditor::Get()->SaveSettings();
                NodeEditor::Get()->MoveToNode(NodeEditor::Get()->GetCurrentFsm()->GetInitialStateId(), NodeType::State);
                folder = "";
                filePath = "";
                Close();
            }
        }
    }

    void LoadFile::Open()
    {
        if (!FileReader::lastFilePath.empty())
            config.filePathName = FileReader::lastFilePath;
        else if (!FileReader::lastPath.empty())
            config.path = FileReader::lastPath;
        Popup::Open();
    }

    CreateFilePopup::CreateFilePopup()
    {
        id = "CreateFile";
        config.flags |= ImGuiWindowFlags_NoCollapse;
        config.path = ".";
    }

    void CreateFilePopup::DrawFields()
    {
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
        {
            Close();
            return;
        }
        if (filePath.empty())
        {
            if (!FileReader::lastFilePath.empty())
                config.filePathName = FileReader::lastFilePath;
            else
            {
                if (!FileReader::lastPath.empty())
                {
                    config.path = FileReader::lastPath;
                }
                config.fileName = fmt::format("{0}.lua", NodeEditor::Get()->GetCurrentFsm()->GetId());
            }
            if (!OpenFileDialog("CreateFileDialog", "Create File", ".lua", &folder, &filePath, config))
                isOpen = true;
            else
                isOpen = false;
            if (!filePath.empty())
            {
                const auto code = fsm->GetLuaCode();
                FileReader::SaveFile(filePath, code);
                fsm->SetLinkedFile(filePath);
                NodeEditor::Get()->SaveSettings();
                folder = "";
                filePath = "";
                Close();
            }
        }
    }

    AddStatePopup::AddStatePopup(const std::string& instanceId)
    {
        id = instanceId;
        name = "";
        stateId = "";
    }

    void AddStatePopup::DrawFields()
    {
        ImGui::Text("Add State");
        ImGui::InputText("State ID", &stateId);
        ImGui::SetItemTooltip("Used for internal identification and must be unique.\nChanging this requires refactor.");
        ImGui::InputText("State Name", &name);
        ImGui::SetItemTooltip("Name of the state that will be displayed in the editor.");
    }

    void AddStatePopup::DrawButtons()
    {
        const auto nodeEditor = NodeEditor::Get();
        if (isDrawn && (!nodeEditor->GetSelectedNode() || nodeEditor->GetSelectedNode()->GetType() == NodeType::State))
        {
            Close();
            return;
        }
        if (isCopy && (!nodeEditor->GetCopiedNode()
            || nodeEditor->GetSelectedNode()->GetType() != NodeType::State
            || !nodeEditor->GetCurrentFsm()->GetState(nodeEditor->GetCopiedNode()->GetId())
            ))
        {
            Close();
            return;
        }
        if (const auto invalid =  nodeEditor->CheckIdValidity(stateId);
            invalid == IdValidityError::Valid && !name.empty())
        {
            if (ImGui::Button("Add"))
            {
                const auto state = std::make_shared<FsmState>(stateId);
                state->SetName(name);
                nodeEditor->GetCurrentFsm()->AddState(state);
                if (position.x < 0)
                {
                    if (const ImGuiWindow* canvas = ImGui::FindWindowByName("Canvas"))
                        position = ImVec2(canvas->Scroll.x + canvas->Size.x / 4, canvas->Scroll.y + canvas->Size.y / 4);
                    else
                        position = {1024,1024};
                }
                state->GetNode()->SetGridPos(position);
                if (isDrawn)
                {
                    const auto fromNode = nodeEditor->GetSelectedNode();
                    if (const auto trigger = nodeEditor->GetCurrentFsm()->GetTrigger(fromNode->GetId()))
                        trigger->SetNextState(state->GetId());
                }
                if (isCopy)
                {
                    const auto copiedNode = nodeEditor->GetCopiedNode();
                    const auto originalState = nodeEditor->GetCurrentFsm()->GetState(copiedNode->GetId());
                    state->SetOnEnter(originalState->GetOnEnter());
                    state->SetOnUpdate(originalState->GetOnUpdate());
                    state->SetOnExit(originalState->GetOnExit());
                    state->SetDescription(originalState->GetDescription());
                    state->SetExitState(originalState->IsExitState());
                    state->GetNode()->SetColor(originalState->GetNode()->GetColor());
                }
                if (nodeEditor->AppendStates())
                {
                    state->AppendToFile();
                    ImGui::InsertNotification({ImGuiToastType::Success, 3000, "State added: %s", stateId.c_str()});
                }
                else
                {
                    ImGui::SetClipboardText(state->GetLuaCode().c_str());
                    ImGui::InsertNotification({ImGuiToastType::Success, 3000, "State added: %s, code copied to clipboard", stateId.c_str()});
                }
                stateId = "";
                name = "";
                Close();
            }
            ImGui::SameLine();
        }
        else if (name.empty())
            ImGui::Text("Name cannot be empty");
        else
            NodeEditor::InformValidityError(invalid);
        if (ImGui::Button("Cancel"))
            Close();
    }

    AddTriggerPopup::AddTriggerPopup(const std::string& instanceId)
    {
        id = instanceId;
        name = "";
        triggerId = "";
    }

    void AddTriggerPopup::DrawFields()
    {
        ImGui::Text("Add Condition");
        ImGui::InputText("Condition ID", &triggerId);
        ImGui::SetItemTooltip("Used for internal identification and must be unique.\nChanging this requires refactor.");
        ImGui::InputText("Condition Name", &name);
        ImGui::SetItemTooltip("Name of the condition that will be displayed in the editor.");
    }
    
    void AddTriggerPopup::DrawButtons()
    {
        const auto nodeEditor = NodeEditor::Get();
        if (isDrawn && (!nodeEditor->GetSelectedNode() || nodeEditor->GetSelectedNode()->GetType() == NodeType::Transition))
        {
            isOpen = false;
            ImGui::CloseCurrentPopup();
            return;
        }
        if (isCopy && (!nodeEditor->GetCopiedNode()
            || nodeEditor->GetSelectedNode()->GetType() != NodeType::Transition
            || !nodeEditor->GetCurrentFsm()->GetTrigger(nodeEditor->GetCopiedNode()->GetId())
            ))
        {
            isOpen = false;
            ImGui::CloseCurrentPopup();
            return;
        }
        if (const auto invalid =  nodeEditor->CheckIdValidity(triggerId);
            invalid == IdValidityError::Valid && !name.empty())
        {
            if (ImGui::Button("Add"))
            {
                const auto condition = std::make_shared<FsmTrigger>(triggerId);
                condition->SetName(name);
                nodeEditor->GetCurrentFsm()->AddTrigger(condition);
                if (position.x < 0)
                {
                    if (const ImGuiWindow* canvas = ImGui::FindWindowByName("Canvas"))
                        position = ImVec2(canvas->Scroll.x + canvas->Size.x / 4, canvas->Scroll.y + canvas->Size.y / 4);
                    else
                        position = {1024,1024};
                }
                condition->GetNode()->SetGridPos(position);
                if (isDrawn)
                {
                    const auto fromNode = nodeEditor->GetSelectedNode();
                    if (const auto state = nodeEditor->GetCurrentFsm()->GetState(fromNode->GetId()))
                        condition->SetCurrentState(state->GetId());
                }
                if (isCopy)
                {
                    const auto copiedNode = nodeEditor->GetCopiedNode();
                    const auto originalCondition = nodeEditor->GetCurrentFsm()->GetTrigger(copiedNode->GetId());
                    condition->SetDescription(originalCondition->GetDescription());
                    condition->SetCondition(originalCondition->GetCondition());
                    condition->SetAction(originalCondition->GetAction());
                    condition->GetNode()->SetColor(originalCondition->GetNode()->GetColor());
                    condition->SetPriority(originalCondition->GetPriority());
                }
                if (nodeEditor->AppendStates())
                {
                    condition->AppendToFile();
                    ImGui::InsertNotification({ImGuiToastType::Success, 3000, "Condition added: %s", triggerId.c_str()});
                }
                else
                {
                    ImGui::SetClipboardText(condition->GetLuaCode().c_str());
                    ImGui::InsertNotification({ImGuiToastType::Success, 3000, "Condition added: %s, code copied to clipboard", triggerId.c_str()});
                }
                triggerId = "";
                name = "";
                Close();
            }
            ImGui::SameLine();
        }
        else if (name.empty())
            ImGui::Text("Name cannot be empty");
        else
            NodeEditor::InformValidityError(invalid);
        if (ImGui::Button("Cancel"))
            Close();
    }

    UnlinkTriggerPopup::UnlinkTriggerPopup(const std::string& instanceId)
    {
        id = instanceId;
        triggerId = "";
    }

    UnlinkStatePopup::UnlinkStatePopup(const std::string& instanceId)
    {
        id = instanceId;
        stateId = "";
    }

    void UnlinkTriggerPopup::DrawButtons()
    {
        const auto state = NodeEditor::Get()->GetCurrentFsm()->GetState(parent);
        if (!state)
        {
            ImGui::CloseCurrentPopup();
            return;
        }
        const std::string removeId = MakeIdString("Unlink Trigger") + triggerId;
        if (ImGui::Button(removeId.c_str()))
        {
            state->RemoveTrigger(triggerId);
            triggerId = "";
            Close();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            triggerId = "";
            Close();
        }
    }

    void UnlinkStatePopup::DrawButtons()
    {
        const auto trigger = NodeEditor::Get()->GetCurrentFsm()->GetTrigger(parent);
        if (!trigger)
        {
            ImGui::CloseCurrentPopup();
            return;
        }
        const std::string removeId = MakeIdString("Unlink State") + stateId;
        if (ImGui::Button(removeId.c_str()))
        {
            if (stateId == trigger->GetNextStateId())
            {
                if (const auto state = trigger->GetCurrentState())
                    state->RemoveTrigger(parent);
                trigger->SetNextState("");
            }
            else if (stateId == trigger->GetCurrentStateId())
            {
                trigger->SetCurrentState("");
            }
            stateId = "";
            Close();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            stateId = "";
            Close();
        }
    }

    DeleteTriggerPopup::DeleteTriggerPopup(const std::string& instanceId)
    {
        id = instanceId;
        parent = "";
    }

    DeleteStatePopup::DeleteStatePopup(const std::string& instanceId)
    {
        id = instanceId;
        parent = "";
    }

    void DeleteStatePopup::DrawButtons()
    {
        if (const auto state = NodeEditor::Get()->GetCurrentFsm()->GetState(parent); !state)
        {
            ImGui::CloseCurrentPopup();
            return;
        }
        const std::string removeId = MakeIdString("Are you sure?") + parent;
        if (ImGui::Button(removeId.c_str()))
        {
            NodeEditor::Get()->GetCurrentFsm()->RemoveState(parent);
            const std::string filePath = NodeEditor::Get()->GetCurrentFsm()->GetLinkedFile();
            if (filePath.empty())
            {
                ImGui::InsertNotification({ImGuiToastType::Error, 3000, "Failed to export file at: %s", filePath.c_str()});
                ImGui::CloseCurrentPopup();
                return;
            }
            std::string code = FileReader::ReadAllText(filePath);
            if (code.empty())
            {
                ImGui::InsertNotification({ImGuiToastType::Error, 3000, "Failed to export file at: %s", filePath.c_str()});
                ImGui::CloseCurrentPopup();
                return;
            }
            const std::regex regex = FsmRegex::IdRegexClassFull("FSM_STATE", parent);
            if (std::smatch match; std::regex_search(code, match, regex))
                code = std::regex_replace(code, regex, "");
            FileReader::SaveFile(filePath, code);
            Close();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            Close();
    }

    void DeleteTriggerPopup::DrawButtons()
    {
        if (const auto trigger = NodeEditor::Get()->GetCurrentFsm()->GetTrigger(parent); !trigger)
        {
            ImGui::CloseCurrentPopup();
            return;
        }
        const std::string removeId = MakeIdString("Are you sure?") + parent;
        if (ImGui::Button(removeId.c_str()))
        {
            NodeEditor::Get()->GetCurrentFsm()->RemoveTrigger(parent);
            const std::string filePath = NodeEditor::Get()->GetCurrentFsm()->GetLinkedFile();
            if (filePath.empty())
            {
                ImGui::InsertNotification({ImGuiToastType::Error, 3000, "Failed to export file at: %s", filePath.c_str()});
                ImGui::CloseCurrentPopup();
                return;
            }
            std::string code = FileReader::ReadAllText(filePath);
            if (code.empty())
            {
                ImGui::InsertNotification({ImGuiToastType::Error, 3000, "Failed to export file at: %s", filePath.c_str()});
                ImGui::CloseCurrentPopup();
                return;
            }
            const std::regex regex = FsmRegex::IdRegexClassFull("FSM_CONDITION", parent);
            if (std::smatch match; std::regex_search(code, match, regex))
                code = std::regex_replace(code, regex, "");
            FileReader::SaveFile(filePath, code);
            Close();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            Close();
    }

    RefactorIdPopup::RefactorIdPopup(const std::string& instanceId, const std::string& objType)
    {
        id = instanceId;
        parent = "";
        this->objType = objType;
    }

    void RefactorIdPopup::DrawFields()
    {
        ImGui::InputText(MakeIdString("New ID").c_str(), &newId);
    }

    void RefactorIdPopup::DrawButtons()
    {
        const auto nodeEditor = NodeEditor::Get();
        if (const auto invalid =  nodeEditor->CheckIdValidity(newId);
            invalid == IdValidityError::Valid)
        {
            if (ImGui::Button(MakeIdString("Refactor").c_str()))
            {
                if (objType == "state")
                {
                    if (const auto state = nodeEditor->GetCurrentFsm()->GetState(parent); state)
                    {
                        state->RefactorId(newId);
                        newId = "";
                        Close();
                    }
                }
                else if (objType == "trigger")
                {
                    if (const auto trigger = nodeEditor->GetCurrentFsm()->GetTrigger(parent); trigger)
                    {
                        trigger->RefactorId(newId);
                        newId = "";
                        Close();
                    }
                }
                else if (objType == "fsm")
                {
                    if (const auto fsm = nodeEditor->GetCurrentFsm(); fsm)
                    {
                        fsm->RefactorId(newId);
                        newId = "";
                        Close();
                    }
                }
            }
            ImGui::SameLine();
        }
        else
            NodeEditor::InformValidityError(invalid);
        if (ImGui::Button("Cancel"))
            Close();
    }

    OptionsPopUp::OptionsPopUp()
    {
        appendToFile = NodeEditor::Get()->AppendStates();
        showPriority = NodeEditor::Get()->ShowPriority();
    }

    void OptionsPopUp::DrawFields()
    {
        ImGui::Checkbox("Append to file", &appendToFile);
        ImGui::SetItemTooltip("If checked, new states and conditions will be appended to the linked file.\nIf unchecked, the code will be copied to the clipboard.");
        ImGui::Checkbox("Show Priority", &showPriority);
        ImGui::SetItemTooltip("Show priority of conditions on the canvas.");
    }

    void OptionsPopUp::DrawButtons()
    {
        if (ImGui::Button("Save"))
        {
            NodeEditor::Get()->SetAppendStates(appendToFile);
            NodeEditor::Get()->SetShowPriority(showPriority);
            NodeEditor::Get()->SaveSettings();
            Close();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            Close();
    }
}
