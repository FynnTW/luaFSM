// ReSharper disable StringLiteralTypo
#include "pch.h"
#include "FsmState.h"

#include <spdlog/fmt/bundled/format.h>
#include "Graphics/Math.h"
#include "imgui/imgui_stdlib.h"

#include "Log.h"
#include "Graphics/Window.h"
#include "imgui/ImGuiNotify.hpp"
#include "imgui/NodeEditor.h"
#include "IO/FileReader.h"

namespace LuaFsm
{
    FsmState::FsmState(const std::string& id)
    {
        FsmState::SetId(id);
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

    nlohmann::json FsmState::Serialize()
    {
        nlohmann::json j;
        j["id"] = m_Id;
        j["name"] = m_Name;
        j["description"] = m_Description;
        j["onEnter"] = m_OnEnter;
        j["onUpdate"] = m_OnUpdate;
        j["onExit"] = m_OnExit;
        j["positionX"] = m_Node.GetTargetPosition().x;
        j["positionY"] = m_Node.GetTargetPosition().y;
        return j;
    }

    void FsmState::ChangeTriggerId(const std::string& oldId, const std::string& newId)
    {
        if (!m_Triggers.contains(oldId))
            return;
        m_Triggers[newId] = m_Triggers[oldId];
        m_Triggers.erase(oldId);
    }

    std::shared_ptr<FsmState> FsmState::Deserialize(const nlohmann::json& json)
    {
        auto state = std::make_shared<FsmState>(json["id"].get<std::string>());
        state->SetName(json["name"].get<std::string>());
        state->SetDescription(json["description"].get<std::string>());
        state->SetOnEnter(json["onEnter"].get<std::string>());
        state->SetOnUpdate(json["onUpdate"].get<std::string>());
        state->SetOnExit(json["onExit"].get<std::string>());
        state->GetNode()->SetTargetPosition({json["positionX"].get<float>(), json["positionY"].get<float>()});
        state->UpdateEditors();
        return state;
    }

    void FsmState::UpdateEditors()
    {
        m_OnEnterEditor.SetText(m_OnEnter);
        m_OnUpdateEditor.SetText(m_OnUpdate);
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

    bool IS_ADD_EVENT_OPEN = false;
    bool IS_ADD_TRIGGER_OPEN = false;
    bool IS_ADD_DATA_OPEN = false;
    bool IS_ADD_STATE_ARGUMENT_OPEN = false;
    bool IS_EDIT_STATE_ARGUMENT_OPEN = false;
    bool OPEN_EVENT_POPUP = false;
    std::string EVENT_TO_REMOVE;
    std::string TRIGGER_TO_REMOVE;
    std::pair<std::string, std::string> EDITED_ARGUMENT;

    std::string FsmState::MakeIdString(const std::string& name) const
    {
        return name + "##" + m_Id;
    }

    std::string NEW_EVENT;
    std::string dataKey;
    bool UNLINK_TRIGGER = false;
    bool DELETE_STATE = false;
    bool EDIT_DATA_DESCR = false;
    std::map<std::string, StateData> tempData;

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

    //std::string capturingRegexState = "\\s*([\\s\\S]*?)end(?!\\s*end\b)(?=(\\s*---@return|\\s*---@param|\\s*function|\\s*$))";
    std::string CAPTURING_REGEX_STATE = R"(\s*([\s\S]*?)\s*end---@endFunc)";
    
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
        SetOnEnter(onEnter);
        std::string onUpdate;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::FunctionBody(m_Id, "onUpdate")))
            onUpdate = match[1].str();
        SetOnUpdate(onUpdate);
        std::string onExit;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::FunctionBody(m_Id, "onExit")))
            onExit = match[1].str();
        SetOnExit(onExit);
        UpdateEditors();
    }

    void FsmState::UpdateToFile(const std::string& oldId)
    {
        if (!NodeEditor::Get()->GetCurrentFsm())
            return;
        const auto filePath = NodeEditor::Get()->GetCurrentFsm()->GetLinkedFile();
        if (filePath.empty())
            return;
        auto code = FileReader::ReadAllText(filePath);
        if (code.empty())
            return;
        UpdateFileContents(code, oldId);
        std::ofstream file(filePath);
        if (!file.is_open())
        {
            ImGui::InsertNotification({ImGuiToastType::Error, 3000, "Failed to export file at: %s", filePath.c_str()});
            return;
        }
        file << code;
        file.close();
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
        regex = FsmRegex::ClassStringRegex(oldId, "name");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.name = \"{1}\"", m_Id, m_Name));
        else if (!m_Name.empty())
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm state %s name entry not found in file!", m_Id.c_str()});
        regex = FsmRegex::ClassStringRegex(oldId, "description");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.description = \"{1}\"", m_Id, m_Description));
        else if (!m_Description.empty())
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm state %s description entry not found in file!", m_Id.c_str()});
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
        fsm->UpdateToFile();
    }

    bool SET_NEW_ID = false;
    
    void FsmState::DrawProperties()
    {
        if (ImGui::Button(MakeIdString("Preview Lua Code").c_str()) || m_LuaCodeEditor.GetText().empty())
            m_LuaCodeEditor.SetText(GetLuaCode());
        ImGui::SameLine();
        if (const auto linkedFile = NodeEditor::Get()->GetCurrentFsm()->GetLinkedFile(); !linkedFile.empty()
            && ImGui::Button(MakeIdString("Refresh From File").c_str()))
        {
            UpdateFromFile(linkedFile);
        }
        ImGui::SameLine();
        if (ImGui::Button(MakeIdString("Set as initial").c_str()))
            NodeEditor::Get()->GetCurrentFsm()->SetInitialState(GetId());
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button(MakeIdString("Delete State").c_str()))
            DELETE_STATE = true;
        ImGui::SetItemTooltip("This doesn't delete it from your lua file! Delete it manually!");
        ImGui::PopStyleColor();
        ImGui::Separator();
        if (ImGui::BeginTabBar(MakeIdString("Node Properties").c_str()))
        {
            if (ImGui::BeginTabItem(MakeIdString("State Properties").c_str()))
            {
                /*
                ImGui::Text("Position: %f, %f", m_Node.GetGridPos().x, m_Node.GetGridPos().y);
                ImGui::Text("DrawPos: %f, %f", m_Node.GetLastDrawPos().x, m_Node.GetLastDrawPos().y);
                */
                ImGui::Text("ID: %s", GetId().c_str());
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Refactor ID").c_str()))
                    SET_NEW_ID = true;
                ImGui::Text("Name");
                std::string name = GetName();
                std::string nameLabel = "##Name" + GetId();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText(nameLabel.c_str(), &name);
                SetName(name);
                ImGui::Separator();
                ImGui::Text("Description");
                std::string description = GetDescription();
                std::string descriptionLabel = "##Description" + GetId();
                ImGui::InputTextMultiline(descriptionLabel.c_str(), &description, {ImGui::GetContentRegionAvail().x, 100});
                 SetDescription(description);
                ImGui::Separator();
                auto color = m_Node.GetColor();
                ImGui::ColorEdit4("Node Color", reinterpret_cast<float*>(&color));
                m_Node.SetColor(color);
                m_Node.SetCurrentColor(color);
                if (m_Node.IsSelected())
                    m_Node.HighLightSelected();
                ImGui::Separator();
                ImGui::Text("Triggers");
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Add Trigger").c_str()))
                    IS_ADD_TRIGGER_OPEN = true;
                if (ImGui::BeginListBox(MakeIdString("Triggers").c_str(),{300, 75 }))
                {
                    for (const auto& [key, value] : GetTriggers())
                    {
                        ImGui::Selectable(MakeIdString(key).c_str(), false);
                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        {
                            NodeEditor::Get()->SetSelectedNode(value->GetNode());
                            if (ImGuiWindow* canvas = ImGui::FindWindowByName("Canvas"))
                                canvas->Scroll = value->GetNode()->GetGridPos() - NodeEditor::Get()->GetCanvasSize() / 2;
                        }
                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                        {
                            TRIGGER_TO_REMOVE = key;
                            UNLINK_TRIGGER = true;
                        }
                    }
                    ImGui::EndListBox();
                }
                ImGui::Separator();
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
            
            if (ImGui::BeginTabItem(MakeIdString("Preview Lua Code").c_str()))
            {
                m_LuaCodeEditor.Render("Lua Code");
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        if (UNLINK_TRIGGER)
        {
            std::string popupId = MakeIdString("Unlink Trigger");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                std::string removeId = MakeIdString("Remove Trigger") + EVENT_TO_REMOVE;
                if (ImGui::Button(removeId.c_str()))
                {
                    RemoveTrigger(TRIGGER_TO_REMOVE);
                    TRIGGER_TO_REMOVE = "";
                    UNLINK_TRIGGER = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    TRIGGER_TO_REMOVE = "";
                    UNLINK_TRIGGER = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        
        if (IS_ADD_TRIGGER_OPEN)
        {
            std::string popupId = MakeIdString("Add Trigger");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                static std::string key;
                ImGui::InputText(MakeIdString("ID").c_str(), &key);
                static std::string name;
                ImGui::InputText(MakeIdString("Name").c_str(), &name);
                if (const auto invalid = std::regex_search(key, FsmRegex::InvalidIdRegex());
                    !invalid && !key.empty() && !name.empty()
                    && !NodeEditor::Get()->GetCurrentFsm()->GetTrigger(key)
                    && !NodeEditor::Get()->GetCurrentFsm()->GetState(key)
                    && ImGui::Button(MakeIdString("Add Trigger").c_str()))
                {
                    auto newTrigger = AddTrigger(key);
                    newTrigger->SetName(name);
                    IS_ADD_TRIGGER_OPEN = false;
                    ImGui::SetClipboardText(newTrigger->GetLuaCode().c_str());
                    newTrigger->AppendToFile();
                    ImGui::InsertNotification({ImGuiToastType::Success, 3000, "Condition added: %s, code copied to clipboard", key.c_str()});
                    key = "";
                    name = "";
                    ImGui::CloseCurrentPopup();
                }
                else if (invalid)
                    ImGui::Text("Invalid ID");
                else if (key.empty())
                    ImGui::Text("ID can't be empty");
                else if (name.empty())
                    ImGui::Text("Name can't be empty");
                else if (NodeEditor::Get()->GetCurrentFsm()->GetTrigger(key))
                    ImGui::Text("Trigger with this ID already exists");
                else if (NodeEditor::Get()->GetCurrentFsm()->GetState(key))
                    ImGui::Text("State with this ID already exists");
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    IS_ADD_TRIGGER_OPEN = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        
        if (DELETE_STATE)
        {
            std::string popupId = MakeIdString("Delete State");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                if (ImGui::Button(MakeIdString("Are you sure?").c_str()))
                {
                    NodeEditor::Get()->GetCurrentFsm()->RemoveState(GetId());
                    DELETE_STATE = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    DELETE_STATE = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        if (SET_NEW_ID)
        {
            std::string popupId = MakeIdString("Refactor ID");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                static std::string newId;
                ImGui::InputText(MakeIdString("New ID").c_str(), &newId);
                if (const auto invalid = std::regex_search(newId, FsmRegex::InvalidIdRegex());
                    !invalid && !newId.empty()
                    && !NodeEditor::Get()->GetCurrentFsm()->GetTrigger(newId)
                    && !NodeEditor::Get()->GetCurrentFsm()->GetState(newId)
                    && ImGui::Button(MakeIdString("Refactor ID").c_str()))
                {
                    RefactorId(newId);
                    SET_NEW_ID = false;
                    newId = "";
                    ImGui::CloseCurrentPopup();
                }
                else if (invalid)
                    ImGui::Text("Invalid ID");
                else if (newId.empty())
                    ImGui::Text("ID can't be empty");
                else if (NodeEditor::Get()->GetCurrentFsm()->GetTrigger(newId))
                    ImGui::Text("Trigger with this ID already exists");
                else if (NodeEditor::Get()->GetCurrentFsm()->GetState(newId))
                    ImGui::Text("State with this ID already exists");
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    SET_NEW_ID = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
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
        }
    }
    
    std::string FsmState::GetLuaCode()
    {
        std::string code;
        code += fmt::format("---@FSM_STATE {0}\n", m_Id);
        code += fmt::format("---@class {0} : FSM_STATE\n", m_Id);
        code += fmt::format("local {0} = FSM_STATE:new({{}})\n", m_Id);
        code += fmt::format("{0}.name = \"{1}\"\n", m_Id, m_Name);
        code += fmt::format("{0}.description = \"{1}\"\n", m_Id, m_Description);
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
        for (const auto& value : m_Triggers | std::views::values)
        {
            code += value->GetLuaCode() + "\n";
        }
        return code;
    }
    
    
    VisualNode* FsmState::DrawNode()
    {
        return m_Node.Draw2(this);
    }
}
