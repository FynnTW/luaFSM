#include "pch.h"
#include "FsmTrigger.h"

#include <spdlog/fmt/bundled/format.h>
#include <data/FsmState.h>

#include "imgui_internal.h"
#include "imgui/imgui_stdlib.h"
#include "imgui/NodeEditor.h"

#include "Graphics/Window.h"

namespace LuaFsm
{
    FsmTrigger::FsmTrigger(const std::string& id)
    {
        FsmTrigger::SetId(id);
        //m_ConditionEditor.SetPalette(Window::GetPalette());
        m_ConditionEditor.SetText(m_Condition);
        m_ConditionEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        //m_OnTrueEditor.SetPalette(Window::GetPalette());
        m_OnTrueEditor.SetText(m_OnTrue);
        m_OnTrueEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        //m_OnFalseEditor.SetPalette(Window::GetPalette());
        m_OnFalseEditor.SetText(m_OnFalse);
        m_OnFalseEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        m_LuaCodeEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        m_Node.SetId(m_Id);
        m_Node.SetColor(IM_COL32(75, 75, 0, 150));
        m_Node.SetHighlightColor(m_Node.GetHighlightColor());
        m_Node.SetHighlightColorSelected(m_Node.GetHighlightColorSelected());
        m_Node.SetBorderColor(IM_COL32(255, 255, 255, 255));
        m_Node.SetType(NodeType::Transition);
        m_Node.SetShape(NodeShape::Circle);
    }

    FsmState* FsmTrigger::GetCurrentState()
    {
        if (m_CurrentState)
            return m_CurrentState;
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
            return nullptr;
        const auto currentState = fsm->GetState(m_CurrentStateId);
        if (currentState)
        {
            m_CurrentState = currentState.get();
            return m_CurrentState;
        }
        return nullptr;
    }

    FsmState* FsmTrigger::GetNextState()
    {
        if (m_NextState)
            return m_NextState;
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
            return nullptr;
        if (const auto nextState = fsm->GetState(m_NextStateId))
        {
            m_NextState = nextState.get();
            return m_NextState;
        }
        return nullptr;
    }

    void FsmTrigger::SetNextState(const std::string& stateId)
    {
        this->m_NextStateId = stateId;
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
            return;
        if (fsm)
            m_NextState = fsm->GetState(stateId).get();
    }

    void FsmTrigger::SetCurrentState(const std::string& stateId)
    {
        this->m_CurrentStateId = stateId;
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
            return;
        if (fsm)
            m_CurrentState = fsm->GetState(stateId).get();
    }

    VisualNode* FsmTrigger::DrawNode()
    {
        return m_Node.Draw(this);
    }

    void FsmTrigger::SetId(const std::string& id)
    {
        const auto oldId = m_Id;
        m_Id = id;
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (fsm == nullptr)
            return;
        if (!oldId.empty())
        {
            for (const auto& [key, value] : fsm->GetStates())
            {
                value->ChangeTriggerId(oldId, id);
            }
            fsm->ChangeTriggerId(oldId, id);
        }
        m_Node.SetId(id);
    }
    
    std::string FsmTrigger::MakeIdString(const std::string& name) const
    {
        return name + "##" + m_Id;
    }

    bool IS_ADD_TRIGGER_ARGUMENT_OPEN = false;
    bool UNLINK_CURRENT_STATE = false;
    bool UNLINK_NEXT_STATE = false;
    bool ADD_CURRENT_STATE = false;
    bool ADD_NEXT_STATE = false;
    bool DELETE_TRIGGER = false;

