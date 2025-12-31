#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include <dpp/dispatcher.h>

class CCommandDispatcher final {
    typedef std::function<dpp::task<>(dpp::slashcommand_t)> CommandCallback;

    static std::map<std::string, CommandCallback> ms_CallbackMap;
    static CommandCallback ms_NotFoundCallback;
    static dpp::cluster *ms_pCluster;

    static dpp::task<> ConfigListCommand(const dpp::slashcommand_t& event);
    static dpp::task<> ConfigGetCommand(const dpp::slashcommand_t& event);
    static dpp::task<> ConfigSetCommand(const dpp::slashcommand_t& event);

    static dpp::task<> NotFoundCallback(const dpp::slashcommand_t& event);

public:
    static void SetCluster(dpp::cluster *pCluster) {
        ms_pCluster = pCluster;
    }
    static dpp::cluster *Cluster() {
        return ms_pCluster;
    }

    static void SetNotFoundCallback(CommandCallback Callback);

    static dpp::task<> DispatchCommand(const dpp::slashcommand_t& event);
    static dpp::task<> OnTagCheckTick(const dpp::timer& timer_handle);
};

#endif // COMMAND_DISPATCHER_H
