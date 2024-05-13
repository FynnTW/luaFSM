// ReSharper disable StringLiteralTypo
#include "pch.h"
#include "FsmState.h"

#include <memory>

#include "imgui/imgui_stdlib.h"

#include "Log.h"
#include "Graphics/Window.h"
#include "imgui/ImGuiNotify.hpp"
#include "imgui/NodeEditor.h"
#include "IO/FileReader.h"

namespace LuaFsm
{
    FsmState::FsmState(const std::string& id): DrawableObject(id)
    {
        m_OnEnterEditor.SetText(m_OnEnter);
        m_OnEnterEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        m_OnUpdateEditor.SetText(m_OnUpdate);
        m_OnUpdateEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        m_OnExitEditor.SetText(m_OnExit);
        m_OnExitEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        m_LuaCodeEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        m_Node.SetId(m_Id);
        m_Node.SetType(NodeType::State);
        m_Node.SetShape(NodeShape::Ellipse);
        InitPopups();
    }

    FsmState::FsmState(const FsmState& other): DrawableObject(other.GetId())
    {
        m_Name = other.m_Name;
        m_Description = other.m_Description;
        m_OnEnter = other.m_OnEnter;
        m_OnUpdate = other.m_OnUpdate;
        m_OnExit = other.m_OnExit;
        m_IsExitState = other.m_IsExitState;
        m_Node = other.m_Node;
        for (const auto& value : other.m_Triggers | std::views::values)
            AddTrigger(value);
        m_Node.SetGridPos(other.m_Node.GetGridPos());
        m_Node.SetColor(other.m_Node.GetColor());
    }

    void FsmState::SetId(const std::string& id)
    {
        const auto oldId = m_Id;
        m_Id = id;
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
            return;
        if (!oldId.empty())
        {
            for (const auto& [key, value] : fsm->GetStates())
            {
                for (const auto& [argKey, argValue] : value->GetTriggers())
                {
                    if (argValue->GetNextStateId() == oldId)
                        argValue->SetNextState(m_Id);
                    if (argValue->GetCurrentStateId() == oldId)
                        argValue->SetCurrentState(m_Id);
                }
            }
        }
        m_Node.SetId(id);
        if (std::strcmp(id.c_str(), oldId.c_str()) == 0)
            return;
        const auto statePtr = fsm->GetState(oldId);
        if (statePtr == nullptr)
            return;
        fsm->AddState(statePtr);
        fsm->RemoveState(oldId);
    }

    void FsmState::ChangeTriggerId(const std::string& oldId, const std::string& newId)
    {
        if (!m_Triggers.contains(oldId))
            return;
        m_Triggers[newId] = m_Triggers[oldId];
        m_Triggers.erase(oldId);
    }

    void FsmState::UpdateEditors()
    {
        Window::TrimTrailingNewlines(m_OnEnter);
        m_OnEnterEditor.SetText(m_OnEnter);
        Window::TrimTrailingNewlines(m_OnUpdate);
        m_OnUpdateEditor.SetText(m_OnUpdate);
        Window::TrimTrailingNewlines(m_OnExit);
        m_OnExitEditor.SetText(m_OnExit);
        m_LuaCodeEditor.SetText(GetLuaCode());
    }

    void FsmState::AddTrigger(const std::string& key, const FsmTriggerPtr& value)
    {
        m_Triggers[key] = value;
        if (value == nullptr)
            return;
        if (value->GetCurrentStateId() != m_Id)
            value->SetCurrentState(m_Id);
    }


    FsmTriggerPtr FsmState::AddTrigger(const std::string& key)
    {
        m_Triggers[key] = std::make_shared<FsmTrigger>(key);
        auto& value = m_Triggers[key]; // Reference to the shared pointer
        value->SetCurrentState(m_Id);
        return value;
    }

