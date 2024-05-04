#include "pch.h"
#include "FsmState.h"

#include <spdlog/fmt/bundled/format.h>

#include "Graphics/Window.h"
#include "Graphics/MathHelpers.h"

namespace LuaFsm
{
    FsmState::FsmState(std::string id)
        : m_Id(std::move(id))
    {
        m_OnInitEditor.SetText(m_OnInit);
        m_OnEnterEditor.SetText(m_OnEnter);
        m_OnUpdateEditor.SetText(m_OnUpdate);
        m_OnExitEditor.SetText(m_OnExit);
        m_Node.SetId(m_Id);
        m_Node.SetType(NodeType::State);
    }

    std::unordered_map<std::string, std::string> FsmState::GetData()
    {
        for (const auto& [key, trigger] : m_Triggers)
            for (const auto& [argKey, argValue] : trigger->GetArguments())
                m_Data[argKey] = "";
        return m_Data;
    }

    void FsmState::SetId(const std::string& id)
    {
        const auto oldId = m_Id;
        m_Id = id;
        if (m_Fsm == nullptr)
            return;
        for (const auto fsm = m_Fsm; const auto& [key, value] : fsm->GetStates())
        {
            for (const auto& [argKey, argValue] : value->GetTriggers())
                if (argValue->GetNextStateId() == oldId)
                    argValue->SetNextState(m_Id);
            if (value->GetId() == oldId)
            {
                fsm->RemoveState(oldId);
                fsm->AddState(id, value);
                break;
            }
        }
    }

    void FsmState::AddTrigger(const std::string& key, const FsmTriggerPtr& value)
    {
        m_Triggers[key] = value;
        value->SetFsm(m_Fsm);
        value->SetCurrentState(m_Id);
        for (const auto& [argKey, argValue] : value->GetArguments())
            AddData(argKey, "");
        m_Fsm->AddTrigger(value.get());
    }

    FsmTriggerPtr FsmState::AddTrigger(const std::string& key)
    {
        m_Triggers[key] = std::make_shared<FsmTrigger>(key);
        const auto value = m_Triggers[key];
        value->SetFsm(m_Fsm);
        value->SetCurrentState(m_Id);
        for (const auto& [argKey, argValue] : value->GetArguments())
            AddData(argKey, "");
        m_Fsm->AddTrigger(value.get());
        return value;
    }

    void FsmState::AddTrigger(const FsmTriggerPtr& value)
    {
        const auto key = value->GetId();
        AddTrigger(key, value);
    }

    FsmTriggerPtr FsmState::GetTrigger(const std::string& key)
    {
        if (!m_Triggers.contains(key))
            return nullptr;
        return m_Triggers[key];
    }

    void FsmState::RemoveTrigger(const std::string& trigger)
    {
        const auto obj = GetTrigger(trigger);
        if (obj == nullptr)
            return;
        obj->SetCurrentState("");
        m_Triggers.erase(trigger);
    }

    std::string FsmState::GetLuaCode(int indent)
    {
        std::string indentTabs;
        for (int i = 0; i < indent; i++)
            indentTabs += "\t";
        std::string code;
        code += indentTabs + fmt::format("{0} = FSM_STATE:new({{\n", m_Id);
        code += indentTabs + fmt::format("\t---Unique Identifier of this State\n");
        code += indentTabs + fmt::format("\t---@type string\n");
        code += indentTabs + fmt::format("\tid = \"{0}\",\n", m_Id);
        code += indentTabs + fmt::format("\t---Name of this State\n");
        code += indentTabs + fmt::format("\t---@type string\n");
        code += indentTabs + fmt::format("\tname = \"{0}\",\n", m_Name);
        code += indentTabs + fmt::format("\t---Description of this State\n");
        code += indentTabs + fmt::format("\t---@type string\n");
        code += indentTabs + fmt::format("\tdescription = \"{0}\",\n", m_Description);
        code += indentTabs + fmt::format("\t---Events where the state updates\n");
        code += indentTabs + fmt::format("\t---@type table<string>\n");
        code += indentTabs + fmt::format("\tevents = {{\n");
        for (const auto& event : m_Events)
            code += indentTabs + fmt::format("\t\t\"{0}\",\n", event);
        code += indentTabs + fmt::format("\t}},\n");
        code += indentTabs + fmt::format("\t---Data the state contains\n");
        code += indentTabs + fmt::format("\t---@type table<string, any>\n");
        code += indentTabs + fmt::format("\tdata = {{\n");
        for (const auto& [key, value] : m_Data)
            code += indentTabs + fmt::format("\t\t{0} = \"{1}\",\n", key, value);
        code += indentTabs + fmt::format("\t}},\n");
        code += indentTabs + fmt::format("\t---@param self FSM_STATE\n");
        code += indentTabs + fmt::format("\tonInit = function(self)\n");
        for (const auto& line : m_OnInitEditor.GetTextLines())
            code += indentTabs + fmt::format("\t\t{0}\n", line);
        code += indentTabs + fmt::format("\tend,\n");
        code += indentTabs + fmt::format("\t---@param self FSM_STATE\n");
        code += indentTabs + fmt::format("\tonEnter = function(self)\n");
        for (const auto& line : m_OnEnterEditor.GetTextLines())
            code += indentTabs + fmt::format("\t\t{0}\n", line);
        code += indentTabs + fmt::format("\tend,\n");
        code += indentTabs + fmt::format("\t---@param self FSM_STATE\n");
        code += indentTabs + fmt::format("\t---@param eventData eventTrigger\n");
        code += indentTabs + fmt::format("\tonUpdate = function(self, eventData)\n");
        for (const auto& line : m_OnUpdateEditor.GetTextLines())
            code += indentTabs + fmt::format("\t\t{0}\n", line);
        code += indentTabs + fmt::format("\tend,\n");
        code += indentTabs + fmt::format("\t---@param self FSM_STATE\n");
        code += indentTabs + fmt::format("\tonExit = function(self)\n");
        for (const auto& line : m_OnExitEditor.GetTextLines())
            code += indentTabs + fmt::format("\t\t{0}\n", line);
        code += indentTabs + fmt::format("\tend,\n");
        code += indentTabs + fmt::format("\t---Conditions for moving to other states\n");
        code += indentTabs + fmt::format("\t---@type table<string, FSM_TRIGGER>\n");
        code += indentTabs + fmt::format("\ttriggers = {{\n");
        for (const auto& [key, value] : m_Triggers)
            code += indentTabs + value->GetLuaCode(indent + 2) + ",\n";
        code += indentTabs + fmt::format("\t}},\n");
        code += indentTabs + fmt::format("}})");
        return code;
    }

