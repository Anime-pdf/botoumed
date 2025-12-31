#ifndef LOGGER_H
#define LOGGER_H

#include <format>
#include <iostream>

#include <dpp/misc-enum.h>

#include <utility/utils.h>

enum class LogLevel
{
    Trace = 0,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
};

LogLevel AdaptDppLogLevel(dpp::loglevel Level);

const char *LogLevelStr(LogLevel Level);
const char *LogLevelColor(LogLevel Level);

class Logger
{
    LogLevel m_Level = LogLevel::Info;

public:
    void SetLogLevel(LogLevel Level)
    {
        m_Level = Level;
    }

    template<LogLevel Level = LogLevel::Debug, typename... TArgs>
    void Log(std::format_string<TArgs...> Fmt, TArgs&&... Args) const
    {
        if(Level < m_Level)
            return;
        std::cout << std::format("{}[{}] {}: {}", LogLevelColor(Level), current_date_time(), LogLevelStr(Level), std::format(Fmt, std::forward<TArgs>(Args)...)) << "\n" << std::flush;
    }

    template<typename... TArgs>
    void Log(LogLevel Level, std::format_string<TArgs...> Fmt, TArgs&&... Args) const
    {
        if(Level < m_Level)
            return;
        std::cout << std::format("{}[{}] {}: {}", LogLevelColor(Level), current_date_time(), LogLevelStr(Level), std::format(Fmt, std::forward<TArgs>(Args)...)) << "\n" << std::flush;
    }
};

extern Logger g_Logger;

#endif // LOGGER_H