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
        m_ActionEditor.SetText(m_Action);
        m_ActionEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
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
        Window::TrimTrailingNewlines(m_Condition);
        m_ConditionEditor.SetText(m_Condition);
        Window::TrimTrailingNewlines(m_Action);
        m_ActionEditor.SetText(m_Action);
        m_LuaCodeEditor.SetText(GetLuaCode());
    }

    std::vector<std::shared_ptr<FsmTrigger>> FsmTrigger::CreateFromFile(const std::string& filePath)
    {
        auto conditions = std::vector<std::shared_ptr<FsmTrigger>>{};
        const std::string code = FileReader::ReadAllText(filePath);
        if (code.empty())
            return conditions;
        std::regex idRegex = FsmRegex::IdRegex("FSM_CONDITION");
        std::sregex_iterator iter(code.begin(), code.end(), idRegex);
        for (std::sregex_iterator end; iter != end; ++iter)
        {
            std::string id = (*iter)[1].str();
            auto condition = std::make_shared<FsmTrigger>(id);
            conditions.emplace_back(condition);
        }
        return conditions;
    }

    std::string FsmTrigger::GetLuaCode()
    {
        std::string code;
        code += fmt::format("---@FSM_CONDITION {0}\n", m_Id);
        code += fmt::format("---@class {0} : FSM_CONDITION\n", m_Id);
        code += fmt::format("local {0} = FSM_CONDITION:new({{}})\n", m_Id);
        code += fmt::format("{0}.name = \"{1}\"\n", m_Id, m_Name);
        code += fmt::format("{0}.description = \"{1}\"\n", m_Id, m_Description);
        code += fmt::format("{0}.editorPos = {{{1}, {2}}}\n", m_Id, m_Node.GetTargetPosition().x, m_Node.GetTargetPosition().y);
        code += fmt::format("{0}.currentStateId = \"{1}\"\n", m_Id, m_CurrentStateId);
        code += fmt::format("{0}.nextStateId = \"{1}\"\n", m_Id, m_NextStateId);
        code += fmt::format("{0}.priority = {1}\n", m_Id, m_Priority);
        if (!m_Condition.empty())
        {
            code += fmt::format("\n---@return boolean isTrue\n");
            code += fmt::format("function {0}:condition()\n", m_Id);
            for (const auto& line : m_ConditionEditor.GetTextLines())
                code += fmt::format("\t{0}\n", line);
            code += fmt::format("end---@endFunc\n");
        }
        if (!m_Action.empty())
        {
            code += fmt::format("\nfunction {0}:action()\n", m_Id);
            for (const auto& line : m_ActionEditor.GetTextLines())
                code += fmt::format("\t{0}\n", line);
            code += fmt::format("end---@endFunc\n");
        }
        return code;
    }

    void FsmTrigger::UpdateFileContents(std::string& code)
    {
        UpdateEditors();
        std::regex regex = FsmRegex::ClassStringRegex(m_Id, "name");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.name = \"{1}\"", m_Id, m_Name));
        regex = FsmRegex::ClassStringRegex(m_Id, "description");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.description = \"{1}\"", m_Id, m_Description));
        regex = FsmRegex::ClassTableRegex(m_Id, "editorPos");
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
                position = GetNode()->GetTargetPosition();
                code = std::regex_replace(code, regex, fmt::format("{0}.editorPos = {{{1}, {2}}}", m_Id, position.x, position.y));
            }
        }
        regex = FsmRegex::ClassStringRegex(m_Id, "currentStateId");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.currentStateId = \"{1}\"", m_Id, m_CurrentStateId));
        regex = FsmRegex::ClassStringRegex(m_Id, "nextStateId");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.nextStateId = \"{1}\"", m_Id, m_NextStateId));
        regex = FsmRegex::ClassIntegerRegex(m_Id, "priority");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.priority = {1}", m_Id, m_Priority));
        regex = FsmRegex::FunctionBodyReplace(m_Id, "condition");
        if (std::smatch match; std::regex_search(code, match, regex))
        {
            std::string func;
            for (const auto& line : m_ConditionEditor.GetTextLines())
                func += fmt::format("\t{0}\n", line);
            code = std::regex_replace(code, regex, "$1\n" + func + "$3");
        }
        regex = FsmRegex::FunctionBodyReplace(m_Id, "action");
        if (std::smatch match; std::regex_search(code, match, regex))
        {
            std::string func;
            for (const auto& line : m_ActionEditor.GetTextLines())
                func += fmt::format("\t{0}\n", line);
            code = std::regex_replace(code, regex, "$1\n" + func + "$3");
        }
    }


    //std::string capturingRegex = "\\s*([\\s\\S]*?)end(?!\\s*end\\b)(?=(\\s*---@return|\\s*---@param|\\s*function|\\s*$))";
    std::string CAPTURING_REGEX = R"(\s*([\s\S]*?)\s*end---@endFunc)";

    void FsmTrigger::UpdateFromFile(const std::string& filePath)
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
        std::string currentStateId;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassStringRegex(m_Id, "currentStateId")))
            currentStateId = match[1].str();
        SetCurrentState(currentStateId);
        std::string nextStateId;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassStringRegex(m_Id, "nextStateId")))
            nextStateId = match[1].str();
        SetNextState(nextStateId);
        int priority = 0;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassIntegerRegex(m_Id, "priority")))
            priority = std::stoi(match[1].str());
        SetPriority(priority);
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
                GetNode()->SetTargetPosition(position);
            }
        }
        std::string condition = "return false";
        if (std::smatch match; std::regex_search(code, match, FsmRegex::FunctionBody(m_Id, "condition")))
            condition = match[1].str();
        SetCondition(condition);
        std::string action;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::FunctionBody(m_Id, "action")))
            action = match[1].str();
        SetAction(action);
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
        m_NextStateId = stateId;
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
            return;
        if (const auto state = fsm->GetState(stateId); state)
            m_NextState = state.get();
    }

    void FsmTrigger::SetCurrentState(const std::string& stateId)
    {
        m_CurrentStateId = stateId;
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
            return;
        if (const auto state = fsm->GetState(stateId); state)
        {
            m_CurrentState = state.get();
            if (!state->GetTriggers().contains(m_Id))
                state->AddTrigger(fsm->GetTrigger(m_Id));
        }
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
        if (ImGui::Button(MakeIdString("Preview Lua Code").c_str()) || m_LuaCodeEditor.GetText().empty())
            m_LuaCodeEditor.SetText(GetLuaCode());
        ImGui::SameLine();
        if (auto linkedFile = NodeEditor::Get()->GetCurrentFsm()->GetLinkedFile(); !linkedFile.empty())
        {
            if (ImGui::Button(MakeIdString("Update From File").c_str()))
                UpdateFromFile(linkedFile);
            ImGui::SameLine();
        }
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button(MakeIdString("Delete Trigger").c_str()))
            DELETE_TRIGGER = true;
        ImGui::SetItemTooltip("This doesn't delete it from your lua file! Delete it manually!");
        ImGui::PopStyleColor();
        ImGui::Separator();
        if (ImGui::BeginTabBar(MakeIdString("Node Properties").c_str()))
        {
            if (ImGui::BeginTabItem(MakeIdString("Trigger Properties").c_str()))
            {
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
                ImGui::PopStyleColor();
                ImGui::Separator();
                ImGui::Text("Condition");
                ImGui::Separator();
                Window::DrawTextEditor(m_ConditionEditor, m_Condition);
                ImGui::Separator();
                
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("Action").c_str()))
            {
                Window::DrawTextEditor(m_ActionEditor, m_Action);
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("Lua Code").c_str()))
            {
                m_LuaCodeEditor.Render("Lua Code");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
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
                    ImGui::SetClipboardText(newState->GetLuaCode().c_str());
                    ImGui::InsertNotification({ImGuiToastType::Success, 3000, "State added: %s, code copied to clipboard", key.c_str()});
                    key = "";
                    name = "";
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
                    const auto trigger = NodeEditor::Get()->GetCurrentFsm()->GetTrigger(GetId());
                    trigger->SetNextState(newState->GetId());
                    newState->SetName(name);
                    ImGui::SetClipboardText(newState->GetLuaCode().c_str());
                    ImGui::InsertNotification({ImGuiToastType::Success, 3000, "State added: %s, code copied to clipboard", key.c_str()});
                    key = "";
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
        trigger->SetAction(json["action"].get<std::string>());
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
        j["action"] = m_Action;
        j["positionX"] = m_Node.GetTargetPosition().x;
        j["positionY"] = m_Node.GetTargetPosition().y;
        return j;
    }
}
