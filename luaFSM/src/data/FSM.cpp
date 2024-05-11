#include "pch.h"
#include "FSM.h"

#include <codecvt>

#include "FsmTrigger.h"
#include <spdlog/fmt/bundled/format.h>

#include <utility>

#include "Graphics/Window.h"
#include "imgui/imgui_stdlib.h"
#include "imgui/NodeEditor.h"
#include "IO/FileReader.h"
#include "json.hpp"
#include "Log.h"
#include <shellapi.h>

#include "imgui/ImGuiNotify.hpp"

namespace LuaFsm
{
    Fsm::Fsm(const std::string& id)
    {
        Fsm::SetId(id);
        m_LuaCodeEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
    }

    FsmStatePtr Fsm::AddState(const std::string& key)
    {
        m_States[key] = make_shared<FsmState>(key);
        auto value = m_States[key];
        if (m_InitialStateId.empty())
            m_InitialStateId = key;
        return value;
    }

    void Fsm::AddState(const FsmStatePtr& value)
    {
        const auto key = value->GetId();
        m_States[key] = value;
        if (m_InitialStateId.empty())
            m_InitialStateId = key;
    }

    std::unordered_map<std::string, FsmTriggerPtr> Fsm::GetTriggers()
    {
        return m_Triggers;
    }

    void Fsm::AddTrigger(const FsmTriggerPtr& value)
    {
        const auto key = value->GetId();
        m_Triggers[key] = value;
        if (const auto state = value->GetCurrentState(); state != nullptr)
            state->AddTrigger(value);
    }

    FsmTriggerPtr Fsm::GetTrigger(const std::string& key)
    {
        if (!m_Triggers.contains(key))
            return nullptr;
        return m_Triggers[key];
    }

    FsmStatePtr Fsm::GetState(const std::string& key)
    {
        if (!m_States.contains(key))
            return nullptr;
        return m_States[key];
    }

    void Fsm::RemoveState(const std::string& state)
    {
        if (NodeEditor::Get()->GetSelectedNode() == GetState(state)->GetNode())
            NodeEditor::Get()->DeselectAllNodes();
        if (m_States.contains(state))
            m_States.erase(state);
        for (const auto& value : m_Triggers | std::views::values)
        {
            if (value->GetCurrentStateId() == state)
                value->SetCurrentState("");
            if (value->GetNextStateId() == state)
                value->SetNextState("");
        }
        if (m_InitialStateId == state)
            m_InitialStateId = "";
    }

    FsmState* Fsm::GetInitialState()
    {
        if (m_InitialStateId.empty())
            return nullptr;
        if (const auto state = GetState(m_InitialStateId); state)
            return state.get();
        return nullptr;
    }

    bool SET_NEW_FSM_ID = false;
    
    void Fsm::DrawProperties()
    {
        if (!m_LinkedFile.empty())
        {
            ImGui::SameLine();
            if (ImGui::Button("Load from file"))
                UpdateFromFile(m_LinkedFile);
            ImGui::SameLine();
            if (ImGui::Button("Save to file"))
                UpdateToFile(m_Id);
        }
        if (ImGui::BeginTabBar("FSM Properties"))
        {
            if (ImGui::BeginTabItem("FSM Properties"))
            {
                ImGui::Text("ID: %s", m_Id.c_str());
                ImGui::SameLine();
                if (ImGui::Button("Refactor ID"))
                    SET_NEW_FSM_ID = true;
                ImGui::InputText("Name", &m_Name);
                ImGui::Separator();
                ImGui::Text("Initial State: ");
                if (!m_InitialStateId.empty())
                {
                    ImGui::SameLine();
                    ImGui::Selectable(m_InitialStateId.c_str(), false);
                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        NodeEditor::Get()->MoveToNode(m_InitialStateId);
                }
                ImGui::Separator();
                ImGui::Text("Linked Lua File: ");
                ImGui::SameLine();
                if (!m_LinkedFile.empty())
                {
                    ImGui::Selectable(m_LinkedFile.c_str());
                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                        ShellExecuteA(nullptr, "open", m_LinkedFile.c_str(),
                                      nullptr, nullptr, SW_SHOWDEFAULT);
                }
                else
                    ImGui::Text("Create a lua file to link it!");
                ImGui::Separator();
                m_LuaCodeEditor.SetPalette(Window::GetPalette());
                if (ImGui::Button("Regenerate code"))
                    m_LuaCodeEditor.SetText(GetLuaCode());
                ImGui::Separator();
                m_LuaCodeEditor.Render("Lua Code");
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }

        if (SET_NEW_FSM_ID)
        {
            ImGui::OpenPopup("Refactor ID");
            if (ImGui::BeginPopupModal("Refactor ID", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Enter new ID for FSM: ");
                static std::string newId;
                ImGui::InputText("##newId", &newId);
                if (const auto invalid = std::regex_search(newId, FsmRegex::InvalidIdRegex());
                    !invalid && !newId.empty()
                    && !GetTrigger(newId)
                    && !GetState(newId)
                    && ImGui::Button("Refactor"))
                {
                    RefactorId(newId);
                    newId = "";
                    SET_NEW_FSM_ID = false;
                    ImGui::CloseCurrentPopup();
                }
                else if (invalid)
                    ImGui::Text("Invalid ID");
                else if (newId.empty())
                    ImGui::Text("ID can't be empty");
                else if (GetTrigger(newId))
                    ImGui::Text("Trigger with this ID already exists");
                else if (GetState(newId))
                    ImGui::Text("State with this ID already exists");
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    SET_NEW_FSM_ID = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }

    void Fsm::RefactorId(const std::string& newId)
    {
        const std::string oldId = m_Id;
        if (GetTrigger(newId) || GetState(newId))
        {
            ImGui::InsertNotification({ImGuiToastType::Error, 3000, "State with id %s already exists!", newId.c_str()});
            return;
        }
        SetId(newId);
        if (GetLinkedFile().empty())
            return;
        UpdateToFile(oldId);
    }

    void Fsm::UpdateToFile(const std::string& oldId)
    {
        const auto filePath = GetLinkedFile();
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
        ImGui::InsertNotification({ImGuiToastType::Success, 3000, "Updated file at: %s", filePath.c_str()});
        UpdateEditors();
    }
    
    void Fsm::UpdateFileContents(std::string& code, const std::string& oldId)
    {
        std::regex regex = FsmRegex::ClassStringRegex(oldId, "name");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.name = \"{1}\"", m_Id, m_Name));
        else if (!m_Name.empty())
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm name entry not found in file!"});
        regex = FsmRegex::ClassStringRegex(oldId, "initialStateId");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.initialStateId = \"{1}\"", m_Id, m_InitialStateId));
        else if (!m_InitialStateId.empty())
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Initial state ID entry not found in file!"});
        for (const auto& value : m_States | std::views::values)
            value->UpdateFileContents(code, value->GetId());
        for (const auto& value : m_Triggers | std::views::values)
            value->UpdateFileContents(code, value->GetId());
    }

