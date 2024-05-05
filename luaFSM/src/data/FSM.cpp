#include "pch.h"
#include "FSM.h"
#include "FsmTrigger.h"
#include <spdlog/fmt/bundled/format.h>

#include <utility>

namespace LuaFsm
{
    Fsm::Fsm(const std::string& id)
    {
        Fsm::SetId(id);
    }

    void Fsm::AddState(const std::string& key, const FsmStatePtr& value)
    {
        m_States[key] = value;
        for (const auto& event : value->GetEvents())
            if (std::ranges::find(m_UpdateEvents, event) == m_UpdateEvents.end())
                AddUpdateEvent(event);
        if (m_InitialStateId.empty())
            m_InitialStateId = key;
    }

    FsmStatePtr Fsm::AddState(const std::string& key)
    {
        m_States[key] = make_shared<FsmState>(key);
        auto value = m_States[key];
        for (const auto& event : value->GetEvents())
            if (std::ranges::find(m_UpdateEvents, event) == m_UpdateEvents.end())
                AddUpdateEvent(event);
        if (m_InitialStateId.empty())
            m_InitialStateId = key;
        return value;
    }

    void Fsm::AddState(const FsmStatePtr& value)
    {
        const auto key = value->GetId();
        AddState(key, value);
    }

    void Fsm::AddTrigger(const std::string& key, FsmTriggerPtr value)
    {
        m_Triggers[key] = std::move(value);
    }

    std::unordered_map<std::string, FsmTriggerPtr> Fsm::GetTriggers()
    {
        return m_Triggers;
    }

    void Fsm::AddTrigger(const FsmTriggerPtr& value)
    {
        const auto key = value->GetId();
        AddTrigger(key, value);
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
        if (m_States.contains(state))
            m_States.erase(state);
        for (const auto& [key, value] : m_Triggers)
        {
            if (value->GetCurrentStateId() == state)
                value->SetCurrentState("");
            if (value->GetNextStateId() == state)
                value->SetNextState("");
        }
        if (m_InitialStateId == state)
            m_InitialStateId = "";
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
        for (const auto& [key, value] : m_States)
            code += value->GetLuaCode(2) + ",\n";
        code += fmt::format("\t}},\n");
        code += fmt::format("}})");
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
        const auto obj = GetTrigger(trigger);
        if (obj == nullptr)
            return;
        if (const auto state = obj->GetCurrentState(); state != nullptr)
            state->RemoveTrigger(trigger);
        m_Triggers.erase(trigger);
    }
}
