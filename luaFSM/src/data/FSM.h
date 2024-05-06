#pragma once
#include <memory>
#include <string>

#include "DrawableObject.h"
#include "FsmState.h"
#include "FsmTrigger.h"
#include "json.hpp"

namespace LuaFsm
{
    class FsmState;
    class FsmTrigger;
    typedef std::shared_ptr<FsmState> FsmStatePtr;
    typedef std::shared_ptr<FsmTrigger> FsmTriggerPtr;
    class Fsm : DrawableObject
    {
    public:
        explicit Fsm(const std::string& id);
        [[nodiscard]] FsmState* GetCurrentState() const { return m_CurrentState; }
        [[nodiscard]] std::string GetInitialState() const { return m_InitialStateId; }
        [[nodiscard]] std::vector<std::string> GetUpdateEvents() const { return m_UpdateEvents; }
        [[nodiscard]] std::unordered_map<std::string, FsmStatePtr> GetStates() const { return m_States; }
        void SetName(const std::string& name) override { m_Name = name; }
        void SetId(const std::string& id) override { m_Id = id; }
        void SetCurrentState(FsmState* currentState) { m_CurrentState = currentState; }
        void SetInitialState(const std::string& initialState) { m_InitialStateId = initialState; }
        void AddUpdateEvent(const std::string& updateEvent) { m_UpdateEvents.push_back(updateEvent); }
        void AddState(const std::string& key, const FsmStatePtr& value);
        FsmStatePtr AddState(const std::string& key);
        void AddState(const FsmStatePtr& value);
        void AddTrigger(const std::string& key, FsmTriggerPtr value);
        void AddTrigger(const FsmTriggerPtr& value);
        FsmTriggerPtr GetTrigger(const std::string& key);
        void RemoveTrigger(const std::string& trigger);
        void ClearTriggers() { m_Triggers.clear(); }
        FsmStatePtr GetState(const std::string& key);
        void RemoveState(const std::string& state);
        void ClearStates() { m_States.clear(); }
        void RemoveUpdateEvent(const std::string& updateEvent) { std::erase(m_UpdateEvents, updateEvent); }
        void ClearUpdateEvents() { m_UpdateEvents.clear(); }
        std::string GetLuaCode();
        void ChangeTriggerId(const std::string& oldId, const std::string& newId);
        std::unordered_map<std::string, FsmTriggerPtr> GetTriggers();
        void DrawProperties();
        static std::shared_ptr<Fsm> ParseFile(const std::string& path);
        std::shared_ptr<Fsm> ParseFile2(const std::string& path);
        nlohmann::json Serialize() const;
        static std::shared_ptr<Fsm> Deserialize(const nlohmann::json& json);

    private:
        std::unordered_map<std::string, FsmStatePtr> m_States{};
        std::unordered_map<std::string, FsmTriggerPtr> m_Triggers{};
        std::unordered_map<int, std::string> m_TriggerIds{};
        FsmState* m_CurrentState;
        std::string m_InitialStateId;
        TextEditor m_LuaCodeEditor;
        std::vector<std::string> m_UpdateEvents{};
    };
}
