#include "pch.h"
#include "FsmTrigger.h"

#include <spdlog/fmt/bundled/format.h>
#include <data/FsmState.h>

#include "imgui/imgui_stdlib.h"
#include "imgui/NodeEditor.h"

#include "Graphics/Window.h"
#include "imgui/ImGuiNotify.hpp"
#include "IO/FileReader.h"

namespace LuaFsm
{
    FsmTrigger::FsmTrigger(const std::string& id): DrawableObject(id)
    {
        m_ConditionEditor.SetText(m_Condition);
        m_ConditionEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        m_ActionEditor.SetText(m_Action);
        m_ActionEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        m_LuaCodeEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        m_Node.SetId(m_Id);
        m_Node.SetColor(IM_COL32(75, 75, 0, 150));
        m_Node.SetHighlightColor(m_Node.GetHighlightColor());
        m_Node.SetHighlightColorSelected(m_Node.GetHighlightColorSelected());
        m_Node.SetBorderColor(IM_COL32(255, 255, 255, 255));
        m_Node.SetType(NodeType::Transition);
        m_Node.SetShape(NodeShape::Circle);
        InitPopups();
    }

    FsmTrigger::FsmTrigger(const FsmTrigger& other): DrawableObject(other.GetId())
    {
        m_Name = other.m_Name;
        m_Description = other.m_Description;
        m_Condition = other.m_Condition;
        m_Action = other.m_Action;
        m_Priority = other.m_Priority;
        m_CurrentStateId = other.m_CurrentStateId;
        m_NextStateId = other.m_NextStateId;
        m_Node.SetGridPos(other.m_Node.GetGridPos());
        m_Node.SetColor(other.m_Node.GetColor());
        m_Node.SetInArrowCurve(other.m_Node.GetInArrowCurve());
        m_Node.SetOutArrowCurve(other.m_Node.GetOutArrowCurve());
    }

    void FsmTrigger::UpdateEditors()
    {
        Window::TrimTrailingNewlines(m_Condition);
        m_ConditionEditor.SetText(m_Condition);
        Window::TrimTrailingNewlines(m_Action);
        m_ActionEditor.SetText(m_Action);
        m_LuaCodeEditor.SetText(GetLuaCode());
    }

    void FsmTrigger::InitPopups()
    {
        const auto unlinkNextStatePopup = std::make_shared<UnlinkStatePopup>("UnlinkNextState" + m_Id);
        unlinkNextStatePopup->parent = m_Id;
        m_PopupManager.AddPopup(TriggerPopups::UnlinkNextState, unlinkNextStatePopup);
        
        const auto unlinkCurrentStatePopup = std::make_shared<UnlinkStatePopup>("UnlinkCurrentState" + m_Id);
        unlinkCurrentStatePopup->parent = m_Id;
        m_PopupManager.AddPopup(TriggerPopups::UnlinkCurrentState, unlinkCurrentStatePopup);
        
        const auto deleteTrigger = std::make_shared<DeleteTriggerPopup>("deleteTrigger" + m_Id);
        deleteTrigger->parent = m_Id;
        m_PopupManager.AddPopup(TriggerPopups::DeleteTrigger, deleteTrigger);
        
        const auto setNewId = std::make_shared<RefactorIdPopup>("SetNewId" + m_Id, "trigger");
        setNewId->parent = m_Id;
        m_PopupManager.AddPopup(TriggerPopups::SetNewId, setNewId);
    }

    std::vector<std::shared_ptr<FsmTrigger>> FsmTrigger::CreateFromFile(const std::string& filePath)
    {
        auto conditions = std::vector<std::shared_ptr<FsmTrigger>>{};
        const std::string code = FileReader::ReadAllText(filePath);
        if (code.empty())
            return conditions;
        std::regex idRegex = FsmRegex::IdRegex("FSM_CONDITION");
        std::sregex_iterator iter(code.begin(), code.end(), idRegex);
        for (std::sregex_iterator end; iter != end; ++iter)
        {
            std::string id = (*iter)[1].str();
            auto condition = std::make_shared<FsmTrigger>(id);
            conditions.emplace_back(condition);
        }
        return conditions;
    }