    void FsmState::InitPopups()
    {
        const auto unlinkTriggerPopup = std::make_shared<UnlinkTriggerPopup>("UnlinkTrigger" + m_Id);
        unlinkTriggerPopup->parent = m_Id;
        m_PopupManager.AddPopup(StatePopups::UnlinkTrigger, unlinkTriggerPopup);
        
        const auto deleteStatePopup = std::make_shared<DeleteStatePopup>("DeleteStatePopup" + m_Id);
        deleteStatePopup->parent = m_Id;
        m_PopupManager.AddPopup(StatePopups::DeleteState, deleteStatePopup);
        
        const auto refactorId = std::make_shared<RefactorIdPopup>("SetNewId" + m_Id, "state");
        refactorId->parent = m_Id;
        m_PopupManager.AddPopup(StatePopups::SetNewId, refactorId);
    }
    
    bool UNLINK_TRIGGER = false;
    bool DELETE_STATE = false;

    std::vector<std::shared_ptr<FsmState>> FsmState::CreateFromFile(const std::string& filePath)
    {
        auto states = std::vector<std::shared_ptr<FsmState>>{};
        const std::string code = FileReader::ReadAllText(filePath);
        if (code.empty())
            return states;
        std::regex idRegex = FsmRegex::IdRegex("FSM_STATE");
        std::sregex_iterator iter(code.begin(), code.end(), idRegex);
        for (std::sregex_iterator end; iter != end; ++iter)
        {
            std::string id = (*iter)[1].str();
            auto state = std::make_shared<FsmState>(id);
            states.emplace_back(state);
        }
        return states;
    }
    
