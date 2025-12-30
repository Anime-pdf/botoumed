#ifndef LOGGER_ADAPTER_H
#define LOGGER_ADAPTER_H

#include <functional>

#include <dpp/dispatcher.h>

#include "logger.h"

class DppLogAdapter
{
public:
    static std::function<void(const dpp::log_t &Log)> Logger()
    {
        return [](const dpp::log_t& event) {
            g_Logger.Log(AdaptDppLogLevel(event.severity), "{}", event.message);
        };
    }
};

#endif // LOGGER_ADAPTER_H