    void Fsm::UpdateEditors()
    {
        m_LuaCodeEditor.SetText(GetLuaCode());
    }

    std::string Fsm::GetActivateFunctionCode()
    {
        std::string code;
        code += fmt::format("\n---Activate this FSM\n");
        code += fmt::format("function {0}:activate()\n", m_Id);
        for (const auto& [key, state] : m_States)
        {
            for (const auto& [id, trigger] : state->GetTriggers())
                code += fmt::format("\t{0}:registerCondition({1})\n", state->GetId(), id);
            code += fmt::format("\tself:registerState({0})\n", key);
        }
        if (m_InitialStateId.empty())
            code += fmt::format("\tself:setInitialState(self.initialStateId)\n");
        else
            code += fmt::format("\tself:setInitialState({0})\n", m_InitialStateId);
        code += fmt::format("end\n");
        return code;
    }
    
    std::string Fsm::GetLuaCode()
    {
        std::string code;
        code += fmt::format("---@FSM {0}\n", m_Id);
        code += fmt::format("---@class {0} : FSM\n", m_Id);
        code += fmt::format("{0} = FSM:new({{}})\n", m_Id);
        code += fmt::format("{0}.name = \"{1}\"\n", m_Id, m_Name);
        code += fmt::format("{0}.initialStateId = \"{1}\"\n\n", m_Id, m_InitialStateId);
        code += GetActivateFunctionCode();
        return code;
    }

    std::shared_ptr<Fsm> Fsm::CreateFromFile(const std::string& filePath)
    {
        const std::string code = FileReader::ReadAllText(filePath);
        if (code.empty())
            return nullptr;
        std::string id;
        std::regex regex = FsmRegex::IdRegex("FSM");
        if (std::smatch match; std::regex_search(code, match, regex))
            id = match[1].str();
        else
            return nullptr;
        auto fsm = std::make_shared<Fsm>(id);
        NodeEditor::Get()->SetCurrentFsm(fsm);
        NodeEditor::Get()->DeselectAllNodes();
        for (const auto states = FsmState::CreateFromFile(filePath); const auto& state : states)
            fsm->AddState(state);
        for (const auto conditions = FsmTrigger::CreateFromFile(filePath); const auto& condition : conditions)
            fsm->AddTrigger(condition);
        fsm->UpdateFromFile(filePath);
        for (const auto& condition : fsm->GetTriggers() | std::views::values)
            if (const auto state = condition->GetCurrentState(); state != nullptr)
                state->AddTrigger(condition);
        fsm->SetLinkedFile(filePath);
        return fsm;
    }

    void Fsm::UpdateFromFile(const std::string& filePath)
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
        std::string initialStateId;
        for (const auto& state : m_States | std::views::values)
            state->UpdateFromFile(filePath);
        for (const auto& trigger : m_Triggers | std::views::values)
            trigger->UpdateFromFile(filePath);
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassStringRegex(m_Id, "initialStateId")))
            initialStateId = match[1].str();
        SetInitialState(initialStateId);
        m_LuaCodeEditor.SetText(GetLuaCode());
        ImGui::InsertNotification({ImGuiToastType::Success, 3000, "Updated from file: %s", filePath.c_str()});
    }

    void Fsm::ChangeTriggerId(const std::string& oldId, const std::string& newId)
    {
        if (m_Triggers.contains(oldId))
        {
            m_Triggers[newId] = m_Triggers[oldId];
            m_Triggers.erase(oldId);
        }
    }

    void Fsm::RemoveTrigger(const std::string& trigger)
    {
        if (NodeEditor::Get()->GetSelectedNode() == GetTrigger(trigger)->GetNode())
            NodeEditor::Get()->DeselectAllNodes();
        const auto obj = GetTrigger(trigger);
        if (obj == nullptr)
            return;
        if (const auto state = obj->GetCurrentState(); state != nullptr)
            state->RemoveTrigger(trigger);
        m_Triggers.erase(trigger);
    }
}
