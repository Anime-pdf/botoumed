#include "command_dispatcher.h"

#include <functional>
#include <utility>

#include <dpp/cluster.h>

#include <tasks/member_search.h>

CCommandDispatcher::CommandCallback CCommandDispatcher::ms_NotFoundCallback = NotFoundCallback;
std::map<std::string, CCommandDispatcher::CommandCallback> CCommandDispatcher::ms_CallbackMap = {
    {"ping", PingCommand},
};

void CCommandDispatcher::SetNotFoundCallback(CCommandDispatcher::CommandCallback Callback) {
    ms_NotFoundCallback = std::move(Callback);
}

dpp::task<> CCommandDispatcher::DispatchCommand(const dpp::slashcommand_t &event) {
    if (const auto it = ms_CallbackMap.find(event.command.get_command_name()); it != ms_CallbackMap.end()) {
        co_await it->second(event);
        co_return;
    }

    if (ms_NotFoundCallback) {
        co_await ms_NotFoundCallback(event);
    }
}

dpp::task<> CCommandDispatcher::OnTagCheckTick(dpp::timer timer_handle) {
    std::vector<dpp::snowflake> member_ids;
    for (const auto &user_id: event.command.get_guild().members | std::views::keys) {
        member_ids.push_back(user_id);
    }
}

dpp::task<> CCommandDispatcher::PingCommand(const dpp::slashcommand_t &event) {
    co_await event.co_reply(dpp::ir_channel_message_with_source,
                                dpp::message("Starting member scan..."));

    auto msg = (co_await event.co_get_original_response()).get<dpp::message>();

    dpp::snowflake guild_id = event.command.guild_id;

    std::vector<dpp::snowflake> member_ids;
    for (const auto &user_id: event.command.get_guild().members | std::views::keys) {
        member_ids.push_back(user_id);
    }

    MemberSearchTask task(*event.owner, guild_id, event, member_ids);
    co_await task.start();
}

dpp::task<> CCommandDispatcher::NotFoundCallback(const dpp::slashcommand_t &event) {
    co_await event.co_reply("Command not found! Command list will be updated soon, hopefully");
}
