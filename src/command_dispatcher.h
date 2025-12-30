#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include <dpp/dispatcher.h>

class CCommandDispatcher final {
    typedef std::function<dpp::task<>(dpp::slashcommand_t)> CommandCallback;

    static std::map<std::string, CommandCallback> ms_CallbackMap;
    static CommandCallback ms_NotFoundCallback;

    static dpp::task<> PingCommand(const dpp::slashcommand_t& event);
    static dpp::task<> NotFoundCallback(const dpp::slashcommand_t& event);

public:
    static void SetNotFoundCallback(CommandCallback Callback);

    static dpp::task<> DispatchCommand(const dpp::slashcommand_t& event);
    static dpp::task<> OnTagCheckTick(dpp::timer timer_handle);

    static dpp::task<> OnMemberJoin(const dpp::guild_member_add_t& event);
};

#endif // COMMAND_DISPATCHER_H