    VisualNode* FsmState::DrawStateNode(NodeEditor* editor)
    {
        if (GetPosition().x < 0)
            SetPosition(editor->GetNextNodePos());
        ImGui::SetNextWindowPos(GetPosition(), ImGuiCond_Once);
        
        m_Node.SetRadius(ImGui::CalcTextSize(m_Name.c_str()).x * 0.5f + 10.0f);
        ImGui::SetNextWindowSize(m_Node.InitSizes(), ImGuiCond_Once);
        ImGui::Begin(m_Id.c_str(), nullptr, NodeEditor::NodeWindowFlags);
        {
            const auto drawList = ImGui::GetWindowDrawList();
            m_Node.HandleSelection(editor);
            
            drawList->AddCircleFilled(m_Node.GetDrawPos(), m_Node.GetRadius(), m_Node.GetCurrentColor());
            drawList->AddCircle(m_Node.GetDrawPos(), m_Node.GetRadius(), m_Node.GetBorderColor());
            drawList->AddText(m_Node.GetTextPos(m_Name.c_str()), IM_COL32_WHITE, m_Name.c_str());
            
            SetPosition(ImGui::GetWindowPos());
            ImGui::End();
        }
        return &m_Node;
    }
    
    
    VisualNode* FsmState::DrawNode(NodeEditor* editor)
    {
        if (GetPosition().x < 0)
            SetPosition(editor->GetNextNodePos());
        ImGui::SetNextWindowPos(GetPosition(), ImGuiCond_Once);
        
        m_Node.SetRadius(ImGui::CalcTextSize(m_Name.c_str()).x * 0.5f + 10.0f);
        ImGui::SetNextWindowSize(m_Node.InitSizes(), ImGuiCond_Once);
        ImGui::Begin(m_Id.c_str(), nullptr, NodeEditor::NodeWindowFlags);
        {
            const auto drawList = ImGui::GetWindowDrawList();
            m_Node.HandleSelection(editor);
            
            drawList->AddCircleFilled(m_Node.GetDrawPos(), m_Node.GetRadius(), m_Node.GetCurrentColor());
            drawList->AddCircle(m_Node.GetDrawPos(), m_Node.GetRadius(), m_Node.GetBorderColor());
            drawList->AddText(m_Node.GetTextPos(m_Name.c_str()), IM_COL32_WHITE, m_Name.c_str());
            
            SetPosition(ImGui::GetWindowPos());
            ImGui::End();
        }
        return &m_Node;
    }

    
    //void FsmState::DrawNode(int id)
    //{
    //    ImNodes::BeginNode(id);
    //    {
    //        ImNodes::BeginNodeTitleBar();
    //        {
    //            ImGui::Text(m_Name.c_str());
    //            ImNodes::EndNodeTitleBar();
    //        }
    //        //ImGui::TextWrapped(m_Description.c_str());
    //        ImNodes::BeginInputAttribute(id + 1);
    //        {
    //            ImGui::Text("Enter condition");
    //            ImNodes::EndInputAttribute();
    //        }
    //        ImNodes::BeginOutputAttribute(id + 2);
    //        {
    //            ImGui::Text("Leave Condition");
    //            ImNodes::EndOutputAttribute();
    //        }
    //        ImNodes::EndNode();
    //    }
    //}
}
