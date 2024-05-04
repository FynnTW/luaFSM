﻿#pragma once
#include <memory>
#include <string>
#include "FsmState.h"
#include "FsmTrigger.h"

namespace LuaFsm
{
    class FsmState;
    class FsmTrigger;
    typedef std::shared_ptr<FsmState> FsmStatePtr;
    class Fsm
    {
    public:
        explicit Fsm(std::string id);
        [[nodiscard]] std::string GetName() const { return m_Name; }
        [[nodiscard]] std::string GetId() const { return m_Id; }
        [[nodiscard]] FsmState* GetCurrentState() const { return m_CurrentState; }
        [[nodiscard]] std::string GetInitialState() const { return m_InitialStateId; }
        [[nodiscard]] std::vector<std::string> GetUpdateEvents() const { return m_UpdateEvents; }
        [[nodiscard]] std::unordered_map<std::string, FsmStatePtr> GetStates() const { return m_States; }
        void SetName(const std::string& name) { m_Name = name; }
        void SetId(const std::string& id) { m_Id = id; }
        void SetCurrentState(FsmState* currentState) { m_CurrentState = currentState; }
        void SetInitialState(const std::string& initialState) { m_InitialStateId = initialState; }
        void AddUpdateEvent(const std::string& updateEvent) { m_UpdateEvents.push_back(updateEvent); }
        void AddState(const std::string& key, const FsmStatePtr& value);
        FsmStatePtr AddState(const std::string& key);
        void AddState(const FsmStatePtr& value);
        void AddTrigger(const std::string& key, FsmTrigger* value);
        void AddTrigger(FsmTrigger* value);
        FsmTrigger* GetTrigger(const std::string& key);
        void RemoveTrigger(const std::string& trigger);
        void ClearTriggers() { m_Triggers.clear(); }
        FsmStatePtr GetState(const std::string& key);
        void RemoveState(const std::string& state) { m_States.erase(state); }
        void ClearStates() { m_States.clear(); }
        void RemoveUpdateEvent(const std::string& updateEvent) { std::erase(m_UpdateEvents, updateEvent); }
        void ClearUpdateEvents() { m_UpdateEvents.clear(); }
        std::string GetLuaCode();
        std::unordered_map<std::string, FsmTrigger*> GetTriggers();

    private:
        std::string m_Name;
        std::string m_Id;
        std::unordered_map<std::string, FsmStatePtr> m_States{};
        std::unordered_map<std::string, FsmTrigger*> m_Triggers{};
        std::unordered_map<int, std::string> m_TriggerIds{};
        FsmState* m_CurrentState;
        std::string m_InitialStateId;
        std::vector<std::string> m_UpdateEvents{};
    };
}
