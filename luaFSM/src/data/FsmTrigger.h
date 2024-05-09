﻿#pragma once
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
        void UpdateEditors();
        static std::vector<std::shared_ptr<FsmTrigger>> CreateFromFile(const std::string& filePath);
        std::string GetExportLuaCode();
        void UpdateFromFile(const std::string& filePath);

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
        
        [[nodiscard]] const std::string& GetAction() const { return m_Action; }
        void SetAction(const std::string& onTrue) { m_Action = onTrue; }
        
        FsmState* GetCurrentState();
        std::string GetCurrentStateId() const { return m_CurrentStateId; }
        void SetCurrentState(const std::string& stateId);
        
        FsmState* GetNextState();
        [[nodiscard]] const std::string& GetNextStateId() const { return m_NextStateId; }
        void SetNextState(const std::string& stateId);
        void UpdateFileContents(std::string& code);

        TextEditor* GetConditionEditor() { return &m_ConditionEditor; }
        TextEditor* GetActionEditor() { return &m_ActionEditor; }
        
        VisualNode* GetNode() { return &m_Node; }
        [[nodiscard]] std::string MakeIdString(const std::string& name) const;
        [[nodiscard]] ImVec2 GetPosition() { return m_Node.GetPosition(); }
        VisualNode* DrawNode();
        void DrawProperties();
        
        static std::shared_ptr<FsmTrigger> Deserialize(const nlohmann::json& json);

        nlohmann::json Serialize() const;
        
        std::string GetLuaCode();

    private:
        VisualNode m_Node{};
        std::string m_Description;
        int m_Priority = 0;
        std::string m_Condition = "return false";
        TextEditor m_ConditionEditor{};
        std::string m_Action;
        TextEditor m_ActionEditor{};
        TextEditor m_LuaCodeEditor{};
        std::unordered_map<std::string, StateData> m_Data{};
        std::string m_NextStateId;
        std::string m_CurrentStateId;
        FsmState* m_CurrentState = nullptr;
        FsmState* m_NextState = nullptr;
    };

}
