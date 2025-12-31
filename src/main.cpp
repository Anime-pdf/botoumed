#include <command_dispatcher.h>
#include <dpp/dpp.h>
#include <logger/adapter.h>

#include <config/registry.h>

int main() {
    std::string token = "lol, github prohibited to push the code with token, thanks ig";

    dpp::cluster bot(token, dpp::i_all_intents);

    g_Logger.SetLogLevel(LogLevel::Debug);

    bot.on_log(DppLogAdapter::Logger());
    bot.on_slashcommand(CCommandDispatcher::DispatchCommand);
    bot.on_guild_member_add(CCommandDispatcher::OnMemberJoin);

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct reg>()) {
            bot.global_command_create(dpp::slashcommand("ping", "Ping pong", bot.me.id));
        }
    });

    bot.start(dpp::st_wait);

    return 0;
}
