// ReSharper disable StringLiteralTypo
#include "pch.h"
#include "FsmState.h"

#include <spdlog/fmt/bundled/format.h>
#include "Graphics/Math.h"
#include "imgui/imgui_stdlib.h"

#include "Log.h"
#include "Graphics/Window.h"
#include "imgui/NodeEditor.h"

namespace LuaFsm
{
    FsmState::FsmState(const std::string& id)
    {
        FsmState::SetId(id);
        m_OnInitEditor.SetText(m_OnInit);
        m_OnInitEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
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
        m_OnUpdateArguments["eventData"] = "eventTrigger";
    }

    std::unordered_map<std::string, std::string> FsmState::GetData()
    {
        for (const auto& [key, trigger] : m_Triggers)
            for (const auto& [argKey, argValue] : trigger->GetArguments())
                m_Data[argKey] = "";
        return m_Data;
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

    nlohmann::json FsmState::Serialize() const
    {
        nlohmann::json j;
        j["id"] = m_Id;
        j["name"] = m_Name;
        j["description"] = m_Description;
        j["onInit"] = m_OnInit;
        j["onEnter"] = m_OnEnter;
        j["onUpdate"] = m_OnUpdate;
        j["onExit"] = m_OnExit;
        j["data"] = m_Data;
        j["positionX"] = m_Node.GetTargetPosition().x;
        j["positionY"] = m_Node.GetTargetPosition().y;
        j["onUpdateArguments"] = m_OnUpdateArguments;
        j["events"] = m_Events;
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
        state->SetOnInit(json["onInit"].get<std::string>());
        state->SetOnEnter(json["onEnter"].get<std::string>());
        state->SetOnUpdate(json["onUpdate"].get<std::string>());
        state->SetOnExit(json["onExit"].get<std::string>());
        for (const auto& [key, value] : json["data"].items())
            state->AddData(key, value.get<std::string>());
        for (const auto& [key, value] : json["onUpdateArguments"].items())
            state->AddOnUpdateArgument(key, value.get<std::string>());
        for (const auto& event : json["events"])
            state->AddEvent(event);
        state->GetNode()->SetTargetPosition({json["positionX"].get<float>(), json["positionY"].get<float>()});
        return state;
    }

    void FsmState::AddTrigger(const std::string& key, const FsmTriggerPtr& value)
    {
        m_Triggers[key] = value;
        value->SetCurrentState(m_Id);
        for (const auto& [argKey, argValue] : value->GetArguments())
            AddData(argKey, "");
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
            return;
        fsm->AddTrigger(value);
    }


    FsmTriggerPtr FsmState::AddTrigger(const std::string& key)
    {
        m_Triggers[key] = std::make_shared<FsmTrigger>(key);
        auto& value = m_Triggers[key]; // Reference to the shared pointer
        std::cout << "Creating FsmTrigger with key: " << key << " at address: " << value.get() << std::endl;
        value->SetCurrentState(m_Id);
        for (const auto& [argKey, argValue] : value->GetArguments())
            AddData(argKey, "");
        NodeEditor::Get()->GetCurrentFsm()->AddTrigger(value);
        return value;
    }

    bool IS_ADD_EVENT_OPEN = false;
    bool IS_ADD_TRIGGER_OPEN = false;
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
    bool UNLINK_TRIGGER = false;
    bool DELETE_STATE = false;
    
    void FsmState::DrawProperties()
    {
        std::string id = GetId();
        if (ImGui::Button(MakeIdString("Refresh").c_str()) || m_LuaCodeEditor.GetText().empty())
            m_LuaCodeEditor.SetText(GetLuaCode());
        if (ImGui::BeginTabBar(MakeIdString("Node Properties").c_str()))
        {
            if (ImGui::BeginTabItem(MakeIdString("Properties").c_str()))
            {
                if (ImGui::Button(MakeIdString("Delete State").c_str()))
                    DELETE_STATE = true;
                if (ImGui::Button(MakeIdString("Set as initial").c_str()))
                    NodeEditor::Get()->GetCurrentFsm()->SetInitialState(GetId());
                ImGui::Text("ID");
                std::string idLabel = "##ID" + GetId();
                ImGui::InputText(idLabel.c_str(), &id);
                ImGui::Text("Name");
                std::string name = GetName();
                std::string nameLabel = "##Name" + GetId();
                ImGui::InputText(nameLabel.c_str(), &name);
                SetName(name);
                ImGui::Text("Description");
                std::string description = GetDescription();
                std::string descriptionLabel = "##Description" + GetId();
                ImGui::InputText(descriptionLabel.c_str(), &description);
                SetDescription(description);
                ImGui::Text("Events");
                ImGui::InputText(MakeIdString("New Event").c_str(), &NEW_EVENT);
                if (!NEW_EVENT.empty() && ImGui::Button(MakeIdString("Add Event").c_str()))
                {
                    AddEvent(NEW_EVENT);
                    NEW_EVENT = "";
                }
                if (ImGui::BeginListBox(MakeIdString("Events").c_str(),{300, 75 }))
                {
                    for (const auto& event : GetEvents())
                    {
                        ImGui::Selectable(MakeIdString(event).c_str(), false);
                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                        {
                            OPEN_EVENT_POPUP = true;
                            EVENT_TO_REMOVE = event;
                        }
                    }
                    ImGui::EndListBox();
                }
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
                            NodeEditor::Get()->SetSelectedNode(value->GetNode());
                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                        {
                            TRIGGER_TO_REMOVE = key;
                            UNLINK_TRIGGER = true;
                        }
                    }
                    ImGui::EndListBox();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(MakeIdString("OnUpdate").c_str()))
            {
                ImGui::Text("Data");
                if (ImGui::BeginListBox(MakeIdString("Data").c_str(),{200, 100 }))
                {
                    
                    for (const auto& [key, value] : GetData())
                    {
                        ImGui::Selectable(MakeIdString(key).c_str(), false);
                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        {
                            std::string text = "self.data." + key;
                            ImGui::SetClipboardText(text.c_str());
                        }
                    }
                    ImGui::EndListBox();
                }
                ImGui::Text("Arguments");
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Add Argument").c_str()))
                    IS_ADD_STATE_ARGUMENT_OPEN = true;
                if (ImGui::BeginListBox(MakeIdString("Arguments").c_str(),{200, 50 }))
                {
                    
                    for (const auto& [key, value] : GetOnUpdateArguments())
                    {
                        ImGui::Selectable(MakeIdString(key).c_str(), false);
                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        {
                            std::string text = key;
                            ImGui::SetClipboardText(text.c_str());
                        }
                        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                        {
                            EDITED_ARGUMENT = {key, value};
                            IS_EDIT_STATE_ARGUMENT_OPEN = true;
                        }
                    }
                    ImGui::EndListBox();
                }
                ImGui::Text("OnUpdate:");
                ImGui::Separator();
                Window::DrawTextEditor(m_OnUpdateEditor, m_OnUpdate);
                ImGui::Separator();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("OnInit").c_str()))
            {
                Window::DrawTextEditor(m_OnInitEditor, m_OnInit);
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("OnEnter").c_str()))
            {
                Window::DrawTextEditor(m_OnEnterEditor, m_OnEnter);
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("OnExit").c_str()))
            {
                Window::DrawTextEditor(m_OnExitEditor, m_OnExit);
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("Lua Code").c_str()))
            {
                m_LuaCodeEditor.Render("Lua Code");
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        if (IS_ADD_STATE_ARGUMENT_OPEN)
        {
            std::string popupId = MakeIdString("Add OnUpdate Argument");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                static std::string key;
                static std::string value = "any";
                ImGui::InputText(MakeIdString("Name").c_str(), &key);
                ImGui::InputText(MakeIdString("Type").c_str(), &value);
                if (!key.empty() && !value.empty() && ImGui::Button(MakeIdString("Add Argument").c_str()))
                {
                    AddOnUpdateArgument(key, value);
                    IS_ADD_STATE_ARGUMENT_OPEN = false;
                    key = "";
                    value = "any";
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    IS_ADD_STATE_ARGUMENT_OPEN = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        if (IS_EDIT_STATE_ARGUMENT_OPEN)
        {
            std::string popupId = MakeIdString("Edit Argument") + EDITED_ARGUMENT.first;
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                static std::string key = EDITED_ARGUMENT.first;
                static std::string value = EDITED_ARGUMENT.second;
                std::string keyId = MakeIdString("Name") + EDITED_ARGUMENT.first;
                std::string valueId = MakeIdString("Type") + EDITED_ARGUMENT.first;
                ImGui::InputText(keyId.c_str(), &key);
                ImGui::InputText(valueId.c_str(), &value);
                if (!key.empty() && !value.empty() && ImGui::Button(MakeIdString("Change Argument").c_str()))
                {
                    RemoveOnUpdateArgument(EDITED_ARGUMENT.first);
                    AddOnUpdateArgument(key, value);
                    IS_EDIT_STATE_ARGUMENT_OPEN = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Remove Argument").c_str()))
                {
                    RemoveOnUpdateArgument(EDITED_ARGUMENT.first);
                    IS_EDIT_STATE_ARGUMENT_OPEN = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    IS_EDIT_STATE_ARGUMENT_OPEN = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        if (OPEN_EVENT_POPUP)
        {
            std::string popupId = MakeIdString("Edit Event") + EVENT_TO_REMOVE;
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                std::string removeId = MakeIdString("Remove Event") + EVENT_TO_REMOVE;
                if (ImGui::Button(removeId.c_str()))
                {
                    RemoveEvent(EVENT_TO_REMOVE);
                    EVENT_TO_REMOVE = "";
                    OPEN_EVENT_POPUP = false;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::Button("Cancel"))
                {
                    EVENT_TO_REMOVE = "";
                    OPEN_EVENT_POPUP = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
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
                if (!key.empty() && !name.empty() && !NodeEditor::Get()->GetCurrentFsm()->GetTrigger(id) && ImGui::Button(MakeIdString("Add Trigger").c_str()))
                {
                    auto newTrigger = AddTrigger(key);
                    newTrigger->SetName(name);
                    IS_ADD_TRIGGER_OPEN = false;
                    key = "";
                    ImGui::CloseCurrentPopup();
                }
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
                if (ImGui::Button("Cancel"))
                {
                    DELETE_STATE = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        if (id != GetId())
        {
            SetId(id);
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

    std::string FsmState::GetLuaCode(int indent)
    {
        std::string indentTabs;
        for (int i = 0; i < indent; i++)
            indentTabs += "\t";
        std::string code;
        code += indentTabs + fmt::format("{0} = FSM_STATE:new({{\n", m_Id);
        code += indentTabs + fmt::format("\t---Unique Identifier of this State\n");
        code += indentTabs + fmt::format("\t---@type string\n");
        code += indentTabs + fmt::format("\tid = \"{0}\",\n", m_Id);
        code += indentTabs + fmt::format("\t---Name of this State\n");
        code += indentTabs + fmt::format("\t---@type string\n");
        code += indentTabs + fmt::format("\tname = \"{0}\",\n", m_Name);
        code += indentTabs + fmt::format("\t---Description of this State\n");
        code += indentTabs + fmt::format("\t---@type string\n");
        code += indentTabs + fmt::format("\tdescription = \"{0}\",\n", m_Description);
        code += indentTabs + fmt::format("\t---Events where the state updates\n");
        code += indentTabs + fmt::format("\t---@type table<string>\n");
        code += indentTabs + fmt::format("\tevents = {{\n");
        for (const auto& event : m_Events)
            code += indentTabs + fmt::format("\t\t\"{0}\",\n", event);
        code += indentTabs + fmt::format("\t}},\n");
        code += indentTabs + fmt::format("\t---Data the state contains\n");
        code += indentTabs + fmt::format("\t---@type table<string, any>\n");
        code += indentTabs + fmt::format("\tdata = {{\n");
        for (const auto& [key, value] : m_Data)
            code += indentTabs + fmt::format("\t\t{0} = \"{1}\",\n", key, value);
        code += indentTabs + fmt::format("\t}},\n");
        code += indentTabs + fmt::format("\t---@param self FSM_STATE\n");
        code += indentTabs + fmt::format("\tonInit = function(self)\n");
        for (const auto& line : m_OnInitEditor.GetTextLines())
            code += indentTabs + fmt::format("\t\t{0}\n", line);
        code += indentTabs + fmt::format("\tend,\n");
        code += indentTabs + fmt::format("\t---@param self FSM_STATE\n");
        code += indentTabs + fmt::format("\tonEnter = function(self)\n");
        for (const auto& line : m_OnEnterEditor.GetTextLines())
            code += indentTabs + fmt::format("\t\t{0}\n", line);
        code += indentTabs + fmt::format("\tend,\n");
        code += indentTabs + fmt::format("\t---@param self FSM_STATE\n");
        for (const auto& [key, value] : m_OnUpdateArguments)
            code += indentTabs + fmt::format("\t---@param {0} {1}\n", key, value);
        code += indentTabs + fmt::format("\tonUpdate = function(self");
        for (const auto& [key, value] : m_OnUpdateArguments)
            code += fmt::format(", {0}", key);
        code += fmt::format(")\n");
        for (const auto& line : m_OnUpdateEditor.GetTextLines())
            code += indentTabs + fmt::format("\t\t{0}\n", line);
        code += indentTabs + fmt::format("\tend,\n");
        code += indentTabs + fmt::format("\t---@param self FSM_STATE\n");
        code += indentTabs + fmt::format("\tonExit = function(self)\n");
        for (const auto& line : m_OnExitEditor.GetTextLines())
            code += indentTabs + fmt::format("\t\t{0}\n", line);
        code += indentTabs + fmt::format("\tend,\n");
        code += indentTabs + fmt::format("\t---Conditions for moving to other states\n");
        code += indentTabs + fmt::format("\t---@type table<string, FSM_TRIGGER>\n");
        code += indentTabs + fmt::format("\ttriggers = {{\n");
        code += indentTabs + fmt::format("\t}},\n");
        code += indentTabs + fmt::format("}})\n");
        for (const auto& [key, value] : m_Triggers)
        {
            code += value->GetLuaCode() + "\n";
            code += fmt::format("{0}:registerTrigger({1})\n", m_Id, key);
        }
        return code;
    }
    
    
    VisualNode* FsmState::DrawNode()
    {
        return m_Node.Draw(this);
    }
}