    void FsmState::UpdateFromFile(const std::string& filePath)
    {
        const std::string code = FileReader::ReadAllText(filePath);
        if (code.empty())
            return;
        std::string name;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassStringRegex(m_Id, "name")))
            name = match[1].str();
        else
            name = m_Id;
        SetName(name);
        std::string description;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassStringRegex(m_Id, "description")))
            description = match[1].str();
        SetDescription(description);
        bool isExitState = false;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassBoolRegex(m_Id, "isExitState")))
            isExitState = match[1].str() == "true";
        SetExitState(isExitState);
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassTableRegex(m_Id, "editorPos")))
        {
            std::string tableContent = match[1].str();
            std::regex idRegex = FsmRegex::ClassTableElementsRegex();
            std::sregex_iterator iter(tableContent.begin(), tableContent.end(), idRegex);
            std::vector<float> elements;
            for (std::sregex_iterator end; iter != end; ++iter) {
                elements.push_back(std::stof((*iter)[1].str()));
            }
            if (elements.size() >= 2)
            {
                ImVec2 position;
                position.x = elements[0];
                position.y = elements[1];
                GetNode()->SetGridPos(position);
            }
        }
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassTableRegex(m_Id, "color")))
        {
            std::string tableContent = match[1].str();
            std::regex idRegex = FsmRegex::ClassTableElementsRegex();
            std::sregex_iterator iter(tableContent.begin(), tableContent.end(), idRegex);
            std::vector<float> elements;
            for (std::sregex_iterator end; iter != end; ++iter) {
                elements.push_back(std::stof((*iter)[1].str()));
            }
            if (elements.size() >= 4)
            {
                ImColor color{};
                color.Value.x = elements[0];
                color.Value.y = elements[1];
                color.Value.z = elements[2];
                color.Value.w = elements[3];
                GetNode()->SetColor(color);
            }
        }
        std::string onEnter;
        std::regex onEnterRegex = FsmRegex::FunctionBody(m_Id, "onEnter");
        if (std::smatch match; std::regex_search(code, match, onEnterRegex))
            onEnter = match[1].str();
        SetOnEnter(FileReader::RemoveStartingTab(onEnter));
        std::string onUpdate;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::FunctionBody(m_Id, "onUpdate")))
            onUpdate = match[1].str();
        SetOnUpdate(FileReader::RemoveStartingTab(onUpdate));
        std::string onExit;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::FunctionBody(m_Id, "onExit")))
            onExit = match[1].str();
        SetOnExit(FileReader::RemoveStartingTab(onExit));
        UpdateEditors();
        CreateLastState();
        m_UnSaved = false;
    }

    void FsmState::UpdateToFile(const std::string& oldId)
    {
        if (!NodeEditor::Get()->GetCurrentFsm())
            return;
        auto code = NodeEditor::Get()->GetCurrentFsm()->GetLinkedFileCode();
        if (code.empty())
            return;
        UpdateFileContents(code, oldId);
        NodeEditor::Get()->GetCurrentFsm()->SaveLinkedFile(code);
        CreateLastState();
    }

    void FsmState::UpdateFileContents(std::string& code, const std::string& oldId)
    {
        UpdateEditors();
        std::regex regex = FsmRegex::IdRegexClass("FSM_STATE", oldId);
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("---@FSM_STATE {}", m_Id));
        else
        {
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "No Entry for state %s found in file", oldId.c_str()});
            return;
        }
        regex = FsmRegex::IdRegexClassAnnotation("FSM_STATE", oldId);
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("class {0} : FSM_STATE", m_Id));
        regex = FsmRegex::IdRegexClassDeclaration("FSM_STATE", oldId);
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0} = FSM_STATE:new", m_Id));
        regex = FsmRegex::ClassStringRegex(oldId, "name");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.name = \"{1}\"", m_Id, m_Name));
        else if (!m_Name.empty())
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm state %s name entry not found in file!", m_Id.c_str()});
        regex = FsmRegex::ClassStringRegex(oldId, "id");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.id = \"{1}\"", m_Id, m_Id));
        else
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "state id entry not found in file!"});
        regex = FsmRegex::ClassStringRegex(oldId, "description");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.description = \"{1}\"", m_Id, m_Description));
        else if (!m_Description.empty())
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm state %s description entry not found in file!", m_Id.c_str()});
        regex = FsmRegex::ClassBoolRegex(oldId, "isExitState");
        if (std::smatch match; std::regex_search(code, match, regex))
        {
            std::string isExitState = m_IsExitState ? "true" : "false";
            code = std::regex_replace(code, regex, fmt::format("{0}.isExitState = {1}", m_Id, isExitState));
        }
        else if (m_IsExitState)
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm state %s isExitState entry not found in file!", m_Id.c_str()});
        regex = FsmRegex::ClassTableRegex(oldId, "editorPos");
        if (std::smatch match; std::regex_search(code, match, regex))
        {
            std::string tableContent = match[1].str();
            std::regex idRegex = FsmRegex::ClassTableElementsRegex();
            std::sregex_iterator iter(tableContent.begin(), tableContent.end(), idRegex);
            std::vector<float> elements;
            for (std::sregex_iterator end; iter != end; ++iter) {
                elements.push_back(std::stof((*iter)[1].str()));
            }
            if (elements.size() >= 2)
            {
                ImVec2 position;
                position.x = elements[0];
                position.y = elements[1];
                position = GetNode()->GetGridPos();
                code = std::regex_replace(code, regex, fmt::format("{0}.editorPos = {{{1}, {2}}}", m_Id, position.x, position.y));
            }
        }
        else
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm state %s position entry not found in file!", m_Id.c_str()});
        regex = FsmRegex::ClassTableRegex(oldId, "color");
        if (std::smatch match; std::regex_search(code, match, regex))
        {
            std::string tableContent = match[1].str();
            std::regex idRegex = FsmRegex::ClassTableElementsRegex();
            std::sregex_iterator iter(tableContent.begin(), tableContent.end(), idRegex);
            std::vector<float> elements;
            for (std::sregex_iterator end; iter != end; ++iter) {
                elements.push_back(std::stof((*iter)[1].str()));
            }
            if (elements.size() >= 4)
            {
                ImColor color{};
                color.Value.x = elements[0];
                color.Value.y = elements[1];
                color.Value.z = elements[2];
                color.Value.w = elements[3];
                color = GetNode()->GetColor();
                code = std::regex_replace(code, regex, fmt::format("{0}.color = {{{1}, {2}, {3}, {4}}}", m_Id, color.Value.x, color.Value.y, color.Value.z, color.Value.w));
            }
        }
        if (!NodeEditor::Get()->FunctionEditorOnly())
        {
            regex = FsmRegex::FunctionBodyReplace(oldId, "onEnter");
            if (std::smatch match; std::regex_search(code, match, regex))
            {
                std::string func;
                for (const auto& line : m_OnEnterEditor.GetTextLines())
                    func += fmt::format("\t{0}\n", line);
                code = std::regex_replace(code, regex, "$1\n" + func + "$3");
                auto regexString = fmt::format("({0}:onEnter)", oldId);
                regex = std::regex(regexString);
                code = std::regex_replace(code, regex, fmt::format("{0}:onEnter", m_Id));
            }
            else if (!m_OnEnter.empty())
                ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm state %s onEnter entry not found in file!", m_Id.c_str()});
            regex = FsmRegex::FunctionBodyReplace(oldId, "onUpdate");
            if (std::smatch match; std::regex_search(code, match, regex))
            {
                std::string func;
                for (const auto& line : m_OnUpdateEditor.GetTextLines())
                    func += fmt::format("\t{0}\n", line);
                code = std::regex_replace(code, regex, "$1\n" + func + "$3");
                auto regexString = fmt::format("({0}:onUpdate)", oldId);
                regex = std::regex(regexString);
                code = std::regex_replace(code, regex, fmt::format("{0}:onEnter", m_Id));
            }
            else if (!m_OnUpdate.empty())
                ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm state %s onUpdate entry not found in file!", m_Id.c_str()});
            regex = FsmRegex::FunctionBodyReplace(oldId, "onExit");
            if (std::smatch match; std::regex_search(code, match, regex))
            {
                std::string func;
                for (const auto& line : m_OnExitEditor.GetTextLines())
                    func += fmt::format("\t{0}\n", line);
                code = std::regex_replace(code, regex, "$1\n" + func + "$3");
                auto regexString = fmt::format("({0}:onExit)", oldId);
                regex = std::regex(regexString);
                code = std::regex_replace(code, regex, fmt::format("{0}:onEnter", m_Id));
            }
            else if (!m_OnExit.empty())
                ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm state %s onExit entry not found in file!", m_Id.c_str()});
        }
        m_UnSaved = false;
    }

    void FsmState::RefactorId(const std::string& newId)
    {
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        const std::string oldId = m_Id;
        if (!fsm)
            return;
        if (fsm->GetTrigger(newId) || fsm->GetState(newId))
        {
            ImGui::InsertNotification({ImGuiToastType::Error, 3000, "State with id %s already exists!", newId.c_str()});
            return;
        }
        SetId(newId);
        if (fsm->GetLinkedFile().empty())
            return;
        UpdateToFile(oldId);
        fsm->UpdateToFile(NodeEditor::Get()->GetCurrentFsm()->GetId());
    }
