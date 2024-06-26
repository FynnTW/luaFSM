﻿#pragma once
#include <memory>
#include <string>

#include "imgui.h"
#include "json.hpp"
#include "Graphics/VisualNode.h"
#include "imgui/TextEditor.h"
#include "imgui/popups/Popup.h"

namespace LuaFsm
{
    
    enum class TriggerPopups
    {
        UnlinkNextState,
        UnlinkCurrentState,
        DeleteTrigger,
        SetNewId
    };
    
    class FsmState;
    class Fsm;
    typedef std::shared_ptr<FsmState> FsmStatePtr;
    class FsmTrigger : DrawableObject
    {
    public:
        FsmTrigger(const std::string& id);
        
        FsmTrigger(const FsmTrigger& other);
        
        void UpdateEditors();
        void InitPopups();

        bool operator==(const FsmTrigger& other) const
        {
            return m_Id == other.GetId()
            && m_Name == other.GetName()
            && m_Condition == other.GetCondition()
            && m_Action == other.GetAction()
            && m_CurrentStateId == other.GetCurrentStateId()
            && m_NextStateId == other.GetNextStateId()     
            && m_Node.GetGridPos() == other.GetNode().GetGridPos()
            && m_Node.GetColor() == static_cast<ImVec4>(other.GetNode().GetColor());
        }

        bool operator==(FsmTrigger& other) const
        {
            return m_Id == other.GetId()
            && m_Name == other.GetName()
            && m_Condition == other.GetCondition()
            && m_Action == other.GetAction()
            && m_CurrentStateId == other.GetCurrentStateId()
            && m_NextStateId == other.GetNextStateId()     
            && m_Node.GetGridPos() == other.GetNode()->GetGridPos()
            && m_Node.GetColor() == static_cast<ImVec4>(other.GetNode()->GetColor());
        }
        
#define COMPARE_FLOATS(a, b) (fabs(a - b) < 0.0001)
        
        bool operator!=(const FsmTrigger& other) const
        {
            return m_Id != other.GetId()
            || m_Name != other.GetName()
            || m_Condition != other.GetCondition()
            || m_Action != other.GetAction()
            || m_CurrentStateId != other.GetCurrentStateId()
            || m_NextStateId != other.GetNextStateId()     
            || m_Node.GetGridPos() != other.GetNode().GetGridPos()
            || !COMPARE_FLOATS(m_Node.GetInArrowCurve(), other.GetNode().GetInArrowCurve())
            || !COMPARE_FLOATS(m_Node.GetOutArrowCurve(), other.GetNode().GetOutArrowCurve())
            || m_Node.GetColor() != static_cast<ImVec4>(other.GetNode().GetColor());
        }
        
        bool operator!=(FsmTrigger& other) const
        {
            return m_Id != other.GetId()
            || m_Name != other.GetName()
            || m_Condition != other.GetCondition()
            || m_Action != other.GetAction()
            || m_CurrentStateId != other.GetCurrentStateId()
            || m_NextStateId != other.GetNextStateId()     
            || m_Node.GetGridPos() != other.GetNode()->GetGridPos()
            || !COMPARE_FLOATS(m_Node.GetInArrowCurve(), other.GetNode()->GetInArrowCurve())
            || !COMPARE_FLOATS(m_Node.GetOutArrowCurve(), other.GetNode()->GetOutArrowCurve())
            || m_Node.GetColor() != static_cast<ImVec4>(other.GetNode()->GetColor());
        }

        [[nodiscard]] bool IsUnSaved() const { return m_UnSaved; }
        void SetUnSaved(const bool unSaved) { m_UnSaved = unSaved; }
        
        static std::vector<std::shared_ptr<FsmTrigger>> CreateFromFile(const std::string& filePath);
        void UpdateFromFile(const std::string& filePath);
        void UpdateToFile(const std::string& oldId);
        void RefactorId(const std::string& newId);

        [[nodiscard]] virtual std::string GetId() const override { return m_Id; }
        void virtual SetId(const std::string& id) override;
        
        [[nodiscard]] virtual std::string GetName() const override { return m_Name; }
        void virtual SetName(const std::string& name) override { m_Name = name; }
        
        [[nodiscard]] const std::string& GetDescription() const { return m_Description; }
        void SetDescription(const std::string& description) { m_Description = description; }
        
        [[nodiscard]] int GetPriority() const { return m_Priority; }
        void SetPriority(const int priority) { m_Priority = priority; }
        
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
        void UpdateFileContents(std::string& code, const std::string& oldId);

        TextEditor* GetConditionEditor() { return &m_ConditionEditor; }
        TextEditor* GetActionEditor() { return &m_ActionEditor; }
        
        VisualNode* GetNode() { return &m_Node; }
        [[nodiscard]] VisualNode GetNode() const { return m_Node; }
        VisualNode* DrawNode();
        void DrawProperties();
        void CreateLastState();
        bool IsChanged();
        void AppendToFile();
        
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
        std::string m_NextStateId;
        std::string m_CurrentStateId;
        PopupManager m_PopupManager;
        std::shared_ptr<FsmTrigger> m_PreviousState = nullptr;
        bool m_UnSaved = false;
        FsmState* m_CurrentState = nullptr;
        FsmState* m_NextState = nullptr;
    };

}