    void FsmTrigger::DrawProperties()
    {
        std::string id = GetId();
        if (ImGui::Button(MakeIdString("Refresh").c_str()) || m_LuaCodeEditor.GetText().empty())
            m_LuaCodeEditor.SetText(GetLuaCode());
        if (ImGui::BeginTabBar(MakeIdString("Node Properties").c_str()))
        {
            if (ImGui::BeginTabItem(MakeIdString("Properties").c_str()))
            {
                if (ImGui::Button(MakeIdString("Delete").c_str()))
                    DELETE_TRIGGER = true;
                ImGui::Text("ID");
                std::string idLabel = "##ID" + GetId();
                ImGui::InputText(idLabel.c_str(), &id);
                ImGui::Text("Name");
                std::string name = GetName();
                std::string nameLabel = "##Name" + GetId();
                ImGui::InputText(nameLabel.c_str(), &name);
                SetName(name);
                ImGui::Text("Description");
                std::string description = GetDescription();
                std::string descriptionLabel = "##Description" + GetId();
                ImGui::InputText(descriptionLabel.c_str(), &description);
                SetDescription(description);
                ImGui::Text("Priority");
                ImGui::InputInt(MakeIdString("Priority").c_str(), &m_Priority);
                ImGui::Text("Arguments");
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Add Argument").c_str()))
                {
                    IS_ADD_TRIGGER_ARGUMENT_OPEN = true;
                }
                if (ImGui::BeginTable(MakeIdString("Arguments").c_str(), 4,
                    ImGuiTableFlags_BordersInnerV
                    | ImGuiTableFlags_BordersOuter
                    | ImGuiTableFlags_SizingFixedFit
                    | ImGuiTableFlags_Resizable
                    | ImGuiTableFlags_Reorderable
                    | ImGuiTableFlags_RowBg))
                {
                    int index = 0;
                    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 200.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 200.0f);
                    ImGui::TableSetupColumn("Remove", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Copy", ImGuiTableColumnFlags_WidthFixed);
                    
                    for (auto& [key, value] : GetArguments())
                    {
                        ImGui::TableNextRow();
                        std::string keyId = "##Key" + std::to_string(index) + GetId();
                        std::string valueId = "##Value" + std::to_string(index) + GetId();
                        std::string keyX = key;
                        std::string valueX = value;

            
                        // Key column
                        ImGui::TableSetColumnIndex(0);
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::InputText(keyId.c_str(), &keyX);

                        // Value column
                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::InputText(valueId.c_str(), &valueX);

                        ImGui::TableSetColumnIndex(2);
                        std::string removeId = "Remove## " + key + std::to_string(index) + GetId();
                        if (ImGui::Button(removeId.c_str()))
                        {
                            m_Arguments.erase(key);
                            break;
                        }
                        
                        ImGui::TableSetColumnIndex(3);
                        std::string copyId = "Copy## " + key + std::to_string(index) + GetId();
                        if (ImGui::Button(copyId.c_str()))
                        {
                            std::string copyString = "self.arguments." + key;
                            ImGui::SetClipboardText(copyString.c_str());
                        }
                        index++;
                    }
                    ImGui::EndTable();
                }
                ImGui::Text("Current State: ");
                ImGui::SameLine();
                if (GetCurrentState())
                {
                    ImGui::Selectable(MakeIdString(m_CurrentStateId).c_str(), false);
                    if (ImGui::IsItemClicked())
                        NodeEditor::Get()->SetSelectedNode(GetCurrentState()->GetNode());
                    if (ImGui::IsItemHovered())
                    {
                        GetCurrentState()->GetNode()->HighLight();
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                        {
                            UNLINK_CURRENT_STATE = true;
                        }
                    }
                }
                else if (ImGui::Button(MakeIdString("Add Current State").c_str()))
                    ADD_CURRENT_STATE = true;
                ImGui::Separator();
                ImGui::Text("Next State: ");
                ImGui::SameLine();
                if (GetNextState())
                {
                    ImGui::Selectable(MakeIdString(m_NextStateId).c_str(), false);
                    if (ImGui::IsItemClicked())
                        NodeEditor::Get()->SetSelectedNode(GetNextState()->GetNode());
                    if (ImGui::IsItemHovered())
                    {
                        GetNextState()->GetNode()->HighLight();
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                        {
                            UNLINK_NEXT_STATE = true;
                        }
                    }
                }
                else if (ImGui::Button(MakeIdString("Add Next State").c_str()))
                    ADD_NEXT_STATE = true;
                ImGui::Separator();
                ImGui::Text("Condition");
                ImGui::Separator();
                Window::DrawTextEditor(m_ConditionEditor, m_Condition);
                ImGui::Separator();
                
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("OnTrue").c_str()))
            {
                Window::DrawTextEditor(m_OnTrueEditor, m_OnTrue);
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("OnFalse").c_str()))
            {
                Window::DrawTextEditor(m_OnFalseEditor, m_OnFalse);
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("Lua Code").c_str()))
            {
                m_LuaCodeEditor.Render("Lua Code");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        if (id != GetId())
        {
            SetId(id);
        }

        if (UNLINK_CURRENT_STATE)
        {
            std::string popupId = MakeIdString("Unlink Current State");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                if (ImGui::Button(MakeIdString("Unlink").c_str()))
                {
                    if (auto state = GetCurrentState())
                        state->RemoveTrigger(GetId());
                    UNLINK_CURRENT_STATE = false;
                    SetCurrentState("");
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::Button(MakeIdString("Cancel").c_str()))
                {
                    UNLINK_CURRENT_STATE = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        if (UNLINK_NEXT_STATE)
        {
            std::string popupId = MakeIdString("Unlink Next State");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                if (ImGui::Button(MakeIdString("Unlink").c_str()))
                {
                    UNLINK_NEXT_STATE = false;
                    SetNextState("");
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::Button(MakeIdString("Cancel").c_str()))
                {
                    UNLINK_NEXT_STATE = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        if (ADD_CURRENT_STATE)
        {
            std::string popupId = MakeIdString("Add Current State");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                static std::string key;
                ImGui::InputText(MakeIdString("id").c_str(), &key);
                static std::string name;
                ImGui::InputText(MakeIdString("name").c_str(), &name);
                if (!key.empty() && !name.empty() &&
                    !NodeEditor::Get()->GetCurrentFsm()->GetState(key)
                    && ImGui::Button(MakeIdString("Add State").c_str()))
                {
                    auto newState = NodeEditor::Get()->GetCurrentFsm()->AddState(key);
                    NodeEditor::Get()->GetCurrentFsm()->GetTrigger(m_Id)->SetCurrentState(newState->GetId());
                    newState->SetName(name);
                    ADD_CURRENT_STATE = false;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::Button(MakeIdString("Cancel").c_str()))
                {
                    ADD_CURRENT_STATE = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        if (ADD_NEXT_STATE)
        {
            std::string popupId = MakeIdString("Add Current State");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                static std::string key;
                ImGui::InputText(MakeIdString("id").c_str(), &key);
                static std::string name;
                ImGui::InputText(MakeIdString("name").c_str(), &name);
                if (!key.empty() && !name.empty() &&
                    !NodeEditor::Get()->GetCurrentFsm()->GetState(key) &&
                    ImGui::Button(MakeIdString("Add State").c_str()))
                {
                    ADD_NEXT_STATE = false;
                    auto newState = NodeEditor::Get()->GetCurrentFsm()->AddState(key);
                    key = "";
                    const auto trigger = NodeEditor::Get()->GetCurrentFsm()->GetTrigger(GetId());
                    trigger->SetNextState(newState->GetId());
                    newState->SetName(name);
                    name = "";
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::Button(MakeIdString("Cancel").c_str()))
                {
                    ADD_NEXT_STATE = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }

        if (IS_ADD_TRIGGER_ARGUMENT_OPEN)
        {
            std::string popupId = MakeIdString("Add Argument");
            ImGui::OpenPopup(popupId.c_str());
            if (ImGui::BeginPopup(popupId.c_str()))
            {
                static std::string key;
                static std::string value;
                ImGui::InputText(MakeIdString("name").c_str(), &key);
                ImGui::InputText(MakeIdString("value").c_str(), &value);
                if (ImGui::Button(MakeIdString("Add Argument").c_str()))
                {
                    AddArgument(key, value);
                    IS_ADD_TRIGGER_ARGUMENT_OPEN = false;
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::Button(MakeIdString("Cancel").c_str()))
                {
                    IS_ADD_TRIGGER_ARGUMENT_OPEN = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }

    std::shared_ptr<FsmTrigger> FsmTrigger::Deserialize(const nlohmann::json& json)
    {
        auto trigger = std::make_shared<FsmTrigger>(json["id"].get<std::string>());
        trigger->SetName(json["name"].get<std::string>());
        trigger->SetDescription(json["description"].get<std::string>());
        trigger->SetPriority(json["priority"].get<int>());
        trigger->SetNextState(json["nextStateId"].get<std::string>());
        trigger->SetCurrentState(json["currentStateId"].get<std::string>());
        for (const auto& [key, value] : json["arguments"].items())
            trigger->AddArgument(key, value.get<std::string>());
        trigger->SetCondition(json["condition"].get<std::string>());
        trigger->SetOnTrue(json["onTrue"].get<std::string>());
        trigger->SetOnFalse(json["onFalse"].get<std::string>());
        trigger->GetNode()->SetTargetPosition({json["positionX"].get<float>(), json["positionY"].get<float>()});
        return trigger;
    }

    nlohmann::json FsmTrigger::Serialize() const
    {
        nlohmann::json j;
        j["id"] = m_Id;
        j["name"] = m_Name;
        j["description"] = m_Description;
        j["priority"] = m_Priority;
        j["nextStateId"] = m_NextStateId;
        j["currentStateId"] = m_CurrentStateId;
        j["arguments"] = m_Arguments;
        j["condition"] = m_Condition;
        j["onTrue"] = m_OnTrue;
        j["onFalse"] = m_OnFalse;
        j["positionX"] = m_Node.GetTargetPosition().x;
        j["positionY"] = m_Node.GetTargetPosition().y;
        return j;
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
        for (const auto& line : m_ConditionEditor.GetTextLines())
            code += indentTabs + fmt::format("\t\t{0}\n", line);
        code += indentTabs + fmt::format("\tend,\n");
        code += indentTabs + fmt::format("\t---@param self FSM_TRIGGER\n");
        code += indentTabs + fmt::format("\tonTrue = function(self)\n");
        for (const auto& line : m_OnTrueEditor.GetTextLines())
            code += indentTabs + fmt::format("\t\t{0}\n", line);
        code += indentTabs + fmt::format("\tend,\n");
        code += indentTabs + fmt::format("\t---@param self FSM_TRIGGER\n");
        code += indentTabs + fmt::format("\tonFalse = function(self)\n");
        for (const auto& line : m_OnFalseEditor.GetTextLines())
            code += indentTabs + fmt::format("\t\t{0}\n", line);
        code += indentTabs + fmt::format("\tend,\n");
        code += indentTabs + fmt::format("}})");
        return code;
    }
}

/*
 * 
        if (m_Node.GetTargetPosition().x < 0)
            m_Node.SetTargetPosition(editor->GetNextNodePos());
        ImGui::SetNextWindowSize(m_Node.InitSizesDiamond(m_Name), ImGuiCond_Once);
        ImVec2 adjustedPos = GetPosition();
        adjustedPos = Math::AddVec2(editor->GetCanvasPos(), adjustedPos);
        if (adjustedPos.x < 0)
        {
            m_Node.SetInvisible(true);
            return nullptr;
        }
        m_Node.SetInvisible(false);
        // Draw the node
        if (!Math::ImVec2Equal(m_Node.GetLastPosition(), adjustedPos))
            ImGui::SetNextWindowPos(adjustedPos);
        m_Node.SetLastPosition(adjustedPos);
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

            auto windowPos = ImGui::GetWindowPos();
            const auto movedAmount =
                Math::SubtractVec2(windowPos, Math::AddVec2(adjustedPos, {ImGui::GetScrollX() / 5, ImGui::GetScrollY() / 5}));
            auto newPos = Math::AddVec2(m_Node.GetTargetPosition(), movedAmount);
            newPos.x = std::max(editor->GetCanvasPos().x + 1, newPos.x);
            newPos.y = std::max(editor->GetCanvasPos().y + 1, newPos.y);
            m_Node.SetTargetPosition(newPos);
            ImGui::End();
        }
        return &m_Node;
 */
