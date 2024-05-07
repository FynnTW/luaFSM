#pragma once
#include <memory>
#include <string>

#include "imgui.h"
#include "json.hpp"
#include "Graphics/VisualNode.h"
#include "imgui/TextEditor.h"

namespace LuaFsm
{
    class FsmState;
    class Fsm;
    typedef std::shared_ptr<FsmState> FsmStatePtr;
    class FsmTrigger : DrawableObject
    {
    public:
        FsmTrigger(const std::string& id);
        
        [[nodiscard]] virtual std::string GetId() const override { return m_Id; }
        void virtual SetId(const std::string& id) override;
        
        [[nodiscard]] virtual std::string GetName() const override { return m_Name; }
        void virtual SetName(const std::string& name) override { m_Name = name; }
        
        [[nodiscard]] const std::string& GetDescription() const { return m_Description; }
        void SetDescription(const std::string& description) { m_Description = description; }
        
        [[nodiscard]] int GetPriority() const { return m_Priority; }
        void SetPriority(const int priority) { m_Priority = priority; }
        
        [[nodiscard]] const std::unordered_map<std::string, StateData>& GetData();
        void AddData(const std::string& key, const StateData& value) { m_Data[key] = value; }
        
        [[nodiscard]] const std::string& GetCondition() const { return m_Condition; }
        void SetCondition(const std::string& condition) { m_Condition = condition; }
        
        [[nodiscard]] const std::string& GetOnTrue() const { return m_OnTrue; }
        void SetOnTrue(const std::string& onTrue) { m_OnTrue = onTrue; }
        
        [[nodiscard]] const std::string& GetOnFalse() const { return m_OnFalse; }
        void SetOnFalse(const std::string& onFalse) { m_OnFalse = onFalse; }
        
        FsmState* GetCurrentState();
        std::string GetCurrentStateId() const { return m_CurrentStateId; }
        void SetCurrentState(const std::string& stateId);
        
        FsmState* GetNextState();
        [[nodiscard]] const std::string& GetNextStateId() const { return m_NextStateId; }
        void SetNextState(const std::string& stateId);

        TextEditor* GetConditionEditor() { return &m_ConditionEditor; }
        TextEditor* GetOnTrueEditor() { return &m_OnTrueEditor; }
        TextEditor* GetOnFalseEditor() { return &m_OnFalseEditor; }
        
        VisualNode* GetNode() { return &m_Node; }
        [[nodiscard]] std::string MakeIdString(const std::string& name) const;
        [[nodiscard]] ImVec2 GetPosition() { return m_Node.GetPosition(); }
        VisualNode* DrawNode();
        void DrawProperties();
        
        static std::shared_ptr<FsmTrigger> Deserialize(const nlohmann::json& json);

        nlohmann::json Serialize() const;
        
        std::string GetLuaCode(int indent = 0);

    private:
        VisualNode m_Node{};
        std::string m_Description;
        int m_Priority = 0;
        std::string m_Condition = "return false";
        TextEditor m_ConditionEditor{};
        std::string m_OnTrue;
        TextEditor m_OnTrueEditor{};
        std::string m_OnFalse;
        TextEditor m_OnFalseEditor{};
        TextEditor m_LuaCodeEditor{};
        std::unordered_map<std::string, StateData> m_Data{};
        std::string m_NextStateId;
        std::string m_CurrentStateId;
        FsmState* m_CurrentState = nullptr;
        FsmState* m_NextState = nullptr;
    };

}
