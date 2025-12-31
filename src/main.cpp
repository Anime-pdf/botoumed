#include <command_dispatcher.h>
#include <dpp/dpp.h>
#include <logger/adapter.h>

#include <config/registry.h>
#include <config/validator/builder.h>

int main() {
    // common
    CONFIG_STRING_READONLY("token", "", Validators::StringNonEmpty());
    CONFIG_STRING_READONLY("admin_role", "", Validators::StringNonEmpty());
    // tag check
    CONFIG_INT("tag_check_interval", 5, Validators::IntRanged(1, 600));
    CONFIG_STRING("guild_id", "", Validators::StringNonEmpty());
    CONFIG_STRING("role_id", "", Validators::StringNonEmpty());
    CONFIG_BOOLEAN("send_add_message", false, Validators::Boolean());
    CONFIG_BOOLEAN("send_del_message", false, Validators::Boolean());
    CONFIG_STRING("message_channel_id", "", Validators::StringNonEmpty());
    CONFIG_STRING("add_message", "{mention} set our server tag and got exclusive role!", Validators::StringNonEmpty());
    CONFIG_STRING("del_message", "{mention} changed his server tag and lost exclusive role!", Validators::StringNonEmpty());

    Config().SetConfigPath("config.json");
    auto result = Config().Load();
    if (!result.has_value()) {
        g_Logger.Log<LogLevel::Critical>("Can't load config: {}", result.error());
        if (result.error() == "File doesn't exist") { // idk what i'm doing
            g_Logger.Log<LogLevel::Critical>("Creating default file");
            result = Config().Save();
            if (!result.has_value()) {
                g_Logger.Log<LogLevel::Critical>("Can't create file with default config: {}", result.error());
            }
        }
        return 1;
    }

    const auto token = Config().Get<std::string>("token");
    if (!token.has_value()) {
        g_Logger.Log<LogLevel::Critical>("Token not set");
        return 1;
    }

    dpp::cluster bot(token.value(), dpp::i_all_intents);
    CCommandDispatcher::SetCluster(&bot);

    g_Logger.SetLogLevel(LogLevel::Debug);

    bot.on_log(DppLogAdapter::Logger());
    bot.on_slashcommand(CCommandDispatcher::DispatchCommand);

    bot.on_ready([&bot](const dpp::ready_t& event) {
        g_Logger.Log<LogLevel::Info>("Started as {}. Guild: {}", bot.me.format_username(), event.guild_count);
        g_Logger.Log<LogLevel::Info>("Config vars: {}", Config().ListAll().size());

        bot.start_timer(CCommandDispatcher::OnTagCheckTick, Config().Get<int>("tag_check_interval").value_or(10) * 60UL);

        if (dpp::run_once<struct reg>()) {
            auto config_list = dpp::slashcommand("config_list", "manage config vars", bot.me.id);

            auto config_get = dpp::slashcommand("config_get", "get config vars", bot.me.id);
            config_get.add_option(dpp::command_option(dpp::co_string, "name", "variable name", true));

            auto config_set = dpp::slashcommand("config_set", "set config vars", bot.me.id);
            config_set.add_option(dpp::command_option(dpp::co_string, "name", "variable name", true));
            config_set.add_option(dpp::command_option(dpp::co_string, "value", "new value", true));

            auto tag_check = dpp::slashcommand("tag_check", "manually start check", bot.me.id);

            // register
            const auto commands = { config_list, config_get, config_set, tag_check };
            bot.global_bulk_command_create(commands);
        }
    });

    bot.start(dpp::st_wait);

    result = Config().Save();
    if (!result.has_value()) {
        g_Logger.Log<LogLevel::Critical>("Can't save config file");
    }

    return 0;
}
