#pragma once
#include <memory>
#include <string>
#include "FsmState.h"

namespace LuaFsm
{
    class Fsm
    {
    public:
        explicit Fsm(std::string id);
        [[nodiscard]] std::string GetName() const { return m_Name; }
        [[nodiscard]] std::string GetId() const { return m_Id; }
        [[nodiscard]] FsmStatePtr GetCurrentState() const { return m_CurrentState; }
        [[nodiscard]] FsmStatePtr GetInitialState() const { return m_InitialState; }
        [[nodiscard]] std::vector<std::string> GetUpdateEvents() const { return m_UpdateEvents; }
        [[nodiscard]] std::unordered_map<std::string, FsmStatePtr> GetStates() const { return m_States; }
        void SetName(const std::string& name) { m_Name = name; }
        void SetId(const std::string& id) { m_Id = id; }
        void SetCurrentState(const FsmStatePtr& currentState) { m_CurrentState = currentState; }
        void SetInitialState(const FsmStatePtr& initialState) { m_InitialState = initialState; }
        void AddUpdateEvent(const std::string& updateEvent) { m_UpdateEvents.push_back(updateEvent); }
        void AddState(const std::string& key, const FsmStatePtr& value);
        void AddState(const FsmStatePtr& value);
        FsmStatePtr GetState(const std::string& key);
        void RemoveState(const std::string& state) { m_States.erase(state); }
        void ClearStates() { m_States.clear(); }
        void RemoveUpdateEvent(const std::string& updateEvent) { std::erase(m_UpdateEvents, updateEvent); }
        void ClearUpdateEvents() { m_UpdateEvents.clear(); }

    private:
        std::string m_Name;
        std::string m_Id;
        std::unordered_map<std::string, FsmStatePtr> m_States{};
        std::unordered_map<int, std::string> m_TriggerIds{};
        FsmStatePtr m_CurrentState;
        FsmStatePtr m_InitialState;
        std::vector<std::string> m_UpdateEvents{};
    };
    
}