    std::string FsmTrigger::GetLuaCode()
    {
        std::string code;
        code += fmt::format("---@FSM_CONDITION {0}\n", m_Id);
        code += fmt::format("---@class {0} : FSM_CONDITION\n", m_Id);
        code += fmt::format("local {0} = FSM_CONDITION:new({{}})\n", m_Id);
        code += fmt::format("{0}.name = \"{1}\"\n", m_Id, m_Name);
        code += fmt::format("{0}.description = \"{1}\"\n", m_Id, m_Description);
        code += fmt::format("{0}.editorPos = {{{1}, {2}}}\n", m_Id, m_Node.GetGridPos().x, m_Node.GetGridPos().y);
        code += fmt::format("{0}.inLineCurve = {1}\n", m_Id, m_Node.GetInArrowCurve());
        code += fmt::format("{0}.outLineCurve = {1}\n", m_Id, m_Node.GetOutArrowCurve());
        code += fmt::format("{0}.color = {{{1}, {2}, {3}, {4}}}\n", m_Id, m_Node.GetColor().Value.x, m_Node.GetColor().Value.y, m_Node.GetColor().Value.z, m_Node.GetColor().Value.w);
        code += fmt::format("{0}.currentStateId = \"{1}\"\n", m_Id, m_CurrentStateId);
        code += fmt::format("{0}.nextStateId = \"{1}\"\n", m_Id, m_NextStateId);
        code += fmt::format("{0}.priority = {1}\n", m_Id, m_Priority);
        if (!m_Condition.empty())
        {
            code += fmt::format("\n---@return boolean isTrue\n");
            code += fmt::format("function {0}:condition()\n", m_Id);
            for (const auto& line : m_ConditionEditor.GetTextLines())
                code += fmt::format("\t{0}\n", line);
            code += fmt::format("end---@endFunc\n");
        }
        if (!m_Action.empty())
        {
            code += fmt::format("\nfunction {0}:action()\n", m_Id);
            for (const auto& line : m_ActionEditor.GetTextLines())
                code += fmt::format("\t{0}\n", line);
            code += fmt::format("end---@endFunc\n");
        }
        return code;
    }

