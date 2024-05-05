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
    class FsmState : DrawableObject
    {
    public:
        FsmState(const std::string& id);
        [[nodiscard]] std::string GetDescription() const { return m_Description; }
        [[nodiscard]] std::string GetOnInit() const { return m_OnInit; }
        [[nodiscard]] std::string GetOnEnter() const { return m_OnEnter; }
        [[nodiscard]] std::string GetOnUpdate() const { return m_OnUpdate; }
        [[nodiscard]] std::vector<std::pair<std::string, std::string>> GetOnUpdateArguments() const { return m_OnUpdateArguments; }
        [[nodiscard]] std::string GetOnExit() const { return m_OnExit; }
        [[nodiscard]] std::unordered_map<std::string, std::string> GetData();
        [[nodiscard]] std::unordered_map<std::string, FsmTriggerPtr> GetTriggers() const { return m_Triggers; }
        [[nodiscard]] std::vector<std::string> GetEvents() const { return m_Events; }

        void AddOnUpdateArgument(const std::string& key, const std::string& value) { m_OnUpdateArguments.emplace_back(
            key, value); }
        void RemoveOnUpdateArgument(const std::string& key)
        {
            if (std::ranges::find_if(m_OnUpdateArguments, [key](const auto& pair) { return pair.first == key; }) != m_OnUpdateArguments.end())
                m_OnUpdateArguments.erase(std::ranges::find_if(m_OnUpdateArguments, [key](const auto& pair) { return pair.first == key; }));
        }
        void SetDescription(const std::string& description) { m_Description = description; }
        void SetOnInit(const std::string& onInit) { m_OnInit = onInit; }
        void SetOnEnter(const std::string& onEnter) { m_OnEnter = onEnter; }
        void SetOnUpdate(const std::string& onUpdate) { m_OnUpdate = onUpdate; }
        void SetOnExit(const std::string& onExit) { m_OnExit = onExit; }
        void AddData(const std::string& key, const std::string& value) { m_Data[key] = value; }
        void AddTrigger(const std::string& key, const FsmTriggerPtr& value);
        FsmTriggerPtr AddTrigger(const std::string& key);
        [[nodiscard]] std::string MakeIdString(const std::string& name) const;
        void DrawProperties();
        void AddTrigger(const FsmTriggerPtr& value);
        FsmTriggerPtr GetTrigger(const std::string& key);
        void AddEvent(const std::string& event) { m_Events.push_back(event); }
        void RemoveEvent(const std::string& event)
        {
            if (std::ranges::find(m_Events, event) != m_Events.end())
                std::erase(m_Events, event);
        }
        void ClearEvents() { m_Events.clear(); }
        void ClearData() { m_Data.clear(); }
        void ClearTriggers() { m_Triggers.clear(); }
        void RemoveTrigger(const std::string& trigger);
        void RemoveData(const std::string& key) { m_Data.erase(key); }
        std::string GetLuaCode(int indent = 0);
        [[nodiscard]] ImVec2 GetPosition() { return m_Node.GetPosition(); }
        VisualNode* DrawNode();
        VisualNode* GetNode() { return &m_Node; }
        TextEditor* GetOnInitEditor() { return &m_OnInitEditor; }
        TextEditor* GetOnEnterEditor() { return &m_OnEnterEditor; }
        TextEditor* GetOnUpdateEditor() { return &m_OnUpdateEditor; }
        TextEditor* GetOnExitEditor() { return &m_OnExitEditor; }
        [[nodiscard]] std::string GetName() const override { return m_Name; }
        [[nodiscard]] std::string GetId() const override { return m_Id; }
        void virtual SetName(const std::string& name) override { m_Name = name; }
        void virtual SetId(const std::string& id) override;
        void ChangeTriggerId(const std::string& oldId, const std::string& newId);

    private:
        VisualNode m_Node{};
        std::string m_Description;
        std::string m_OnInit;
        TextEditor m_OnInitEditor{};
        std::string m_OnEnter;
        TextEditor m_OnEnterEditor{};
        std::string m_OnUpdate;
        TextEditor m_OnUpdateEditor{};
        TextEditor m_LuaCodeEditor{};
        std::vector<std::pair<std::string, std::string>> m_OnUpdateArguments{};
        std::string m_OnExit;
        TextEditor m_OnExitEditor{};
        std::unordered_map<std::string, std::string> m_Data{};
        std::unordered_map<std::string, std::shared_ptr<FsmTrigger>> m_Triggers{};
        std::vector<std::string> m_TriggersIds{};
        std::vector<std::string> m_Events{};
    };
}
