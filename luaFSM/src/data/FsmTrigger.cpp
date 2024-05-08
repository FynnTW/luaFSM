#include "pch.h"
#include "FsmTrigger.h"

#include <spdlog/fmt/bundled/format.h>
#include <data/FsmState.h>

#include "imgui_internal.h"
#include "imgui/imgui_stdlib.h"
#include "imgui/NodeEditor.h"

#include "Graphics/Window.h"
#include "imgui/ImGuiNotify.hpp"
#include "IO/FileReader.h"

namespace LuaFsm
{
    FsmTrigger::FsmTrigger(const std::string& id)
    {
        FsmTrigger::SetId(id);
        //m_ConditionEditor.SetPalette(Window::GetPalette());
        m_ConditionEditor.SetText(m_Condition);
        m_ConditionEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        //m_OnTrueEditor.SetPalette(Window::GetPalette());
        m_OnTrueEditor.SetText(m_OnTrue);
        m_OnTrueEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        //m_OnFalseEditor.SetPalette(Window::GetPalette());
        m_OnFalseEditor.SetText(m_OnFalse);
        m_OnFalseEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        m_LuaCodeEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        m_Node.SetId(m_Id);
        m_Node.SetColor(IM_COL32(75, 75, 0, 150));
        m_Node.SetHighlightColor(m_Node.GetHighlightColor());
        m_Node.SetHighlightColorSelected(m_Node.GetHighlightColorSelected());
        m_Node.SetBorderColor(IM_COL32(255, 255, 255, 255));
        m_Node.SetType(NodeType::Transition);
        m_Node.SetShape(NodeShape::Circle);
    }

    void FsmTrigger::UpdateEditors()
    {
        m_ConditionEditor.SetText(m_Condition);
        m_OnTrueEditor.SetText(m_OnTrue);
        m_OnFalseEditor.SetText(m_OnFalse);
        m_LuaCodeEditor.SetText(GetLuaCode());
    }


    //std::string capturingRegex = "\\s*([\\s\\S]*?)end(?!\\s*end\\b)(?=(\\s*---@return|\\s*---@param|\\s*function|\\s*$))";
    std::string CAPTURING_REGEX = R"(\s*([\s\S]*?)\s*end---@endFunc)";

    void FsmTrigger::UpdateFromFile(const std::string& filePath)
    {
        const std::string code = FileReader::ReadAllText(filePath);
        if (code.empty())
            return;
        const std::string conditionFuncName = fmt::format("{0}:condition\\(\\)", m_Id);
        const std::regex conditionRegex(conditionFuncName + CAPTURING_REGEX);
        if (std::smatch conditionMatch; std::regex_search(code, conditionMatch, conditionRegex))
        {
            m_Condition = FileReader::RemoveStartingTab(conditionMatch[1].str());
        }
        const std::string onTrueFuncName = fmt::format("{0}:onTrue\\(\\)", m_Id);
        const std::regex onTrueRegex(onTrueFuncName + CAPTURING_REGEX);
        if (std::smatch onTrueMatch; std::regex_search(code, onTrueMatch, onTrueRegex))
        {
            m_OnTrue = FileReader::RemoveStartingTab(onTrueMatch[1].str());
        }
        const std::string onFalseFuncName = fmt::format("{0}:onFalse\\(\\)", m_Id);
        const std::regex onFalseRegex(onFalseFuncName + CAPTURING_REGEX);
        if (std::smatch onFalseMatch; std::regex_search(code, onFalseMatch, onFalseRegex))
        {
            m_OnFalse = FileReader::RemoveStartingTab(onFalseMatch[1].str());
        }
        UpdateEditors();
    }

    FsmState* FsmTrigger::GetCurrentState()
    {
        if (m_CurrentState)
            return m_CurrentState;
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
            return nullptr;
        const auto currentState = fsm->GetState(m_CurrentStateId);
        if (currentState)
        {
            m_CurrentState = currentState.get();
            return m_CurrentState;
        }
        return nullptr;
    }

    FsmState* FsmTrigger::GetNextState()
    {
        if (m_NextState)
            return m_NextState;
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
            return nullptr;
        if (const auto nextState = fsm->GetState(m_NextStateId))
        {
            m_NextState = nextState.get();
            return m_NextState;
        }
        return nullptr;
    }

    void FsmTrigger::SetNextState(const std::string& stateId)
    {
        this->m_NextStateId = stateId;
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
            return;
        if (fsm)
            m_NextState = fsm->GetState(stateId).get();
    }

