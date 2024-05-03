#include "pch.h"
#include "FsmState.h"


namespace LuaFsm
{
    FsmState::FsmState(std::string id)
        : m_Id(std::move(id))
    {
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
                    argValue->SetNextStateId(m_Id);
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
        value->SetCurrentState(std::make_shared<FsmState>(*this));
        for (const auto& [argKey, argValue] : value->GetArguments())
            AddData(argKey, "");
        if (m_Triggers[key]->GetIntId() == -1)
        {
            int newId = 0;
            for (const auto& [i, trigger] : m_Triggers)
            {
                if (trigger->GetIntId() == newId)
                {
                    newId+=3;
                    continue;
                }
                value->SetIntId(newId);
                value->SetInputId(newId + 1);
                value->SetOutputId(newId + 2);
                break;
            }     
        }
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
}
