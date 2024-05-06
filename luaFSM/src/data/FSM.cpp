#include "pch.h"
#include "FSM.h"
#include "FsmTrigger.h"
#include <spdlog/fmt/bundled/format.h>

#include <utility>

#include "Graphics/Window.h"
#include "imgui/imgui_stdlib.h"
#include "imgui/NodeEditor.h"
#include "IO/FileReader.h"
#include "json.hpp"

namespace LuaFsm
{
    Fsm::Fsm(const std::string& id)
    {
        Fsm::SetId(id);
        m_LuaCodeEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
    }

    void Fsm::AddState(const std::string& key, const FsmStatePtr& value)
    {
        m_States[key] = value;
        for (const auto& event : value->GetEvents())
            if (std::ranges::find(m_UpdateEvents, event) == m_UpdateEvents.end())
                AddUpdateEvent(event);
        if (m_InitialStateId.empty())
            m_InitialStateId = key;
    }

    FsmStatePtr Fsm::AddState(const std::string& key)
    {
        m_States[key] = make_shared<FsmState>(key);
        auto value = m_States[key];
        for (const auto& event : value->GetEvents())
            if (std::ranges::find(m_UpdateEvents, event) == m_UpdateEvents.end())
                AddUpdateEvent(event);
        if (m_InitialStateId.empty())
            m_InitialStateId = key;
        return value;
    }

    void Fsm::AddState(const FsmStatePtr& value)
    {
        const auto key = value->GetId();
        AddState(key, value);
    }

    void Fsm::AddTrigger(const std::string& key, FsmTriggerPtr value)
    {
        m_Triggers[key] = std::move(value);
    }

    std::unordered_map<std::string, FsmTriggerPtr> Fsm::GetTriggers()
    {
        return m_Triggers;
    }

    void Fsm::AddTrigger(const FsmTriggerPtr& value)
    {
        const auto key = value->GetId();
        AddTrigger(key, value);
    }

    FsmTriggerPtr Fsm::GetTrigger(const std::string& key)
    {
        if (!m_Triggers.contains(key))
            return nullptr;
        return m_Triggers[key];
    }

    FsmStatePtr Fsm::GetState(const std::string& key)
    {
        if (!m_States.contains(key))
            return nullptr;
        return m_States[key];
    }

    void Fsm::RemoveState(const std::string& state)
    {
        if (m_States.contains(state))
            m_States.erase(state);
        for (const auto& [key, value] : m_Triggers)
        {
            if (value->GetCurrentStateId() == state)
                value->SetCurrentState("");
            if (value->GetNextStateId() == state)
                value->SetNextState("");
        }
        if (m_InitialStateId == state)
            m_InitialStateId = "";
    }
    
