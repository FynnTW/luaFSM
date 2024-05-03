#pragma once
#include "spdlog/spdlog.h"

namespace LuaFsm
{
    class Log
    {
    public:
        static void Init();
        Log();
        ~Log();
        inline static std::shared_ptr<spdlog::logger>& GetLogger() { return m_Log; }

    private:
        static std::shared_ptr<spdlog::logger> m_Log;
    
    };
    
}

#define LOG_ERROR(...)::LuaFsm::Log::GetLogger()->error(__VA_ARGS__)
#define LOG_WARN(...) ::LuaFsm::Log::GetLogger()->warn(__VA_ARGS__)
#define LOG_INFO(...) ::LuaFsm::Log::GetLogger()->info(__VA_ARGS__)
#define LOG_TRACE(...)::LuaFsm::Log::GetLogger()->trace(__VA_ARGS__)
