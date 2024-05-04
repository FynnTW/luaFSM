#pragma once
#include <memory>
#include <string>

#include "FsmTrigger.h"
#include "Fsm.h"
#include "Graphics/MathHelpers.h"
#include "Graphics/VisualNode.h"
#include "imgui/NodeEditor.h"
#include "imgui/TextEditor.h"
#include "imgui/imnodes/imnodes.h"

namespace LuaFsm
{
    class Fsm;
    class FsmTrigger;
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
        [[nodiscard]] std::unordered_map<std::string, std::string> GetData();
        [[nodiscard]] std::unordered_map<std::string, FsmTriggerPtr> GetTriggers() const { return m_Triggers; }
        [[nodiscard]] Fsm* GetFsm() const { return m_Fsm; }
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
        FsmTriggerPtr AddTrigger(const std::string& key);
        void AddTrigger(const FsmTriggerPtr& value);
        FsmTriggerPtr GetTrigger(const std::string& key);
        void SetFsm(Fsm* fsm) { m_Fsm = fsm; }
        void AddEvent(const std::string& event) { m_Events.push_back(event); }
        void RemoveEvent(const std::string& event) { std::erase(m_Events, event); }
        void ClearEvents() { m_Events.clear(); }
        void ClearData() { m_Data.clear(); }
        void ClearTriggers() { m_Triggers.clear(); }
        void RemoveTrigger(const std::string& trigger);
        void RemoveData(const std::string& key) { m_Data.erase(key); }
        std::string GetLuaCode(int indent = 0);
        VisualNode* DrawStateNode(NodeEditor* editor);
        [[nodiscard]] ImVec2 GetPosition() const { return m_Node.GetPosition(); }
        void SetPosition(const ImVec2& position) { m_Node.SetPosition(position); }
        VisualNode* DrawNode(NodeEditor* editor);
        VisualNode* GetNode() { return &m_Node; }
        TextEditor* GetOnInitEditor() { return &m_OnInitEditor; }
        TextEditor* GetOnEnterEditor() { return &m_OnEnterEditor; }
        TextEditor* GetOnUpdateEditor() { return &m_OnUpdateEditor; }
        TextEditor* GetOnExitEditor() { return &m_OnExitEditor; }
        
        
        
    private:
        std::string m_Name;
        VisualNode m_Node{};
        std::string m_Id;
        std::string m_Description;
        std::string m_OnInit;
        TextEditor m_OnInitEditor{};
        std::string m_OnEnter;
        TextEditor m_OnEnterEditor{};
        std::string m_OnUpdate;
        TextEditor m_OnUpdateEditor{};
        std::string m_OnExit;
        TextEditor m_OnExitEditor{};
        std::unordered_map<std::string, std::string> m_Data{};
        std::unordered_map<std::string, std::shared_ptr<FsmTrigger>> m_Triggers{};
        std::vector<std::string> m_TriggersIds{};
        Fsm* m_Fsm;
        std::vector<std::string> m_Events{};
    };
}
