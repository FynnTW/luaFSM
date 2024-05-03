#pragma once
#include <memory>
#include <string>

#include "FsmTrigger.h"
#include "Fsm.h"

namespace LuaFsm
{
    class Fsm;
    typedef std::shared_ptr<FsmTrigger> FsmTriggerPtr;
    typedef std::shared_ptr<Fsm> FsmPtr;
    class FsmState
    {
    public:
        FsmState(std::string id);
        [[nodiscard]] std::string GetName() const { return m_Name; }
        [[nodiscard]] std::string GetId() const { return m_Id; }
        [[nodiscard]] std::string GetDescription() const { return m_Description; }
        [[nodiscard]] std::string GetOnInit() const { return m_OnInit; }
        [[nodiscard]] std::string GetOnEnter() const { return m_OnEnter; }
        [[nodiscard]] std::string GetOnUpdate() const { return m_OnUpdate; }
        [[nodiscard]] std::string GetOnExit() const { return m_OnExit; }
        [[nodiscard]] std::unordered_map<std::string, std::string> GetData() const { return m_Data; }
        [[nodiscard]] std::unordered_map<std::string, FsmTriggerPtr> GetTriggers() const { return m_Triggers; }
        [[nodiscard]] std::shared_ptr<Fsm> GetFsm() const { return m_Fsm; }
        [[nodiscard]] std::vector<std::string> GetEvents() const { return m_Events; }

        void SetName(const std::string& name) { m_Name = name; }
        void SetId(const std::string& id);
        void SetDescription(const std::string& description) { m_Description = description; }
        void SetOnInit(const std::string& onInit) { m_OnInit = onInit; }
        void SetOnEnter(const std::string& onEnter) { m_OnEnter = onEnter; }
        void SetOnUpdate(const std::string& onUpdate) { m_OnUpdate = onUpdate; }
        void SetOnExit(const std::string& onExit) { m_OnExit = onExit; }
        void AddData(const std::string& key, const std::string& value) { m_Data[key] = value; }
        void AddTrigger(const std::string& key, const FsmTriggerPtr& value);
        void AddTrigger(const FsmTriggerPtr& value);
        FsmTriggerPtr GetTrigger(const std::string& key);
        void SetFsm(const FsmPtr& fsm) { m_Fsm = fsm; }
        void AddEvent(const std::string& event) { m_Events.push_back(event); }
        void RemoveEvent(const std::string& event) { std::erase(m_Events, event); }
        void ClearEvents() { m_Events.clear(); }
        void ClearData() { m_Data.clear(); }
        [[nodiscard]] int GetIntId() const { return m_IntId; }
        void SetIntId(const int intId) { m_IntId = intId; }
        [[nodiscard]] int GetInputId() const { return m_InputId; }
        void SetInputId(const int inputId) { m_InputId = inputId; }
        [[nodiscard]] int GetOutputId() const { return m_OutputId; }
        void SetOutputId(const int outputId) { m_OutputId = outputId; }
        void ClearTriggers() { m_Triggers.clear(); }
        void RemoveTrigger(const std::string& trigger) { m_Triggers.erase(trigger); }
        void RemoveData(const std::string& key) { m_Data.erase(key); }

    private:
        std::string m_Name;
        std::string m_Id;
        int m_IntId = -1;
        int m_InputId = -1;
        int m_OutputId = -1;
        std::string m_Description;
        std::string m_OnInit;
        std::string m_OnEnter;
        std::string m_OnUpdate;
        std::string m_OnExit;
        std::unordered_map<std::string, std::string> m_Data{};
        std::unordered_map<std::string, std::shared_ptr<FsmTrigger>> m_Triggers{};
        FsmPtr m_Fsm;
        std::vector<std::string> m_Events;
    };

    class FsmStateNode
    {
        
    };
}
