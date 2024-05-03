#include "pch.h"
#include "Log.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/fmt/ostr.h"

namespace LuaFsm
{
   std::shared_ptr<spdlog::logger> Log::m_Log;

   void Log::Init()
   {
       spdlog::set_pattern("%^[%T] %n: %v%$");
       m_Log = spdlog::stdout_color_mt("LuaFsm");
       m_Log->set_level(spdlog::level::trace);
   }

    Log::Log()
    = default;

    Log::~Log()
    = default;
}
