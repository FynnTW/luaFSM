#include "pch.h"
#include "FSM.h"
#include "FsmTrigger.h"
#include <spdlog/fmt/bundled/format.h>

#include <utility>

#include "Graphics/Window.h"
#include "imgui/imgui_stdlib.h"
#include "imgui/NodeEditor.h"
#include "IO/FileReader.h"
#include "json.hpp"

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
        if (ImGui::Button("Refresh") || m_LuaCodeEditor.GetText().empty())
            m_LuaCodeEditor.SetText(GetLuaCode());
        if (ImGui::BeginTabBar("FSM Properties"))
        {
            if (ImGui::BeginTabItem("Properties"))
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
                m_LuaCodeEditor.SetPalette(Window::GetPalette());
                m_LuaCodeEditor.Render("Lua Code");
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
        return fsm;
    }

    std::string Fsm::GetLuaCode()
    {
        std::string code;
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
        for (const auto& [key, value] : m_States)
        {
            code += value->GetLuaCode(0) + "\n";
        }
        code += fmt::format("\t---Activate this FSM\n");
        code += fmt::format("function {0}:activate()\n", m_Id);
        for (const auto& [key, state] : m_States)
        {
            for (const auto& [id, trigger] : state->GetTriggers())
                code += fmt::format("{0}:registerTrigger({1})\n", state->GetId(), id);
            code += fmt::format("self:registerState({0})\n", key);
            
        }
        code += fmt::format("self:setInitialState({0})\n", m_InitialStateId);
        code += fmt::format("end\n");
        
        return code;
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
