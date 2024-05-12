#pragma once
#include <memory>
#include <string>

#include "FsmTrigger.h"
#include "Fsm.h"
#include "Graphics/VisualNode.h"
#include "imgui/TextEditor.h"

namespace LuaFsm
{
    class Fsm;
    class FsmTrigger;
    typedef std::shared_ptr<FsmTrigger> FsmTriggerPtr;
    typedef std::shared_ptr<Fsm> FsmPtr;

    enum class StatePopups
    {
        UnlinkTrigger,
        DeleteState,
        SetNewId
    };
    
    class FsmState final : DrawableObject
    {
    public:
        explicit FsmState(const std::string& id);
        
        FsmState(const FsmState& other);
        
        void InitPopups();

        bool operator==(const FsmState& other)
        {
            for (const auto& key : m_Triggers | std::views::keys)
                if (!other.GetTriggers().contains(key))
                    return false;
            for (const auto& key : other.GetTriggers() | std::views::keys)
                if (!m_Triggers.contains(key))
                    return false;
            return m_Id == other.GetId() &&
            m_Name == other.GetName() &&
            m_OnEnter == other.GetOnEnter() &&
            m_OnUpdate == other.GetOnUpdate() &&
            m_OnExit == other.GetOnExit() &&
            m_Description == other.GetDescription() &&
            m_IsExitState == other.IsExitState() &&
            m_Node.GetGridPos() == other.GetNode().GetGridPos() &&
            m_Node.GetColor() == static_cast<ImVec4>(other.GetNode().GetColor());
        }

        bool operator==(FsmState& other)
        {
            for (const auto& key : m_Triggers | std::views::keys)
                if (!other.GetTriggers().contains(key))
                    return false;
            for (const auto& key : other.GetTriggers() | std::views::keys)
                if (!m_Triggers.contains(key))
                    return false;
            return m_Id == other.GetId() &&
            m_Name == other.GetName() &&
            m_OnEnter == other.GetOnEnter() &&
            m_OnUpdate == other.GetOnUpdate() &&
            m_OnExit == other.GetOnExit() &&
            m_Description == other.GetDescription() &&
            m_IsExitState == other.IsExitState() &&
            m_Node.GetGridPos() == other.GetNode()->GetGridPos() &&
            m_Node.GetColor() == static_cast<ImVec4>(other.GetNode()->GetColor());
        }

        bool operator!=(const FsmState& other) const
        {
            for (const auto& key : m_Triggers | std::views::keys)
                if (!other.GetTriggers().contains(key))
                    return false;
            for (const auto& key : other.GetTriggers() | std::views::keys)
                if (!m_Triggers.contains(key))
                    return false;
            return m_Id == other.GetId() ||
            m_Name == other.GetName() ||
            m_OnEnter == other.GetOnEnter() ||
            m_OnUpdate == other.GetOnUpdate() ||
            m_OnExit == other.GetOnExit() ||
            m_Description == other.GetDescription() ||
            m_IsExitState == other.IsExitState() ||
            m_Node.GetGridPos() == other.GetNode().GetGridPos() ||
            m_Node.GetColor() == static_cast<ImVec4>(other.GetNode().GetColor());
        }

        bool operator!=(FsmState& other) const
        {
            for (const auto& key : m_Triggers | std::views::keys)
                if (!other.GetTriggers().contains(key))
                    return true;
            for (const auto& key : other.GetTriggers() | std::views::keys)
                if (!m_Triggers.contains(key))
                    return true;
            return m_Id != other.GetId() ||
            m_Name != other.GetName() ||
            m_OnEnter != other.GetOnEnter() ||
            m_OnUpdate != other.GetOnUpdate() ||
            m_OnExit != other.GetOnExit() ||
            m_Description != other.GetDescription() ||
            m_IsExitState != other.IsExitState() ||
            m_Node.GetGridPos() != other.GetNode()->GetGridPos() ||
            m_Node.GetColor() != static_cast<ImVec4>(other.GetNode()->GetColor());
        }

        [[nodiscard]] bool IsUnSaved() const { return m_UnSaved; }
        void SetUnSaved(const bool unSaved) { m_UnSaved = unSaved; }
        
        void SetName(const std::string& name) override { m_Name = name; }
        void SetId(const std::string& id) override;
        void RefactorId(const std::string& newId);
        
        [[nodiscard]] std::string GetDescription() const { return m_Description; }
        void SetDescription(const std::string& description) { m_Description = description; }
        
        [[nodiscard]] std::string GetOnEnter() const { return m_OnEnter; }
        void SetOnEnter(const std::string& onEnter) { m_OnEnter = onEnter; }
        
        [[nodiscard]] std::string GetOnUpdate() const { return m_OnUpdate; }
        void SetOnUpdate(const std::string& onUpdate) { m_OnUpdate = onUpdate; }
        
        [[nodiscard]] std::string GetOnExit() const { return m_OnExit; }
        void SetOnExit(const std::string& onExit) { m_OnExit = onExit; }
        
        [[nodiscard]] std::unordered_map<std::string, FsmTriggerPtr> GetTriggers() const { return m_Triggers; }
        void AddTrigger(const std::string& key, const FsmTriggerPtr& value);
        void AddTrigger(const FsmTriggerPtr& value);
        FsmTriggerPtr AddTrigger(const std::string& key);
        FsmTriggerPtr GetTrigger(const std::string& key);
        void ClearTriggers() { m_Triggers.clear(); }
        void RemoveTrigger(const std::string& trigger);
        
        static std::vector<std::shared_ptr<FsmState>> CreateFromFile(const std::string& filePath);
        void UpdateFromFile(const std::string& filePath);
        void UpdateFileContents(std::string& code, const std::string& oldId);
        void UpdateToFile(const std::string& oldId);
        void AppendToFile();
        
        void DrawProperties();
        void CreateLastState();
        bool IsChanged();
        std::string GetLuaCode();
        
        VisualNode* DrawNode();
        VisualNode* GetNode() { return &m_Node; }
        VisualNode GetNode() const { return m_Node; }
        TextEditor* GetOnEnterEditor() { return &m_OnEnterEditor; }
        TextEditor* GetOnUpdateEditor() { return &m_OnUpdateEditor; }
        TextEditor* GetOnExitEditor() { return &m_OnExitEditor; }
        [[nodiscard]] std::string GetName() const override { return m_Name; }
        [[nodiscard]] std::string GetId() const override { return m_Id; }
        void ChangeTriggerId(const std::string& oldId, const std::string& newId);
        void UpdateEditors();
        void SetExitState(const bool isExitState) { m_IsExitState = isExitState; }
        [[nodiscard]] bool IsExitState() const { return m_IsExitState; }

    private:
        VisualNode m_Node{};
        std::string m_Description;
        std::string m_OnEnter;
        TextEditor m_OnEnterEditor{};
        std::string m_OnUpdate;
        TextEditor m_OnUpdateEditor{};
        std::string m_OnExit;
        TextEditor m_OnExitEditor{};
        TextEditor m_LuaCodeEditor{};
        bool m_UnSaved = false;
        PopupManager m_PopupManager{};
        std::shared_ptr<FsmState> m_PreviousState = nullptr;
        bool m_IsExitState = false;
        std::unordered_map<std::string, std::shared_ptr<FsmTrigger>> m_Triggers{};
    };
}