#define COMPARE_COLOR(a, b) ((a).x - (b).x > 0.0001f || (a).y - (b).y > 0.0001f || (a).z - (b).z > 0.0001f || (a).w - (b).w > 0.0001f)
    
    void FsmState::DrawProperties()
    {
        if (ImGui::BeginMenuBar())
        {
            if (const auto linkedFile = NodeEditor::Get()->GetCurrentFsm()->GetLinkedFile(); !linkedFile.empty())
            {
                if (ImGui::Button(MakeIdString("Load from file").c_str()))
                    UpdateFromFile(linkedFile);
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Save to file").c_str()))
                    UpdateToFile(m_Id);
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button(MakeIdString("Delete").c_str()))
                    m_PopupManager.OpenPopup(static_cast<int>(StatePopups::DeleteState));
                ImGui::SetItemTooltip("This doesn't delete it from your lua file! Delete it manually!");
                ImGui::PopStyleColor();
            }
            ImGui::EndMenuBar();
        }
        if (ImGui::BeginTabBar(MakeIdString("Node Properties").c_str()))
        {
            if (ImGui::BeginTabItem(MakeIdString("State Properties").c_str()))
            {
                ImGui::Text("ID: %s", GetId().c_str());
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Refactor ID").c_str()))
                    m_PopupManager.OpenPopup(static_cast<int>(StatePopups::SetNewId));
                ImGui::Text("Name");
                std::string name = GetName();
                const std::string nameLabel = "##Name" + GetId();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText(nameLabel.c_str(), &name);
                SetName(name);
                ImGui::Separator();
                ImGui::Text("Description");
                std::string description = GetDescription();
                const std::string descriptionLabel = "##Description" + GetId();
                ImGui::InputTextMultiline(descriptionLabel.c_str(), &description, {ImGui::GetContentRegionAvail().x, 60});
                 SetDescription(description);
                ImGui::Separator();
                bool isInitial = NodeEditor::Get()->GetCurrentFsm()->GetInitialStateId() == GetId();
                if (ImGui::Checkbox(MakeIdString("Is Initial State").c_str(), &isInitial))
                {
                    if (isInitial)
                        NodeEditor::Get()->GetCurrentFsm()->SetInitialState(GetId());
                    else if (NodeEditor::Get()->GetCurrentFsm()->GetInitialStateId() == GetId())
                        NodeEditor::Get()->GetCurrentFsm()->SetInitialState("");
                }
                ImGui::SameLine();
                ImGui::Checkbox(MakeIdString("Is Exit State").c_str(), &m_IsExitState);
                ImGui::Separator();
                auto color = m_Node.GetColor();
                ImGui::ColorEdit4(MakeIdString("Node Color").c_str(), reinterpret_cast<float*>(&color));
                if (ImVec4(color) != ImVec4(m_Node.GetColor()))
                {
                    m_Node.SetColor(color);
                    m_Node.SetCurrentColor(color);
                }
                ImGui::Separator();
                if (!IsExitState())
                {
                    ImGui::Text("Triggers");
                    if (ImGui::BeginListBox(MakeIdString("Triggers").c_str(),{300, 120 }))
                    {
                        std::vector<FsmTrigger*> sortedTriggers;
                        for (const auto& [key, value] : GetTriggers())
                            sortedTriggers.push_back(value.get());
                        std::ranges::sort(sortedTriggers,
                            [](const FsmTrigger* a, const FsmTrigger* b)
                        {
                            return a->GetPriority() > b->GetPriority();
                        });
                        for (const auto& trigger : sortedTriggers)
                        {
                            std::string key = trigger->GetId();
                            std::string label = key + fmt::format(" : priority {0}", trigger->GetPriority());
                            ImGui::Selectable(MakeIdString(label).c_str(), false);
                            ImGui::SetItemTooltip(fmt::format("{0}\n{1}", trigger->GetName(), trigger->GetDescription()).c_str());
                            if (ImGui::IsItemClicked())
                                NodeEditor::Get()->SetSelectedNode(trigger->GetNode());
                            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                            {
                                m_PopupManager.GetPopup<UnlinkTriggerPopup>(StatePopups::UnlinkTrigger)->triggerId = key;
                                m_PopupManager.OpenPopup(StatePopups::UnlinkTrigger);
                            }
                            if (ImGui::IsItemHovered())
                            {
                                trigger->GetNode()->SetIsHighlighted(true);
                                if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
                                    NodeEditor::Get()->MoveToNode(key, NodeType::Transition);
                                if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                                {
                                    m_PopupManager.GetPopup<UnlinkTriggerPopup>(StatePopups::UnlinkTrigger)->triggerId = key;
                                    m_PopupManager.OpenPopup(StatePopups::UnlinkTrigger);
                                }
                            }
                            else
                                trigger->GetNode()->SetIsHighlighted(false);
                        }
                        ImGui::EndListBox();
                    }
                    ImGui::Separator();
                }
                else if (!m_Triggers.empty())
                {
                    for (const auto& [key, value] : GetTriggers())
                        RemoveTrigger(key);
                }
                ImGui::Text("OnEnter:");
                Window::DrawTextEditor(m_OnEnterEditor, m_OnEnter);
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("OnUpdate").c_str()))
            {
                Window::DrawTextEditor(m_OnUpdateEditor, m_OnUpdate);
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("OnExit").c_str()))
            {
                Window::DrawTextEditor(m_OnExitEditor, m_OnExit);
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("Lua Code").c_str()))
            {
                if (ImGui::Button(MakeIdString("Regenerate Code").c_str()) || m_LuaCodeEditor.GetText().empty())
                    m_LuaCodeEditor.SetText(GetLuaCode());
                m_LuaCodeEditor.Render("Lua Code");
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }

        m_PopupManager.ShowOpenPopups();
    }

    void FsmState::CreateLastState()
    {
        m_PreviousState = std::make_shared<FsmState>(*this);
    }

    bool FsmState::IsChanged()
    {
        if (m_PreviousState == nullptr)
            return true;
        if (*this != *m_PreviousState || m_UnSaved)
        {
            m_UnSaved = true;
            return true;
        }
        return false;
    }

    void FsmState::AddTrigger(const FsmTriggerPtr& value)
    {
        const auto key = value->GetId();
        AddTrigger(key, value);
    }

    FsmTriggerPtr FsmState::GetTrigger(const std::string& key)
    {
        if (!m_Triggers.contains(key))
            return nullptr;
        return m_Triggers[key];
    }

    void FsmState::RemoveTrigger(const std::string& trigger)
    {
        const auto obj = GetTrigger(trigger);
        if (obj == nullptr)
            return;
        obj->SetCurrentState("");
        m_Triggers.erase(trigger);
    }

    void FsmState::AppendToFile()
    {
        if (const auto fsm = NodeEditor::Get()->GetCurrentFsm(); fsm)
        {
            const auto filePath = fsm->GetLinkedFile();
            if (filePath.empty())
                return;
            auto code = FileReader::ReadAllText(filePath);
            if (code.empty())
                return;
            code += "\n\n";
            code += GetLuaCode();
            std::ofstream file(filePath);
            if (!file.is_open())
            {
                ImGui::InsertNotification({ImGuiToastType::Error, 3000, "Failed to export file at: %s", filePath.c_str()});
                return;
            }
            file << code;
            file.close();
            CreateLastState();
        }
    }
    
    std::string FsmState::GetLuaCode()
    {
        std::string code;
        code += fmt::format("---@FSM_STATE {0}\n", m_Id);
        code += fmt::format("---@class {0} : FSM_STATE\n", m_Id);
        code += fmt::format("local {0} = FSM_STATE:new({{}})\n", m_Id);
        code += fmt::format("{0}.id = \"{1}\"\n", m_Id, m_Id);
        code += fmt::format("{0}.name = \"{1}\"\n", m_Id, m_Name);
        code += fmt::format("{0}.description = \"{1}\"\n", m_Id, m_Description);
        std::string isExitState = m_IsExitState ? "true" : "false";
        code += fmt::format("{0}.isExitState = {1}\n", m_Id, isExitState);
        code += fmt::format("{0}.editorPos = {{{1}, {2}}}\n", m_Id, m_Node.GetGridPos().x, m_Node.GetGridPos().y);
        code += fmt::format("{0}.color = {{{1}, {2}, {3}, {4}}}\n", m_Id, m_Node.GetColor().Value.x, m_Node.GetColor().Value.y, m_Node.GetColor().Value.z, m_Node.GetColor().Value.w);
        if (!m_OnUpdate.empty())
        {
            code += fmt::format("\nfunction {0}:onUpdate()\n", m_Id);
            for (const auto& line : m_OnUpdateEditor.GetTextLines())
                code += fmt::format("\t{0}\n", line);
            code += fmt::format("end---@endFunc\n");
        }
        if (!m_OnEnter.empty())
        {
            code += fmt::format("\nfunction {0}:onEnter()\n", m_Id);
            for (const auto& line : m_OnEnterEditor.GetTextLines())
                code += fmt::format("\t{0}\n", line);
            code += fmt::format("end---@endFunc\n");
        }
        if (!m_OnExit.empty())
        {
            code += fmt::format("\nfunction {0}:onExit()\n", m_Id);
            for (const auto& line : m_OnExitEditor.GetTextLines())
                code += fmt::format("\t{0}\n", line);
            code += fmt::format("end---@endFunc\n");
        }
        //for (const auto& value : m_Triggers | std::views::values)
        //{
        //    code += value->GetLuaCode() + "\n";
        //}
        return code;
    }
    
    
    VisualNode* FsmState::DrawNode()
    {
        return m_Node.Draw(this);
    }
}
