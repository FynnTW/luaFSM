#include "pch.h"
#include "FsmTrigger.h"

#include <spdlog/fmt/bundled/format.h>

namespace LuaFsm
{
    FsmTrigger::FsmTrigger(std::string id)
        : m_Id(std::move(id))
    {
    }

    std::string FsmTrigger::GetLuaCode()
    {
        std::string code;
        code += fmt::format("{0} = FSM_TRIGGER:new({{\n", m_Id);
        code += fmt::format("\tid = \"{0}\",\n", m_Id);
        code += fmt::format("\tname = \"{0}\",\n", m_Name);
        code += fmt::format("\tdescription = \"{0}\",\n", m_Description);
        code += fmt::format("\tpriority = {0},\n", m_Priority);
        code += fmt::format("\tnextStateId = \"{0}\",\n", m_NextStateId);
        code += fmt::format("\targuments = {{\n");
        for (const auto& [key, value] : m_Arguments)
            code += fmt::format("\t\t{0} = \"{1}\",\n", key, value);
        code += fmt::format("\t}},\n");
        code += fmt::format("\tcondition = function(this)\n");
        code += fmt::format("\t\t{0}\n", m_Condition);
        code += fmt::format("\tend,\n");
        code += fmt::format("\tonTrue = function(this)\n");
        code += fmt::format("\t\t{0}\n", m_OnTrue);
        code += fmt::format("\tend,\n");
        code += fmt::format("\tonFalse = function(this)\n");
        code += fmt::format("\t\t{0}\n", m_OnFalse);
        code += fmt::format("\tend,\n");
        code += fmt::format("}})\n");
        return code;
    }
}
