#include "pch.h"
#include "FsmTrigger.h"

#include <spdlog/fmt/bundled/format.h>
#include <data/FsmState.h>

#include "imgui/imnodes/imnodes.h"

namespace LuaFsm
{
    FsmTrigger::FsmTrigger(std::string id)
        : m_Id(std::move(id))
    {
        m_ConditionEditor.SetText(m_Condition);
        m_OnTrueEditor.SetText(m_OnTrue);
        m_OnFalseEditor.SetText(m_OnFalse);
        m_Node.SetId(m_Id);
        m_Node.SetType(NodeType::Transition);
    }

    void FsmTrigger::SetNextState(const std::string& stateId)
    {
        m_NextStateId = stateId;
        if (m_Fsm)
            m_NextState = m_Fsm->GetState(stateId).get();
    }

    void FsmTrigger::SetCurrentState(const std::string& stateId)
    {
        m_CurrentStateId = stateId;
        if (m_Fsm)
            m_CurrentState = m_Fsm->GetState(stateId).get();
    }

    VisualNode* FsmTrigger::DrawNode(NodeEditor* editor)
    {
        if (GetPosition().x < 0)
            SetPosition(editor->GetNextNodePos());
        ImGui::SetNextWindowPos(GetPosition(), ImGuiCond_Once);
        
        ImGui::SetNextWindowSize(m_Node.InitSizesDiamond(m_Name), ImGuiCond_Once);
        ImGui::Begin(m_Id.c_str(), nullptr, NodeEditor::NodeWindowFlags);
        {
            const auto drawList = ImGui::GetWindowDrawList();
            m_Node.HandleSelection(editor);

            const auto center = m_Node.GetDrawPos();
            const float width = m_Node.GetSize().x;
            const float height = m_Node.GetSize().y;
            const auto top = ImVec2(center.x, center.y - height / 2 + 3.0f);
            const auto bottom = ImVec2(center.x, center.y + height / 2 - 3.0f);
            const auto left = ImVec2(center.x - width / 2 + 3.0f, center.y);
            const auto right = ImVec2(center.x + width / 2 - 3.0f, center.y);
            drawList->AddQuadFilled(left, top, right, bottom, m_Node.GetCurrentColor());
            drawList->AddQuad(left, top, right, bottom, m_Node.GetBorderColor());
            
            drawList->AddText(m_Node.GetTextPos(m_Name.c_str()), IM_COL32_WHITE, m_Name.c_str());
            
            SetPosition(ImGui::GetWindowPos());
            ImGui::End();
        }
        return &m_Node;
    }

    std::string FsmTrigger::GetLuaCode(int indent)
    {
        std::string indentTabs;
        for (int i = 0; i < indent; i++)
            indentTabs += "\t";
        std::string code;
        code += indentTabs + fmt::format("{0} = FSM_TRIGGER:new({{\n", m_Id);
        code += indentTabs + fmt::format("\t---Unique Identifier of this Trigger\n");
        code += indentTabs + fmt::format("\t---@type string\n");
        code += indentTabs + fmt::format("\tid = \"{0}\",\n", m_Id);
        code += indentTabs + fmt::format("\t---Name of this Trigger\n");
        code += indentTabs + fmt::format("\t---@type string\n");
        code += indentTabs + fmt::format("\tname = \"{0}\",\n", m_Name);
        code += indentTabs + fmt::format("\t---Description of this Trigger\n");
        code += indentTabs + fmt::format("\t---@type string\n");
        code += indentTabs + fmt::format("\tdescription = \"{0}\",\n", m_Description);
        code += indentTabs + fmt::format("\t---Priority of this Trigger\n");
        code += indentTabs + fmt::format("\t---@type integer\n");
        code += indentTabs + fmt::format("\tpriority = {0},\n", m_Priority);
        code += indentTabs + fmt::format("\t---Name of the state this trigger leads to\n");
        code += indentTabs + fmt::format("\t---@type string\n");
        code += indentTabs + fmt::format("\tnextStateId = \"{0}\",\n", m_NextStateId);
        code += indentTabs + fmt::format("\t---Data you can utilize inside the functions\n");
        code += indentTabs + fmt::format("\t---@type table<string, any>\n");
        code += indentTabs + fmt::format("\targuments = {{\n");
        for (const auto& [key, value] : m_Arguments)
            code += indentTabs + fmt::format("\t\t{0} = \"{1}\",\n", key, value);
        code += indentTabs + fmt::format("\t}},\n");
        code += indentTabs + fmt::format("\t---@param self FSM_TRIGGER\n");
        code += indentTabs + fmt::format("\t---@return boolean isTrue\n");
        code += indentTabs + fmt::format("\tcondition = function(self)\n");
        code += indentTabs + fmt::format("\t\t{0}\n", m_Condition);
        code += indentTabs + fmt::format("\tend,\n");
        code += indentTabs + fmt::format("\t---@param self FSM_TRIGGER\n");
        code += indentTabs + fmt::format("\tonTrue = function(self)\n");
        code += indentTabs + fmt::format("\t\t{0}\n", m_OnTrue);
        code += indentTabs + fmt::format("\tend,\n");
        code += indentTabs + fmt::format("\t---@param self FSM_TRIGGER\n");
        code += indentTabs + fmt::format("\tonFalse = function(self)\n");
        code += indentTabs + fmt::format("\t\t{0}\n", m_OnFalse);
        code += indentTabs + fmt::format("\tend,\n");
        code += indentTabs + fmt::format("}})");
        return code;
    }
}
