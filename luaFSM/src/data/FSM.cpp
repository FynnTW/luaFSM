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
        m_OnActivateEditor.SetText(m_OnActivate);
        m_OnActivateEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        m_OnProcessEditor.SetText(m_OnProcess);
        m_OnProcessEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        m_OnTerminateEditor.SetText(m_OnTerminate);
        m_OnTerminateEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
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
    
    void Fsm::DrawProperties()
    {
        std::string id = GetId();
        if (ImGui::Button("Preview Code") || m_LuaCodeEditor.GetText().empty())
            m_LuaCodeEditor.SetText(GetLuaCode());
        if (const auto linkedFile = NodeEditor::Get()->GetCurrentFsm()->GetLinkedFile(); !linkedFile.empty() && ImGui::Button("Refresh From File"))
            UpdateFromFile(linkedFile);
        if (ImGui::BeginTabBar("FSM Properties"))
        {
            if (ImGui::BeginTabItem("FSM Properties"))
            {
                ImGui::InputText("Id", &id);
                ImGui::InputText("Name", &m_Name);
                ImGui::Separator();
                ImGui::Text("Initial State: ");
                if (!GetInitialStateId().empty())
                {
                    ImGui::SameLine();
                    ImGui::Selectable(GetInitialStateId().c_str(), false);
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    NodeEditor::Get()->SetSelectedNode(NodeEditor::Get()->GetNode(GetInitialStateId()));
                ImGui::Separator();
                ImGui::Text("Linked Lua File: ");
                ImGui::SameLine();
                if (!m_LinkedFile.empty())
                {
                    ImGui::Selectable(m_LinkedFile.c_str());
                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                    {
                        ShellExecuteA(nullptr, "open", m_LinkedFile.c_str(), nullptr, nullptr, SW_SHOWDEFAULT);
                    }
                }
                else
                    ImGui::Text("Export once to link a file!");
                ImGui::Separator();
                m_LuaCodeEditor.SetPalette(Window::GetPalette());
                m_LuaCodeEditor.Render("Lua Code");
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("OnActivate"))
            {
                Window::DrawTextEditor(m_OnActivateEditor, m_OnActivate);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("OnProcess"))
            {
                Window::DrawTextEditor(m_OnProcessEditor, m_OnProcess);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("OnTerminate"))
            {
                Window::DrawTextEditor(m_OnTerminateEditor, m_OnTerminate);
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        if (id != GetId())
        {
            SetId(id);
        }
    }
    
    nlohmann::json Fsm::Serialize() const
    {
        nlohmann::json j;
        j["id"] = m_Id;
        j["name"] = m_Name;
        j["initialStateId"] = m_InitialStateId;
        nlohmann::json states;
        for (const auto& [key, value] : m_States)
        {
            states[key] = value->Serialize();
        }
        j["states"] = states;
        nlohmann::json triggers;
        for (const auto& [key, value] : m_Triggers)
        {
            triggers[key] = value->Serialize();
        }
        j["triggers"] = triggers;
        j["OnActivate"] = m_OnActivate;
        j["OnProcess"] = m_OnProcess;
        j["OnTerminate"] = m_OnTerminate;
        return j;
    }

    std::shared_ptr<Fsm> Fsm::Deserialize(const nlohmann::json& json)
    {
        auto fsm = std::make_shared<Fsm>(json["id"].get<std::string>());
        NodeEditor::Get()->SetCurrentFsm(fsm);
        fsm->SetName(json["name"].get<std::string>());
        fsm->SetInitialState(json["initialStateId"].get<std::string>());
        for (const auto& [key, value] : json["states"].items())
            fsm->AddState(FsmState::Deserialize(value));
        for (const auto& [key, value] : json["triggers"].items())
        {
            const auto trigger = FsmTrigger::Deserialize(value);
            if (const auto state = trigger->GetCurrentState(); state != nullptr)
                state->AddTrigger(trigger);
            else
                fsm->AddTrigger(trigger);
        }
        fsm->SetOnActivate(json["OnActivate"].get<std::string>());
        fsm->SetOnProcess(json["OnProcess"].get<std::string>());
        fsm->SetOnTerminate(json["OnTerminate"].get<std::string>());
        fsm->UpdateEditors();
        return fsm;
    }

    void Fsm::UpdateEditors()
    {
        m_OnActivateEditor.SetText(m_OnActivate);
        m_OnProcessEditor.SetText(m_OnProcess);
        m_OnTerminateEditor.SetText(m_OnTerminate);
        m_LuaCodeEditor.SetText(GetLuaCode());
    }

    std::string Fsm::GetLuaCode()
    {
        std::string code;
        code += fmt::format("---@diagnostic disable: inject-field\n");
        code += fmt::format("require(\"FSM\")\n\n");
        code += fmt::format("---@type FSM\n");
        code += fmt::format("{0} = FSM:new({{\n", m_Id);
        code += fmt::format("\t---Unique Identifier of this FSM\n");
        code += fmt::format("\t---@type string\n");
        code += fmt::format("\tid = \"{0}\",\n", m_Id);
        code += fmt::format("\t---Name of this FSM\n");
        code += fmt::format("\t---@type string\n");
        code += fmt::format("\tname = \"{0}\",\n", m_Name);
        code += fmt::format("\t---Name of the initial state\n");
        code += fmt::format("\t---@type string\n");
        code += fmt::format("\tinitialStateId = \"{0}\",\n", m_InitialStateId);
        code += fmt::format("\t---States within this FSM\n");
        code += fmt::format("\t---@type table<string, FSM_STATE>\n");
        code += fmt::format("\tstates = {{\n");
        code += fmt::format("\t}},\n");
        code += fmt::format("}})\n");
        code += fmt::format("\n");
        code += fmt::format("\nfunction {0}:onActivate()", m_Id);
        if (m_OnActivate.empty())
            code +=  fmt::format(" ");
        else
        {
            code += fmt::format("\n");
            for (const auto& line : m_OnActivateEditor.GetTextLines())
                code += fmt::format("\t{0}\n", line);
        }
        code += fmt::format("end---@endFunc\n");
        code += fmt::format("\nfunction {0}:onProcess()", m_Id);
        if (m_OnProcess.empty())
            code +=  fmt::format(" ");
        else
        {
            code += fmt::format("\n");
            for (const auto& line : m_OnProcessEditor.GetTextLines())
                code += fmt::format("\t{0}\n", line);
        }
        code += fmt::format("end---@endFunc\n");
        code += fmt::format("\nfunction {0}:onTerminate()", m_Id);
        if (m_OnTerminate.empty())
            code +=  fmt::format(" ");
        else
        {
            code += fmt::format("\n");
            for (const auto& line : m_OnTerminateEditor.GetTextLines())
                code += fmt::format("\t{0}\n", line);
        }
        code += fmt::format("end---@endFunc\n\n");
        for (const auto& value : m_States | std::views::values)
        {
            code += value->GetLuaCode(0) + "\n";
        }
        code += fmt::format("\t---Activate this FSM\n");
        code += fmt::format("function {0}:activate()\n", m_Id);
        for (const auto& [key, state] : m_States)
        {
            for (const auto& [id, trigger] : state->GetTriggers())
                code += fmt::format("\t{0}:registerTrigger({1})\n", state->GetId(), id);
            code += fmt::format("\tself:registerState({0})\n", key);
            
        }
        code += fmt::format("\tself:setInitialState({0})\n", m_InitialStateId);
        code += fmt::format("\tself:onActivate()\n");
        code += fmt::format("end\n");
        
        return code;
    }
    
    std::string CAPTURING_REGEX_FSM = R"(\s*([\s\S]*?)\s*end---@endFunc)";

    void Fsm::UpdateFromFile(const std::string& filePath)
    {
        const std::string code = FileReader::ReadAllText(filePath);
        if (code.empty())
            return;
        const std::string onActivateFuncName = fmt::format("{0}:onActivate\\(\\)", m_Id);
        const std::regex onActivateRegex(onActivateFuncName + CAPTURING_REGEX_FSM);
        if (std::smatch match; std::regex_search(code, match, onActivateRegex))
        {
            m_OnActivate = FileReader::RemoveStartingTab(match[1].str());
        }
        const std::string onProcessFuncName = fmt::format("{0}:onProcess\\(\\)", m_Id);
        const std::regex onProcessRegex(onProcessFuncName + CAPTURING_REGEX_FSM);
        if (std::smatch match; std::regex_search(code, match, onProcessRegex))
        {
            m_OnProcess = FileReader::RemoveStartingTab(match[1].str());
        }
        const std::string onTerminateFuncName = fmt::format("{0}:onTerminate\\(\\)", m_Id);
        const std::regex onTerminateRegex(onTerminateFuncName + CAPTURING_REGEX_FSM);
        if (std::smatch match; std::regex_search(code, match, onTerminateRegex))
        {
            m_OnTerminate = FileReader::RemoveStartingTab(match[1].str());
        }
        for (const auto& state : m_States | std::views::values)
            state->UpdateFromFile(filePath);
        for (const auto& trigger : m_Triggers | std::views::values)
            trigger->UpdateFromFile(filePath);
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
