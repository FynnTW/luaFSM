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

    std::unordered_map<std::string, StateData> FsmState::GetData()
    {
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

    nlohmann::json FsmState::Serialize()
    {
        nlohmann::json j;
        j["id"] = m_Id;
        j["name"] = m_Name;
        j["description"] = m_Description;
        j["onInit"] = m_OnInit;
        j["onEnter"] = m_OnEnter;
        j["onUpdate"] = m_OnUpdate;
        j["onExit"] = m_OnExit;
        nlohmann::json data;
        for (auto& [key, value] : m_Data)
            data[key] = value.Serialize();
        j["data"] = data;
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
            state->AddData(key, StateData::Deserialize(value));
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
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
            return;
        fsm->AddTrigger(value);
    }


    FsmTriggerPtr FsmState::AddTrigger(const std::string& key)
    {
        m_Triggers[key] = std::make_shared<FsmTrigger>(key);
        auto& value = m_Triggers[key]; // Reference to the shared pointer
        value->SetCurrentState(m_Id);
        NodeEditor::Get()->GetCurrentFsm()->AddTrigger(value);
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
    
    void FsmState::DrawProperties()
    {
        std::string id = GetId();
        if (ImGui::Button(MakeIdString("Refresh").c_str()) || m_LuaCodeEditor.GetText().empty())
            m_LuaCodeEditor.SetText(GetLuaCode());
        if (ImGui::BeginTabBar(MakeIdString("Node Properties").c_str()))
        {
            if (ImGui::BeginTabItem(MakeIdString("Properties").c_str()))
            {
                ImGui::Text("ID");
                std::string idLabel = "##ID" + GetId();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText(idLabel.c_str(), &id);
                ImGui::Text("Name");
                std::string name = GetName();
                std::string nameLabel = "##Name" + GetId();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText(nameLabel.c_str(), &name);
                SetName(name);
                ImGui::Text("Description");
                std::string description = GetDescription();
                std::string descriptionLabel = "##Description" + GetId();
                ImGui::InputTextMultiline(descriptionLabel.c_str(), &description, {ImGui::GetContentRegionAvail().x, 100});
                 SetDescription(description);
                ImGui::Text("Events");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
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
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button(MakeIdString("Delete State").c_str()))
                    DELETE_STATE = true;
                ImGui::PopStyleColor();
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Set as initial").c_str()))
                    NodeEditor::Get()->GetCurrentFsm()->SetInitialState(GetId());
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(MakeIdString("OnUpdate").c_str()))
            {
                ImGui::Text("Data");
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Add Data").c_str()))
                {
                    IS_ADD_DATA_OPEN = true;
                }
                if (ImGui::BeginTable(MakeIdString("Arguments").c_str(), 6,
                    ImGuiTableFlags_BordersInnerV
                    | ImGuiTableFlags_BordersOuter
                    | ImGuiTableFlags_SizingFixedFit
                    | ImGuiTableFlags_Resizable
                    | ImGuiTableFlags_Reorderable
                    | ImGuiTableFlags_RowBg))
                {
                    int index = 0;
                    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 200.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 200.0f);
                    ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Edit Description", ImGuiTableColumnFlags_WidthFixed, 160.f);
                    ImGui::TableSetupColumn("Copy", ImGuiTableColumnFlags_WidthFixed, 60.f);
                    ImGui::TableSetupColumn("Remove", ImGuiTableColumnFlags_WidthFixed, 40.f);
                    
                    for (auto& [key, stateData] : GetData())
                    {
                        if (!tempData.contains(key))
                            tempData[key] = stateData;
                        ImGui::TableNextRow();
                        std::string keyId = "##Key" + std::to_string(index) + GetId();
                        std::string valueId = "##Value" + std::to_string(index) + GetId();
                        std::string typeId = "##Type" + std::to_string(index) + GetId();
                        
                        // Key column
                        ImGui::TableSetColumnIndex(0);
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::InputText(keyId.c_str(), &tempData[key].name);

                        // Value column
                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::InputText(valueId.c_str(), &tempData[key].value);

                        // Type column
                        ImGui::TableSetColumnIndex(2);
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::InputText(typeId.c_str(), &tempData[key].type);

                        ImGui::TableSetColumnIndex(3);
                        std::string editId = "Edit Description## " + key + std::to_string(index) + GetId();
                        if (ImGui::Button(editId.c_str()))
                        {
                            EDIT_DATA_DESCR = true;
                            dataKey = key;
                        }

                        ImGui::TableSetColumnIndex(4);
                        std::string removeId = "Remove## " + key + std::to_string(index) + GetId();
                        if (ImGui::Button(removeId.c_str()))
                        {
                            m_Data.erase(key);
                            break;
                        }
                        
                        ImGui::TableSetColumnIndex(5);
                        std::string copyId = "Copy## " + key + std::to_string(index) + GetId();
                        if (ImGui::Button(copyId.c_str()))
                        {
                            std::string copyString = "self.data." + key;
                            ImGui::SetClipboardText(copyString.c_str());
                        }
                        index++;
                    }
                    ImGui::EndTable();
                }
                for (auto& [key, stateData] : tempData)
                {
                    if (stateData.name != key)
                    {
                        if (m_Data.contains(key))
                            m_Data.erase(key);
                        m_Data[stateData.name] = stateData;
                    }
                    else if (m_Data.contains(key))
                        m_Data[key] = stateData;
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

        if (EDIT_DATA_DESCR)
        {
            std::string popupId = MakeIdString("Edit Description");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                static std::string descr = GetData()[dataKey].comment;
                ImGui::InputTextMultiline("Description", &descr, {300, 50});
                if (ImGui::Button("Change Description"))
                {
                    EDIT_DATA_DESCR = false;
                    GetData()[dataKey].comment = descr;
                    dataKey = "";
                    descr = "";
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    EDIT_DATA_DESCR = false;
                    dataKey = "";
                    descr = "";
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        if (IS_ADD_DATA_OPEN)
        {
            std::string popupId = MakeIdString("Add Argument");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                static std::string key;
                static std::string value = "nil";
                static std::string type;
                static std::string comment;
                ImGui::InputText(MakeIdString("Name").c_str(), &key);
                ImGui::InputText(MakeIdString("Value").c_str(), &value);
                ImGui::InputText(MakeIdString("Type").c_str(), &type);
                ImGui::InputTextMultiline("Description", &comment, {300, 50});
                if (!m_Data.contains(key) && ImGui::Button(MakeIdString("Add Data").c_str()))
                {
                    if (value.empty() || value == "nil")
                    {
                        if (type == "number")
                            value = "0";
                        else if (type == "string")
                            value = "";
                        else if (type == "integer")
                            value = "0";
                        else if (type == "boolean")
                            value = "false";
                        else if (std::regex_match(value, std::regex("table")))
                            value = "{}";
                        else
                            value = "nil";
                    }
                    AddData(key, {key, value, type, comment});
                    IS_ADD_DATA_OPEN = false;
                    key = "";
                    value = "";
                    type = "any";
                    comment = "";
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Cancel").c_str()))
                {
                    IS_ADD_DATA_OPEN = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
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
                ImGui::SameLine();
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
                ImGui::SameLine();
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
        code += indentTabs + fmt::format("\n---------------------------------------------------------------------------\n");
        code += indentTabs + fmt::format("----------------------------- <FSM STATE> ---------------------------------\n");
        code += indentTabs + fmt::format("---------------------------------------------------------------------------\n");
        code += indentTabs + fmt::format("\n---@type FSM_STATE\n");
        code += indentTabs + fmt::format("local {0} = FSM_STATE:new({{\n", m_Id);
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
        for (const auto& [key, data] : m_Data)
        {
            if (!data.comment.empty())
                code += indentTabs + fmt::format("\t\t---{0}\n", data.comment);
            code += indentTabs + fmt::format("\t\t---@type {0}\n", data.type);
            if (data.type == "string")
                code += indentTabs + fmt::format("\t\t{0} = \"{1}\",\n", key, data.value);
            else
            {
                code += indentTabs + fmt::format("\t\t{0} = {1},\n", key, data.value);
            }
        }
        code += indentTabs + fmt::format("\t}},\n");
        code += indentTabs + fmt::format("\t---Conditions for moving to other states\n");
        code += indentTabs + fmt::format("\t---@type table<string, FSM_TRIGGER>\n");
        code += indentTabs + fmt::format("\ttriggers = {{\n");
        code += indentTabs + fmt::format("\t}},\n");
        code += indentTabs + fmt::format("}})\n");
        code += indentTabs + fmt::format("\nfunction {0}:onInit()", m_Id);
        if (m_OnInit.empty())
            code += indentTabs + fmt::format(" ");
        else
        {
            code += indentTabs + fmt::format("\n");
            for (const auto& line : m_OnInitEditor.GetTextLines())
                code += indentTabs + fmt::format("\t{0}\n", line);
        }
        code += indentTabs + fmt::format("end\n");
        for (const auto& [key, value] : m_OnUpdateArguments)
            code += indentTabs + fmt::format("\n---@param {0} {1}\n", key, value);
        code += indentTabs + fmt::format("function {0}:onUpdate(", m_Id);
        int argNum = 0;
        for (const auto& [key, value] : m_OnUpdateArguments)
        {
            if (argNum++ > 0)
                code += ", ";
            code += fmt::format("{0}", key);
        }
        code += fmt::format(")");
        if (m_OnUpdate.empty())
            code += indentTabs + fmt::format(" ");
        else
        {
            code += indentTabs + fmt::format("\n");
            for (const auto& line : m_OnUpdateEditor.GetTextLines())
                code += indentTabs + fmt::format("\t{0}\n", line);
        }
        code += indentTabs + fmt::format("end\n");
        code += indentTabs + fmt::format("\nfunction {0}:onEnter()", m_Id);
        if (m_OnEnter.empty())
            code += indentTabs + fmt::format(" ");
        else
        {
            code += indentTabs + fmt::format("\n");
            for (const auto& line : m_OnEnterEditor.GetTextLines())
                code += indentTabs + fmt::format("\t{0}\n", line);
        }
        code += indentTabs + fmt::format("end\n");
        code += indentTabs + fmt::format("\nfunction {0}:onExit()", m_Id);
        if (m_OnExit.empty())
            code += indentTabs + fmt::format(" ");
        else
        {
            code += indentTabs + fmt::format("\n");
            for (const auto& line : m_OnExitEditor.GetTextLines())
                code += indentTabs + fmt::format("\t{0}\n", line);
        }
        code += indentTabs + fmt::format("end\n");
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
