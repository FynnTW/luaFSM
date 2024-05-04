#pragma once
#include <memory>
#include <string>

#include "FsmState.h"
#include "imgui.h"
#include "Graphics/VisualNode.h"
#include "imgui/TextEditor.h"

namespace LuaFsm
{
    class FsmState;
    class Fsm;
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
        [[nodiscard]] FsmState* GetCurrentState() const { return m_CurrentState; }
        [[nodiscard]] FsmState* GetNextState() const { return m_NextState; }
        void SetName(const std::string& name) { m_Name = name; }
        void SetId(const std::string& id) { m_Id = id; }
        void SetDescription(const std::string& description) { m_Description = description; }
        void SetPriority(const int priority) { m_Priority = priority; }
        void SetCondition(const std::string& condition) { m_Condition = condition; }
        void SetOnTrue(const std::string& onTrue) { m_OnTrue = onTrue; }
        void SetOnFalse(const std::string& onFalse) { m_OnFalse = onFalse; }
        void AddArgument(const std::string& key, const std::string& value) { m_Arguments[key] = value; }
        void SetNextState(const std::string& stateId);
        void SetCurrentState(const std::string& stateId);
        void SetFsm(Fsm* fsm) { m_Fsm = fsm; }
        [[nodiscard]] Fsm* GetFsm() const { return m_Fsm; }
        std::string GetLuaCode(int indent = 0);
        [[nodiscard]] ImVec2 GetPosition() const { return m_Node.GetPosition(); }
        void SetPosition(const ImVec2& position) { m_Node.SetPosition(position); }
        VisualNode* DrawNode(NodeEditor* editor);
        VisualNode* GetNode() { return &m_Node; }
        TextEditor* GetConditionEditor() { return &m_ConditionEditor; }
        TextEditor* GetOnTrueEditor() { return &m_OnTrueEditor; }
        TextEditor* GetOnFalseEditor() { return &m_OnFalseEditor; }

    private:
        std::string m_Name;
        VisualNode m_Node{};
        std::string m_Id;
        std::string m_Description;
        int m_Priority = 0;
        std::string m_Condition = "return false";
        TextEditor m_ConditionEditor{};
        std::string m_OnTrue;
        TextEditor m_OnTrueEditor{};
        std::string m_OnFalse;
        TextEditor m_OnFalseEditor{};
        std::unordered_map<std::string, std::string> m_Arguments{};
        std::string m_NextStateId;
        std::string m_CurrentStateId;
        FsmState* m_CurrentState = nullptr;
        FsmState* m_NextState = nullptr;
        Fsm* m_Fsm = nullptr;
    };

}
