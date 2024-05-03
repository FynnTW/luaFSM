#include "pch.h"
#include "FSM.h"

namespace LuaFsm
{
    Fsm::Fsm(std::string id)
        : m_Id(std::move(id))
    {
        
    }

    void Fsm::AddState(const std::string& key, const FsmStatePtr& value)
    {
        m_States[key] = value;
        value->SetFsm(std::make_shared<Fsm>(*this));
        for (const auto& event : value->GetEvents())
            if (std::ranges::find(m_UpdateEvents, event) == m_UpdateEvents.end())
                AddUpdateEvent(event);
        if (m_States[key]->GetIntId() == -1)
        {
            int newId = 0;
            for (const auto& [i, state] : m_States)
            {
                if (state->GetIntId() == newId)
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

    void Fsm::AddState(const FsmStatePtr& value)
    {
        auto key = value->GetId();
        AddState(key, value);
    }

    FsmStatePtr Fsm::GetState(const std::string& key)
    {
        if (!m_States.contains(key))
            return nullptr;
        return m_States[key];
    }
}
