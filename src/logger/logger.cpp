#include "logger.h"

Logger g_Logger;

LogLevel AdaptDppLogLevel(dpp::loglevel Level)
{
    switch (Level) {
        case dpp::loglevel::ll_trace:
            return LogLevel::Trace;
        case dpp::loglevel::ll_debug:
            return LogLevel::Debug;
        case dpp::loglevel::ll_info:
            return LogLevel::Info;
        case dpp::loglevel::ll_warning:
            return LogLevel::Warning;
        case dpp::loglevel::ll_error:
            return LogLevel::Error;
        case dpp::loglevel::ll_critical:
            return LogLevel::Critical;
    }
    return LogLevel::Trace;
}
const char *LogLevelStr(LogLevel Level)
{
    switch (Level) {
        case LogLevel::Trace:
            return "Trace";
        case LogLevel::Debug:
            return "Debug";
        case LogLevel::Info:
            return "Info";
        case LogLevel::Warning:
            return "Warning";
        case LogLevel::Error:
            return "Error";
        case LogLevel::Critical:
            return "Critical";
    }
    return "Trace";
}
const char *LogLevelColor(LogLevel Level)
{
    switch (Level) {
        case LogLevel::Trace:
            return "\033[90m"; // Grey
        case LogLevel::Debug:
            return "\033[36m"; // Cyan
        case LogLevel::Info:
            return "\033[0m"; // Default
        case LogLevel::Warning:
            return "\033[33m"; // Yellow
        case LogLevel::Error:
            return "\033[31m"; // Red
        case LogLevel::Critical:
            return "\033[31m"; // Red
    }
    return "\033[0m";
}