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
        [[nodiscard]] std::string GetOnEnter() const { return m_OnEnter; }
        [[nodiscard]] std::string GetOnUpdate() const { return m_OnUpdate; }
        [[nodiscard]] std::string GetOnExit() const { return m_OnExit; }
        [[nodiscard]] std::unordered_map<std::string, FsmTriggerPtr> GetTriggers() const { return m_Triggers; }
        
        void SetDescription(const std::string& description) { m_Description = description; }
        void SetOnEnter(const std::string& onEnter) { m_OnEnter = onEnter; }
        void SetOnUpdate(const std::string& onUpdate) { m_OnUpdate = onUpdate; }
        void SetOnExit(const std::string& onExit) { m_OnExit = onExit; }
        void AddTrigger(const std::string& key, const FsmTriggerPtr& value);
        FsmTriggerPtr AddTrigger(const std::string& key);
        [[nodiscard]] std::string MakeIdString(const std::string& name) const;
        static std::vector<std::shared_ptr<FsmState>> CreateFromFile(const std::string& filePath);
        void UpdateFromFile(const std::string& filePath);
        void UpdateFileContents(std::string& code);
        void UpdateToFile();
        void DrawProperties();
        void AddTrigger(const FsmTriggerPtr& value);
        FsmTriggerPtr GetTrigger(const std::string& key);
        void ClearTriggers() { m_Triggers.clear(); }
        void RemoveTrigger(const std::string& trigger);
        std::string GetExportLuaCode();
        std::string GetLuaCode();
        [[nodiscard]] ImVec2 GetPosition() { return m_Node.GetPosition(); }
        VisualNode* DrawNode();
        VisualNode* GetNode() { return &m_Node; }
        TextEditor* GetOnEnterEditor() { return &m_OnEnterEditor; }
        TextEditor* GetOnUpdateEditor() { return &m_OnUpdateEditor; }
        TextEditor* GetOnExitEditor() { return &m_OnExitEditor; }
        [[nodiscard]] std::string GetName() const override { return m_Name; }
        [[nodiscard]] std::string GetId() const override { return m_Id; }
        void virtual SetName(const std::string& name) override { m_Name = name; }
        void virtual SetId(const std::string& id) override;
        nlohmann::json Serialize();
        void ChangeTriggerId(const std::string& oldId, const std::string& newId);
        static std::shared_ptr<FsmState> Deserialize(const nlohmann::json& json);
        void UpdateEditors();

    private:
        VisualNode m_Node{};
        std::string m_Description;
        std::string m_OnEnter;
        TextEditor m_OnEnterEditor{};
        std::string m_OnUpdate;
        TextEditor m_OnUpdateEditor{};
        TextEditor m_LuaCodeEditor{};
        std::string m_OnExit;
        TextEditor m_OnExitEditor{};
        std::unordered_map<std::string, std::shared_ptr<FsmTrigger>> m_Triggers{};
    };
}