    void Fsm::DrawProperties()
    {
        std::string id = GetId();
        if (ImGui::Button("Refresh") || m_LuaCodeEditor.GetText().empty())
        {
            //m_LuaCodeEditor.SetText(GetLuaCode());
            auto json = Serialize();
            auto str = json.dump(4);
            m_LuaCodeEditor.SetText(str);
        }
        if (ImGui::BeginTabBar("FSM Properties0"))
        {
            
            if (ImGui::BeginTabItem("Properties"))
            {
                
                ImGui::InputText("Id", &id);
                ImGui::InputText("Name", &m_Name);
                if (!GetInitialState().empty())
                    ImGui::Selectable(GetInitialState().c_str(), false);
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    NodeEditor::Get()->SetSelectedNode(NodeEditor::Get()->GetNode(GetInitialState()));
                m_LuaCodeEditor.SetPalette(Window::GetPalette());
                m_LuaCodeEditor.Render("Lua Code");
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        if (id != GetId())
        {
            SetId(id);
        }
    }

    FsmPtr Fsm::ParseFile(const std::string& path)
    {
        const auto reader = std::make_shared<FileReader>();
        auto fsm = std::make_shared<Fsm>("");
        reader->Open(path);
        bool inFsmTable = false;
        if (!reader->IsOpen())
            return nullptr;
        do
        {
            auto line = reader->ReadLine();
            FileReader::RemoveLuaComments(line);
            FileReader::RemoveTabs(line);
            if (line.empty())
                continue;
            if (!inFsmTable && FileReader::LineContains(line, "FSM:new"))
            {
                if (std::string varName = reader->GetVarName(line, "FSM:new"); !varName.empty())
                {
                    fsm->SetId(varName);
                    inFsmTable = true;
                    continue;
                }
            }
            if (inFsmTable)
            {
                if (FileReader::LineContains(line, "name"))
                {
                    if (std::string name = reader->GetTableField(line, "name"); !name.empty())
                    {
                        fsm->SetName(name);
                        continue;
                    }
                }
                if (FileReader::LineContains(line, "initialStateId"))
                {
                    if (std::string initialState = reader->GetTableField(line, "initialStateId"); !initialState.empty())
                    {
                        fsm->SetInitialState(initialState);
                        continue;
                    }
                }
            }
        } while (!reader->IsEndOfFile());
        reader->Close();
        return fsm;
    }


    

    FsmPtr Fsm::ParseFile2(const std::string& path)
    {
        const auto reader = std::make_shared<FileReader>();
        auto fsm = std::make_shared<Fsm>("");
        reader->Open(path);
        std::string variableName;
        if (!reader->IsOpen())
            return nullptr;
        do
        {
            auto word = reader->ReadWord();
            if (word.empty())
                continue;
            
        } while (!reader->IsEndOfFile());
        reader->Close();
        return fsm;
    }

    nlohmann::json Fsm::Serialize() const
    {
        nlohmann::json j;
        j["id"] = m_Id;
        j["name"] = m_Name;
        j["initialStateId"] = m_InitialStateId;
        nlohmann::json states;
        for (const auto& [key, value] : m_States)
        {
            states[key] = value->Serialize();
        }
        j["states"] = states;
        nlohmann::json triggers;
        for (const auto& [key, value] : m_Triggers)
        {
            triggers[key] = value->Serialize();
        }
        j["triggers"] = triggers;
        return j;
    }

    std::shared_ptr<Fsm> Fsm::Deserialize(const nlohmann::json& json)
    {
        auto fsm = std::make_shared<Fsm>(json["id"].get<std::string>());
        fsm->SetName(json["name"].get<std::string>());
        fsm->SetInitialState(json["initialStateId"].get<std::string>());
        for (const auto& [key, value] : json["states"].items())
        {
            fsm->AddState(key, FsmState::Deserialize(value));
        }
        for (const auto& [key, value] : json["triggers"].items())
        {
            fsm->AddTrigger(key, FsmTrigger::Deserialize(value));
            if (const auto trigger = fsm->GetTrigger(key); trigger != nullptr)
            {
                if (const auto state = trigger->GetCurrentState(); state != nullptr)
                    state->AddTrigger(trigger);
            }
        }
        return fsm;
    }

    std::string Fsm::GetLuaCode()
    {
        std::string code;
        code += fmt::format("---@type FSM\n");
        code += fmt::format("{0} = FSM:new({{\n", m_Id);
        code += fmt::format("\t---Unique Identifier of this FSM\n");
        code += fmt::format("\t---@type string\n");
        code += fmt::format("\tid = \"{0}\",\n", m_Id);
        code += fmt::format("\t---Name of this FSM\n");
        code += fmt::format("\t---@type string\n");
        code += fmt::format("\tname = \"{0}\",\n", m_Name);
        code += fmt::format("\t---Name of the initial state\n");
        code += fmt::format("\t---@type string\n");
        code += fmt::format("\tinitialStateId = \"{0}\",\n", m_InitialStateId);
        code += fmt::format("\t---States within this FSM\n");
        code += fmt::format("\t---@type table<string, FSM_STATE>\n");
        code += fmt::format("\tstates = {{\n");
        code += fmt::format("\t}},\n");
        code += fmt::format("}})\n");
        for (const auto& [key, value] : m_States)
        {
            code += value->GetLuaCode(0) + "\n";
            code += fmt::format("{0}:registerState({1})\n", m_Id, key);
        }
        return code;
    }

    void Fsm::ChangeTriggerId(const std::string& oldId, const std::string& newId)
    {
        if (m_Triggers.contains(oldId))
        {
            m_Triggers[newId] = m_Triggers[oldId];
            m_Triggers.erase(oldId);
        }
    }

    void Fsm::RemoveTrigger(const std::string& trigger)
    {
        const auto obj = GetTrigger(trigger);
        if (obj == nullptr)
            return;
        if (const auto state = obj->GetCurrentState(); state != nullptr)
            state->RemoveTrigger(trigger);
        m_Triggers.erase(trigger);
    }
}
