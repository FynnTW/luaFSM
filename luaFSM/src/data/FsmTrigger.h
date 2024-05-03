#pragma once
#include <memory>
#include <string>

namespace LuaFsm
{
    class FsmState;
    typedef std::shared_ptr<FsmState> FsmStatePtr;
    class FsmTrigger
    {
    public:
        FsmTrigger(std::string id);
        [[nodiscard]] const std::string& GetName() const { return m_Name; }
        [[nodiscard]] const std::string& GetId() const { return m_Id; }
        [[nodiscard]] const std::string& GetDescription() const { return m_Description; }
        [[nodiscard]] int GetPriority() const { return m_Priority; }
        [[nodiscard]] const std::string& GetCondition() const { return m_Condition; }
        [[nodiscard]] const std::string& GetOnTrue() const { return m_OnTrue; }
        [[nodiscard]] const std::string& GetOnFalse() const { return m_OnFalse; }
        [[nodiscard]] const std::unordered_map<std::string, std::string>& GetArguments() const { return m_Arguments; }
        [[nodiscard]] const std::string& GetNextStateId() const { return m_NextStateId; }
        [[nodiscard]] const FsmStatePtr& GetCurrentState() const { return m_CurrentState; }
        [[nodiscard]] const FsmStatePtr& GetNextState() const { return m_NextState; }
        void SetName(const std::string& name) { m_Name = name; }
        void SetId(const std::string& id) { m_Id = id; }
        void SetDescription(const std::string& description) { m_Description = description; }
        void SetPriority(const int priority) { m_Priority = priority; }
        void SetCondition(const std::string& condition) { m_Condition = condition; }
        void SetOnTrue(const std::string& onTrue) { m_OnTrue = onTrue; }
        void SetOnFalse(const std::string& onFalse) { m_OnFalse = onFalse; }
        void AddArgument(const std::string& key, const std::string& value) { m_Arguments[key] = value; }
        void SetNextStateId(const std::string& nextStateId) { m_NextStateId = nextStateId; }
        void SetCurrentState(const FsmStatePtr& currentState) { m_CurrentState = currentState; }
        void SetNextState(const FsmStatePtr& nextState) { m_NextState = nextState; }
        [[nodiscard]] int GetIntId() const { return m_IntId; }
        void SetIntId(const int intId) { m_IntId = intId; }
        [[nodiscard]] int GetInputId() const { return m_InputId; }
        void SetInputId(const int inputId) { m_InputId = inputId; }
        [[nodiscard]] int GetOutputId() const { return m_OutputId; }
        void SetOutputId(const int outputId) { m_OutputId = outputId; }
        std::string GetLuaCode();

    private:
        std::string m_Name;
        std::string m_Id;
        int m_IntId;
        int m_InputId = -1;
        int m_OutputId = -1;
        std::string m_Description;
        int m_Priority = 0;
        std::string m_Condition;
        std::string m_OnTrue;
        std::string m_OnFalse;
        std::unordered_map<std::string, std::string> m_Arguments{};
        std::string m_NextStateId;
        FsmStatePtr m_CurrentState;
        FsmStatePtr m_NextState;
    };

}
