#include "pch.h"
#include "FSM.h"
#include "FsmTrigger.h"
#include <spdlog/fmt/bundled/format.h>

namespace LuaFsm
{
    Fsm::Fsm(std::string id)
        : m_Id(std::move(id))
    {
        
    }

    void Fsm::AddState(const std::string& key, const FsmStatePtr& value)
    {
        m_States[key] = value;
        value->SetFsm(this);
        for (const auto& event : value->GetEvents())
            if (std::ranges::find(m_UpdateEvents, event) == m_UpdateEvents.end())
                AddUpdateEvent(event);
    }

    FsmStatePtr Fsm::AddState(const std::string& key)
    {
        m_States[key] = make_shared<FsmState>(key);
        auto value = m_States[key];
        value->SetFsm(this);
        for (const auto& event : value->GetEvents())
            if (std::ranges::find(m_UpdateEvents, event) == m_UpdateEvents.end())
                AddUpdateEvent(event);
        return value;
    }

    void Fsm::AddState(const FsmStatePtr& value)
    {
        auto key = value->GetId();
        AddState(key, value);
    }

    void Fsm::AddTrigger(const std::string& key, FsmTrigger* value)
    {
        m_Triggers[key] = value;
        value->SetCurrentState(m_Id);
    }

    std::unordered_map<std::string, FsmTrigger*> Fsm::GetTriggers()
    {
        return m_Triggers;
    }

    void Fsm::AddTrigger(FsmTrigger* value)
    {
        const auto key = value->GetId();
        AddTrigger(key, value);
    }

    FsmTrigger* Fsm::GetTrigger(const std::string& key)
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