    void FsmTrigger::SetCurrentState(const std::string& stateId)
    {
        this->m_CurrentStateId = stateId;
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
            return;
        if (fsm)
            m_CurrentState = fsm->GetState(stateId).get();
    }

    VisualNode* FsmTrigger::DrawNode()
    {
        return m_Node.Draw(this);
    }

    void FsmTrigger::SetId(const std::string& id)
    {
        const auto oldId = m_Id;
        m_Id = id;
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (fsm == nullptr)
            return;
        if (!oldId.empty())
        {
            for (const auto& [key, value] : fsm->GetStates())
            {
                value->ChangeTriggerId(oldId, id);
            }
            fsm->ChangeTriggerId(oldId, id);
        }
        m_Node.SetId(id);
    }

    const std::unordered_map<std::string, StateData>& FsmTrigger::GetData()
    {
        m_Data.clear();
        if (const auto currentState = GetCurrentState())
            m_Data = currentState->GetData();
        return m_Data;
    }

    std::string FsmTrigger::MakeIdString(const std::string& name) const
    {
        return name + "##" + m_Id;
    }

    bool UNLINK_CURRENT_STATE = false;
    bool UNLINK_NEXT_STATE = false;
    bool ADD_CURRENT_STATE = false;
    bool ADD_NEXT_STATE = false;
    bool DELETE_TRIGGER = false;