    void FsmTrigger::UpdateFileContents(std::string& code, const std::string& oldId)
    {
        UpdateEditors();
        std::regex regex = FsmRegex::IdRegexClass("FSM_CONDITION", oldId);
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("---@FSM_CONDITION {}", m_Id));
        else
        {
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "No Entry for condition %s found in file", oldId.c_str()});
            return;
        }
        regex = FsmRegex::IdRegexClassAnnotation("FSM_CONDITION", oldId);
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("class {0} : FSM_CONDITION", m_Id));
        regex = FsmRegex::IdRegexClassDeclaration("FSM_CONDITION", oldId);
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0} = FSM_CONDITION:new", m_Id));
        regex = FsmRegex::ClassStringRegex(oldId, "name");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.name = \"{1}\"", m_Id, m_Name));
        else if (!m_Name.empty())
                ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm condition %s name entry not found in file!", m_Id.c_str()});
        regex = FsmRegex::ClassStringRegex(oldId, "description");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.description = \"{1}\"", m_Id, m_Description));
        else if (!m_Description.empty())
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm condition %s description entry not found in file!", m_Id.c_str()});
        regex = FsmRegex::ClassTableRegex(oldId, "editorPos");
        if (std::smatch match; std::regex_search(code, match, regex))
        {
            std::string tableContent = match[1].str();
            std::regex idRegex = FsmRegex::ClassTableElementsRegex();
            std::sregex_iterator iter(tableContent.begin(), tableContent.end(), idRegex);
            std::vector<float> elements;
            for (std::sregex_iterator end; iter != end; ++iter) {
                elements.push_back(std::stof((*iter)[1].str()));
            }
            if (elements.size() >= 2)
            {
                ImVec2 position;
                position.x = elements[0];
                position.y = elements[1];
                position = GetNode()->GetGridPos();
                code = std::regex_replace(code, regex, fmt::format("{0}.editorPos = {{{1}, {2}}}", m_Id, position.x, position.y));
            }
        }
        else
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm condition %s position entry not found in file!", m_Id.c_str()});
        regex = FsmRegex::ClassTableRegex(oldId, "color");
        if (std::smatch match; std::regex_search(code, match, regex))
        {
            std::string tableContent = match[1].str();
            std::regex idRegex = FsmRegex::ClassTableElementsRegex();
            std::sregex_iterator iter(tableContent.begin(), tableContent.end(), idRegex);
            std::vector<float> elements;
            for (std::sregex_iterator end; iter != end; ++iter) {
                elements.push_back(std::stof((*iter)[1].str()));
            }
            if (elements.size() >= 4)
            {
                ImColor color{};
                color.Value.x = elements[0];
                color.Value.y = elements[1];
                color.Value.z = elements[2];
                color.Value.w = elements[3];
                color = GetNode()->GetColor();
                code = std::regex_replace(code, regex, fmt::format("{0}.color = {{{1}, {2}, {3}, {4}}}", m_Id, color.Value.x, color.Value.y, color.Value.z, color.Value.w));
            }
        }
        regex = FsmRegex::ClassStringRegex(oldId, "currentStateId");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.currentStateId = \"{1}\"", m_Id, m_CurrentStateId));
        else if (!m_CurrentStateId.empty())
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm condition %s current state entry not found in file!", m_Id.c_str()});
        regex = FsmRegex::ClassStringRegex(oldId, "nextStateId");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.nextStateId = \"{1}\"", m_Id, m_NextStateId));
        else if (!m_NextStateId.empty())
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm condition %s next state entry not found in file!", m_Id.c_str()});
        regex = FsmRegex::ClassIntegerRegex(oldId, "priority");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.priority = {1}", m_Id, m_Priority));
        else if (m_Priority != 0)
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm condition %s priority entry not found in file!", m_Id.c_str()});
        regex = FsmRegex::ClassFloatRegex(oldId, "inLineCurve");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.inLineCurve = {1}", m_Id, m_Node.GetInArrowCurve()));
        else if (m_Node.GetInArrowCurve() > 0.0001f || m_Node.GetInArrowCurve() < 0.0001f)
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm condition %s inLineCurve entry not found in file!", m_Id.c_str()});

        regex = FsmRegex::ClassFloatRegex(oldId, "outLineCurve");
        if (std::smatch match; std::regex_search(code, match, regex))
            code = std::regex_replace(code, regex, fmt::format("{0}.outLineCurve = {1}", m_Id, m_Node.GetOutArrowCurve()));
        else if (m_Node.GetOutArrowCurve() > 0.0001f || m_Node.GetOutArrowCurve() < 0.0001f)
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm condition %s outLineCurve entry not found in file!", m_Id.c_str()});

        regex = FsmRegex::FunctionBodyReplace(oldId, "condition");
        if (std::smatch match; std::regex_search(code, match, regex))
        {
            std::string func;
            for (const auto& line : m_ConditionEditor.GetTextLines())
                func += fmt::format("\t{0}\n", line);
            code = std::regex_replace(code, regex, "$1\n" + func + "$3");
            auto regexString = fmt::format("({0}:condition)", oldId);
            regex = std::regex(regexString);
            code = std::regex_replace(code, regex, fmt::format("{0}:condition", m_Id));
        }
        else if (!m_Condition.empty())
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm state %s condition entry not found in file!", m_Id.c_str()});
        regex = FsmRegex::FunctionBodyReplace(oldId, "action");
        if (std::smatch match; std::regex_search(code, match, regex))
        {
            std::string func;
            for (const auto& line : m_ActionEditor.GetTextLines())
                func += fmt::format("\t{0}\n", line);
            code = std::regex_replace(code, regex, "$1\n" + func + "$3");
            auto regexString = fmt::format("({0}:action)", oldId);
            regex = std::regex(regexString);
            code = std::regex_replace(code, regex, fmt::format("{0}:action", m_Id));
        }
        else if (!m_Action.empty())
            ImGui::InsertNotification({ImGuiToastType::Warning, 3000, "Fsm state %s action entry not found in file!", m_Id.c_str()});
        m_UnSaved = false;
    }

    void FsmTrigger::UpdateFromFile(const std::string& filePath)
    {
        const std::string code = FileReader::ReadAllText(filePath);
        if (code.empty())
            return;
        std::string name;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassStringRegex(m_Id, "name")))
            name = match[1].str();
        else
            name = m_Id;
        SetName(name);
        std::string description;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassStringRegex(m_Id, "description")))
            description = match[1].str();
        SetDescription(description);
        std::string currentStateId;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassStringRegex(m_Id, "currentStateId")))
            currentStateId = match[1].str();
        SetCurrentState(currentStateId);
        std::string nextStateId;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassStringRegex(m_Id, "nextStateId")))
            nextStateId = match[1].str();
        SetNextState(nextStateId);
        int priority = 0;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassIntegerRegex(m_Id, "priority")))
            priority = std::stoi(match[1].str());
        SetPriority(priority);
        float inLineCurve = 0;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassFloatRegex(m_Id, "inLineCurve")))
            inLineCurve = std::stof(match[1].str());
        m_Node.SetInArrowCurve(inLineCurve);
        float outLineCurve = 0;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassFloatRegex(m_Id, "outLineCurve")))
            outLineCurve = std::stof(match[1].str());
        m_Node.SetOutArrowCurve(outLineCurve);
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassTableRegex(m_Id, "editorPos")))
        {
            std::string tableContent = match[1].str();
            std::regex idRegex = FsmRegex::ClassTableElementsRegex();
            std::sregex_iterator iter(tableContent.begin(), tableContent.end(), idRegex);
            std::vector<float> elements;
            for (std::sregex_iterator end; iter != end; ++iter) {
                elements.push_back(std::stof((*iter)[1].str()));
            }
            if (elements.size() >= 2)
            {
                ImVec2 position;
                position.x = elements[0];
                position.y = elements[1];
                GetNode()->SetGridPos(position);
            }
        }
        if (std::smatch match; std::regex_search(code, match, FsmRegex::ClassTableRegex(m_Id, "color")))
        {
            std::string tableContent = match[1].str();
            std::regex idRegex = FsmRegex::ClassTableElementsRegex();
            std::sregex_iterator iter(tableContent.begin(), tableContent.end(), idRegex);
            std::vector<float> elements;
            for (std::sregex_iterator end; iter != end; ++iter) {
                elements.push_back(std::stof((*iter)[1].str()));
            }
            if (elements.size() >= 4)
            {
                ImColor color{};
                color.Value.x = elements[0];
                color.Value.y = elements[1];
                color.Value.z = elements[2];
                color.Value.w = elements[3];
                GetNode()->SetColor(color);
            }
        }
        std::string condition = "return false";
        if (std::smatch match; std::regex_search(code, match, FsmRegex::FunctionBody(m_Id, "condition")))
            condition = match[1].str();
        SetCondition(condition);
        std::string action;
        if (std::smatch match; std::regex_search(code, match, FsmRegex::FunctionBody(m_Id, "action")))
            action = match[1].str();
        SetAction(action);
        UpdateEditors();
        m_UnSaved = false;
    }

    void FsmTrigger::UpdateToFile(const std::string& oldId)
    {
        if (!NodeEditor::Get()->GetCurrentFsm())
            return;
        auto code = NodeEditor::Get()->GetCurrentFsm()->GetLinkedFileCode();
        if (code.empty())
            return;
        UpdateFileContents(code, oldId);
        NodeEditor::Get()->GetCurrentFsm()->SaveLinkedFile(code);
        CreateLastState();
    }

    void FsmTrigger::RefactorId(const std::string& newId)
    {
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        const std::string oldId = m_Id;
        if (!fsm)
            return;
        if (fsm->GetTrigger(newId) || fsm->GetState(newId))
        {
            ImGui::InsertNotification({ImGuiToastType::Error, 3000, "Condition with id %s already exists!", newId.c_str()});
            return;
        }
        SetId(newId);
        if (fsm->GetLinkedFile().empty())
            return;
        UpdateToFile(oldId);
        fsm->UpdateToFile(fsm->GetId());
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
        m_NextStateId = stateId;
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
            return;
        if (const auto state = fsm->GetState(stateId); state)
            m_NextState = state.get();
        else
            m_NextState = nullptr;
    }

    void FsmTrigger::SetCurrentState(const std::string& stateId)
    {
        m_CurrentStateId = stateId;
        const auto fsm = NodeEditor::Get()->GetCurrentFsm();
        if (!fsm)
            return;
        if (const auto state = fsm->GetState(stateId); state)
        {
            m_CurrentState = state.get();
            if (!state->GetTriggers().contains(m_Id))
                state->AddTrigger(fsm->GetTrigger(m_Id));
        }
        else
            m_CurrentState = nullptr;
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

#define COMPARE_COLOR(a, b) ((a).x - (b).x > 0.0001f || (a).y - (b).y > 0.0001f || (a).z - (b).z > 0.0001f || (a).w - (b).w > 0.0001f)

    void FsmTrigger::DrawProperties()
    {
        if (ImGui::BeginMenuBar())
        {
            if (const auto linkedFile = NodeEditor::Get()->GetCurrentFsm()->GetLinkedFile(); !linkedFile.empty())
            {
                if (ImGui::Button(MakeIdString("Load from file").c_str()))
                    UpdateFromFile(linkedFile);
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Save to file").c_str()))
                    UpdateToFile(m_Id);
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button(MakeIdString("Delete").c_str()))
                    m_PopupManager.OpenPopup(static_cast<int>(TriggerPopups::DeleteTrigger));
                ImGui::SetItemTooltip("This doesn't delete it from your lua file! Delete it manually!");
                ImGui::PopStyleColor();
            }
            ImGui::EndMenuBar();
        }
        if (ImGui::BeginTabBar(MakeIdString("Node Properties").c_str()))
        {
            if (ImGui::BeginTabItem(MakeIdString("Trigger Properties").c_str()))
            {
                ImGui::Text("ID: %s", GetId().c_str());
                ImGui::SameLine();
                if (ImGui::Button(MakeIdString("Refactor ID").c_str()))
                    m_PopupManager.OpenPopup(TriggerPopups::SetNewId);
                ImGui::Text("Name");
                std::string name = GetName();
                const std::string nameLabel = "##Name" + GetId();
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputText(nameLabel.c_str(), &name);
                SetName(name);
                ImGui::Text("Description");
                std::string description = GetDescription();
                const std::string descriptionLabel = "##Description" + GetId();
                ImGui::InputTextMultiline(descriptionLabel.c_str(), &description, {ImGui::GetContentRegionAvail().x, 60});
                SetDescription(description);
                ImGui::Text("Priority");
                ImGui::SetNextItemWidth(150.f);
                ImGui::InputInt(MakeIdString("Priority").c_str(), &m_Priority);
                ImGui::SetItemTooltip("Priority decides the order in which conditions get evaluated.");
                ImGui::Separator();
                ImGui::SetNextItemWidth(100.f);
                float inCurve = m_Node.GetInArrowCurve();
                ImGui::SliderFloat(MakeIdString("In curve").c_str(), &inCurve, -1.0f, 1.0f);
                m_Node.SetInArrowCurve(inCurve);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100.f);
                float outCurve = m_Node.GetOutArrowCurve();
                ImGui::SliderFloat(MakeIdString("Out curve").c_str(), &outCurve, -1.0f, 1.0f);
                m_Node.SetOutArrowCurve(outCurve);
                ImGui::Separator();
                auto color = m_Node.GetColor();
                ImGui::ColorEdit4("Node Color", reinterpret_cast<float*>(&color));
                if (ImVec4(color) != ImVec4(m_Node.GetColor()))
                {
                    m_Node.SetColor(color);
                    m_Node.SetCurrentColor(color);
                }
                ImGui::Separator();
                ImGui::Text("Current State: ");
                if (const auto currentState = GetCurrentState())
                {
                    ImGui::SameLine();
                    ImGui::Selectable(MakeIdString(m_CurrentStateId).c_str(), false);
                    ImGui::SetItemTooltip(fmt::format("{0}\n{1}", currentState->GetName(), currentState->GetDescription()).c_str());
                    if (ImGui::IsItemClicked())
                        NodeEditor::Get()->MoveToNode(m_CurrentStateId, NodeType::State);
                    if (ImGui::IsItemHovered())
                    {
                        currentState->GetNode()->SetIsHighlighted(true);
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                        {
                            m_PopupManager.GetPopup<UnlinkStatePopup>(TriggerPopups::UnlinkCurrentState)->stateId = m_CurrentStateId;
                            m_PopupManager.OpenPopup(TriggerPopups::UnlinkCurrentState);
                        }
                    }
                    else
                        currentState->GetNode()->SetIsHighlighted(false);
                }
                ImGui::Separator();
                ImGui::Text("Next State: ");
                if (const auto nextState = GetNextState())
                {
                    ImGui::SameLine();
                    ImGui::Selectable(MakeIdString(m_NextStateId).c_str(), false);
                    ImGui::SetItemTooltip(fmt::format("{0}\n{1}", nextState->GetName(), nextState->GetDescription()).c_str());
                    if (ImGui::IsItemClicked())
                        NodeEditor::Get()->MoveToNode(m_NextStateId, NodeType::State);
                    if (ImGui::IsItemHovered())
                    {
                        nextState->GetNode()->SetIsHighlighted(true);
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                        {
                            m_PopupManager.GetPopup<UnlinkStatePopup, TriggerPopups>(TriggerPopups::UnlinkNextState)->stateId = m_NextStateId;
                            m_PopupManager.OpenPopup(TriggerPopups::UnlinkNextState);
                        }
                    }
                    else
                        nextState->GetNode()->SetIsHighlighted(false);
                }
                ImGui::Separator();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                ImGui::PopStyleColor();
                ImGui::Separator();
                ImGui::Text("Condition");
                ImGui::Separator();
                Window::DrawTextEditor(m_ConditionEditor, m_Condition);
                ImGui::Separator();
                
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("Action").c_str()))
            {
                Window::DrawTextEditor(m_ActionEditor, m_Action);
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem(MakeIdString("Lua Code").c_str()))
            {
                if (ImGui::Button(MakeIdString("Regenerate Code").c_str()) || m_LuaCodeEditor.GetText().empty())
                    m_LuaCodeEditor.SetText(GetLuaCode());
                ImGui::Separator();
                m_LuaCodeEditor.Render("Lua Code");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        m_PopupManager.ShowOpenPopups();
    }

    void FsmTrigger::CreateLastState()
    {
        m_PreviousState = std::make_shared<FsmTrigger>(*this);
    }

    bool FsmTrigger::IsChanged()
    {
        if (m_PreviousState == nullptr)
            return true;
        if (*this != *m_PreviousState || m_UnSaved)
        {
            m_UnSaved = true;
            return true;
        }
        return false;
    }

    void FsmTrigger::AppendToFile()
    {
        if (const auto fsm = NodeEditor::Get()->GetCurrentFsm(); fsm)
        {
            if (auto code = fsm->GetLinkedFileCode(); !code.empty())
            {
                code += "\n\n";
                code += GetLuaCode();
                fsm->SaveLinkedFile(code);
                CreateLastState();
            }
        }
    }
}