    void FsmTrigger::DrawProperties()
    {
        std::string id = GetId();
        if (ImGui::Button(MakeIdString("Refresh Code").c_str()) || m_LuaCodeEditor.GetText().empty())
            m_LuaCodeEditor.SetText(GetLuaCode());
        ImGui::SameLine();
        if (auto linkedFile = NodeEditor::Get()->GetCurrentFsm()->GetLinkedFile(); !linkedFile.empty()
            && ImGui::Button(MakeIdString("Refresh From File").c_str()))
            UpdateFromFile(linkedFile);
        if (ImGui::BeginTabBar(MakeIdString("Node Properties").c_str()))
        {
            if (ImGui::BeginTabItem(MakeIdString("Trigger Properties").c_str()))
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
                ImGui::InputTextMultiline(descriptionLabel.c_str(), &description, {ImGui::GetContentRegionAvail().x, 60});
                SetDescription(description);
                ImGui::Text("Priority");
                ImGui::SetNextItemWidth(150.f);
                ImGui::InputInt(MakeIdString("Priority").c_str(), &m_Priority);
                ImGui::Separator();
                ImGui::Text("Current State: ");
                ImGui::SameLine();
                if (GetCurrentState())
                {
                    ImGui::Selectable(MakeIdString(m_CurrentStateId).c_str(), false);
                    if (ImGui::IsItemClicked())
                        NodeEditor::Get()->SetSelectedNode(GetCurrentState()->GetNode());
                    if (ImGui::IsItemHovered())
                    {
                        GetCurrentState()->GetNode()->HighLight();
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                        {
                            UNLINK_CURRENT_STATE = true;
                        }
                    }
                }
                else if (ImGui::Button(MakeIdString("Add Current State").c_str()))
                    ADD_CURRENT_STATE = true;
                ImGui::Separator();
                ImGui::Text("Next State: ");
                ImGui::SameLine();
                if (GetNextState())
                {
                    ImGui::Selectable(MakeIdString(m_NextStateId).c_str(), false);
                    if (ImGui::IsItemClicked())
                        NodeEditor::Get()->SetSelectedNode(GetNextState()->GetNode());
                    if (ImGui::IsItemHovered())
                    {
                        GetNextState()->GetNode()->HighLight();
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                        {
                            UNLINK_NEXT_STATE = true;
                        }
                    }
                }
                else if (ImGui::Button(MakeIdString("Add Next State").c_str()))
                    ADD_NEXT_STATE = true;
                ImGui::Separator();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button(MakeIdString("Delete Trigger").c_str()))
                    DELETE_TRIGGER = true;
                ImGui::PopStyleColor();
                ImGui::Separator();
                ImGui::Text("Data");
                if (ImGui::BeginListBox(MakeIdString("Data").c_str(), {ImGui::GetContentRegionAvail().x, 100}))
                {
                    for (const auto& [key, data] : GetData())
                    {
                        std::string text = key;
                        if (!data.value.empty())
                        {
                            if (data.type == "string")
                                text += " (\"" + data.value + "\")";
                            else
                                text += " (" + data.value + ")";
                        }
                        text += " : " + data.type;
                        ImGui::Selectable(MakeIdString(text).c_str(), false);
                        if (ImGui::IsItemHovered())
                        {
                            if (ImGui::BeginTooltip())
                            {
                                if (!data.comment.empty())
                                    ImGui::Text(data.comment.c_str());
                                ImGui::EndTooltip();
                            }
                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                            {
                                std::string copyText = "self.data." + key;
                                ImGui::SetClipboardText(copyText.c_str());
                            }
                        }
                    }
                    ImGui::EndListBox();
                }
                ImGui::Separator();
                ImGui::Text("Condition");
                ImGui::Separator();
                Window::DrawTextEditor(m_ConditionEditor, m_Condition);
                ImGui::Separator();
                
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("OnTrue").c_str()))
            {
                Window::DrawTextEditor(m_OnTrueEditor, m_OnTrue);
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("OnFalse").c_str()))
            {
                Window::DrawTextEditor(m_OnFalseEditor, m_OnFalse);
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("Lua Code").c_str()))
            {
                m_LuaCodeEditor.Render("Lua Code");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        if (id != GetId())
        {
            SetId(id);
        }

        if (DELETE_TRIGGER)
        {
            std::string popupId = MakeIdString("Delete Trigger");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                if (ImGui::Button(MakeIdString("Are you sure?").c_str()))
                {
                    NodeEditor::Get()->GetCurrentFsm()->RemoveTrigger(GetId());
                    DELETE_TRIGGER = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Cancel").c_str()))
                {
                    DELETE_TRIGGER = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        if (UNLINK_CURRENT_STATE)
        {
            std::string popupId = MakeIdString("Unlink Current State");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                if (ImGui::Button(MakeIdString("Unlink").c_str()))
                {
                    if (auto state = GetCurrentState())
                        state->RemoveTrigger(GetId());
                    UNLINK_CURRENT_STATE = false;
                    SetCurrentState("");
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Cancel").c_str()))
                {
                    UNLINK_CURRENT_STATE = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        if (UNLINK_NEXT_STATE)
        {
            std::string popupId = MakeIdString("Unlink Next State");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                if (ImGui::Button(MakeIdString("Unlink").c_str()))
                {
                    UNLINK_NEXT_STATE = false;
                    SetNextState("");
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Cancel").c_str()))
                {
                    UNLINK_NEXT_STATE = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        if (ADD_CURRENT_STATE)
        {
            std::string popupId = MakeIdString("Add Current State");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                static std::string key;
                ImGui::InputText(MakeIdString("id").c_str(), &key);
                static std::string name;
                ImGui::InputText(MakeIdString("name").c_str(), &name);
                if (!key.empty() && !name.empty() &&
                    !NodeEditor::Get()->GetCurrentFsm()->GetState(key)
                    && ImGui::Button(MakeIdString("Add State").c_str()))
                {
                    auto newState = NodeEditor::Get()->GetCurrentFsm()->AddState(key);
                    NodeEditor::Get()->GetCurrentFsm()->GetTrigger(m_Id)->SetCurrentState(newState->GetId());
                    newState->SetName(name);
                    ADD_CURRENT_STATE = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Cancel").c_str()))
                {
                    ADD_CURRENT_STATE = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        if (ADD_NEXT_STATE)
        {
            std::string popupId = MakeIdString("Add Current State");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                static std::string key;
                ImGui::InputText(MakeIdString("id").c_str(), &key);
                static std::string name;
                ImGui::InputText(MakeIdString("name").c_str(), &name);
                if (!key.empty() && !name.empty() &&
                    !NodeEditor::Get()->GetCurrentFsm()->GetState(key) &&
                    ImGui::Button(MakeIdString("Add State").c_str()))
                {
                    ADD_NEXT_STATE = false;
                    auto newState = NodeEditor::Get()->GetCurrentFsm()->AddState(key);
                    key = "";
                    const auto trigger = NodeEditor::Get()->GetCurrentFsm()->GetTrigger(GetId());
                    trigger->SetNextState(newState->GetId());
                    newState->SetName(name);
                    name = "";
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Cancel").c_str()))
                {
                    ADD_NEXT_STATE = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }

    std::shared_ptr<FsmTrigger> FsmTrigger::Deserialize(const nlohmann::json& json)
    {
        auto trigger = std::make_shared<FsmTrigger>(json["id"].get<std::string>());
        trigger->SetName(json["name"].get<std::string>());
        trigger->SetDescription(json["description"].get<std::string>());
        trigger->SetPriority(json["priority"].get<int>());
        trigger->SetNextState(json["nextStateId"].get<std::string>());
        trigger->SetCurrentState(json["currentStateId"].get<std::string>());
        trigger->SetCondition(json["condition"].get<std::string>());
        trigger->SetOnTrue(json["onTrue"].get<std::string>());
        trigger->SetOnFalse(json["onFalse"].get<std::string>());
        trigger->GetNode()->SetTargetPosition({json["positionX"].get<float>(), json["positionY"].get<float>()});
        trigger->UpdateEditors();
        return trigger;
    }

    nlohmann::json FsmTrigger::Serialize() const
    {
        nlohmann::json j;
        j["id"] = m_Id;
        j["name"] = m_Name;
        j["description"] = m_Description;
        j["priority"] = m_Priority;
        j["nextStateId"] = m_NextStateId;
        j["currentStateId"] = m_CurrentStateId;
        j["condition"] = m_Condition;
        j["onTrue"] = m_OnTrue;
        j["onFalse"] = m_OnFalse;
        j["positionX"] = m_Node.GetTargetPosition().x;
        j["positionY"] = m_Node.GetTargetPosition().y;
        return j;
    }


    std::string FsmTrigger::GetLuaCode(int indent)
    {
        std::string indentTabs;
        for (int i = 0; i < indent; i++)
            indentTabs += "\t";
        std::string code;
        code += indentTabs + fmt::format("\n---------------------------------------------------------------------------\n");
        code += indentTabs + fmt::format("--------------------------- <FSM TRIGGER> ---------------------------------\n");
        code += indentTabs + fmt::format("---------------------------------------------------------------------------\n");
        code += indentTabs + fmt::format("\n---@type FSM_TRIGGER\n");
        code += indentTabs + fmt::format("local {0} = FSM_TRIGGER:new({{\n", m_Id);
        code += indentTabs + fmt::format("\t---Unique Identifier of this Trigger\n");
        code += indentTabs + fmt::format("\t---@type string\n");
        code += indentTabs + fmt::format("\tid = \"{0}\",\n", m_Id);
        code += indentTabs + fmt::format("\t---Name of this Trigger\n");
        code += indentTabs + fmt::format("\t---@type string\n");
        code += indentTabs + fmt::format("\tname = \"{0}\",\n", m_Name);
        code += indentTabs + fmt::format("\t---Description of this Trigger\n");
        code += indentTabs + fmt::format("\t---@type string\n");
        code += indentTabs + fmt::format("\tdescription = \"{0}\",\n", m_Description);
        code += indentTabs + fmt::format("\t---Priority of this Trigger\n");
        code += indentTabs + fmt::format("\t---@type integer\n");
        code += indentTabs + fmt::format("\tpriority = {0},\n", m_Priority);
        code += indentTabs + fmt::format("\t---Name of the state this trigger leads to\n");
        code += indentTabs + fmt::format("\t---@type string\n");
        code += indentTabs + fmt::format("\tnextStateId = \"{0}\",\n", m_NextStateId);
        code += indentTabs + fmt::format("\t---Data the trigger inherits\n");
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
        code += indentTabs + fmt::format("}})\n");
        code += indentTabs + fmt::format("\n---@return boolean isTrue\n");
        code += indentTabs + fmt::format("function {0}:condition()", m_Id);
        if (m_Condition.empty())
            code += indentTabs + fmt::format(" ");
        else
        {
            code += indentTabs + fmt::format("\n");
            for (const auto& line : m_ConditionEditor.GetTextLines())
                code += indentTabs + fmt::format("\t{0}\n", line);
        }
        code += indentTabs + fmt::format("end---@endFunc\n");
        code += indentTabs + fmt::format("\nfunction {0}:onTrue()", m_Id);
        if (m_OnTrue.empty())
            code += indentTabs + fmt::format(" ");
        else
        {
            code += indentTabs + fmt::format("\n");
            for (const auto& line : m_OnTrueEditor.GetTextLines())
                code += indentTabs + fmt::format("\t{0}\n", line);
        }
        code += indentTabs + fmt::format("end---@endFunc\n");
        code += indentTabs + fmt::format("\nfunction {0}:onFalse()", m_Id);
        if (m_OnFalse.empty())
            code += indentTabs + fmt::format(" ");
        else
        {
            code += indentTabs + fmt::format("\n");
            for (const auto& line : m_OnFalseEditor.GetTextLines())
                code += indentTabs + fmt::format("\t{0}\n", line);
        }
        code += indentTabs + fmt::format("end---@endFunc\n");
        return code;
    }
}
