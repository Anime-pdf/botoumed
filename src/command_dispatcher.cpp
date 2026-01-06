#include "command_dispatcher.h"

#include <functional>
#include <utility>

#include <config/registry.h>
#include <dpp/cluster.h>

#include <tasks/member_search.h>

CCommandDispatcher::CommandCallback CCommandDispatcher::ms_NotFoundCallback = NotFoundCallback;
dpp::cluster *CCommandDispatcher::ms_pCluster = nullptr;
std::map<std::string, CCommandDispatcher::CommandCallback> CCommandDispatcher::ms_CallbackMap = {
    {"config_list", ConfigListCommand},
    {"config_get", ConfigGetCommand},
    {"config_set", ConfigSetCommand},
    {"tag_check", TagCheckCommand},
};

void CCommandDispatcher::SetNotFoundCallback(CCommandDispatcher::CommandCallback Callback) {
    ms_NotFoundCallback = std::move(Callback);
}

bool HasRole(const dpp::guild_member& member, const dpp::snowflake role_id) {
    return std::ranges::find(member.get_roles(), role_id) != member.get_roles().end();
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

std::string ReplaceAll(std::string str, const std::string &from, const std::string &to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

dpp::task<> CCommandDispatcher::OnTagCheckTick() {
    dpp::snowflake guild_id;
    dpp::snowflake role_id;
    bool send_add_message = false;
    bool send_del_message = false;
    dpp::snowflake channel_id;
    std::string add_message;
    std::string del_message;
    {
        auto guild_id_str = Config().Get<std::string>("guild_id");
        if (!guild_id_str.has_value()) {
            g_Logger.Log<LogLevel::Error>("`guild_id` config var not found");
            co_return;
        }
        guild_id = guild_id_str.value();
        auto role_id_str = Config().Get<std::string>("role_id");
        if (!role_id_str.has_value()) {
            g_Logger.Log<LogLevel::Error>("`role_id` config var not found");
            co_return;
        }
        role_id = role_id_str.value();

        send_add_message = Config().Get<bool>("send_add_message").value_or(false);
        send_del_message = Config().Get<bool>("send_del_message").value_or(false);

        if (send_add_message || send_del_message) {
            auto channel_id_str = Config().Get<std::string>("message_channel_id");
            if (!channel_id_str.has_value())
                send_add_message = send_del_message = false;
            else
                channel_id = channel_id_str.value();

            auto add_message_raw = Config().Get<std::string>("add_message");
            if (!add_message_raw.has_value())
                send_add_message = false;
            else
                add_message = add_message_raw.value();

            auto del_message_raw = Config().Get<std::string>("del_message");
            if (!del_message_raw.has_value())
                send_del_message = false;
            else
                del_message = del_message_raw.value();
        }
    }

    auto CorrectGuild = [guild_id](const dpp::utility::primaryguild &guild) {
        return guild.enabled && guild.id == guild_id;
    };

    auto guild = dpp::find_guild(guild_id);
    if (guild == nullptr) {
        g_Logger.Log<LogLevel::Error>("Guild with `{}` doesn't exist", guild_id.str());
        co_return;
    }

    auto members = guild->members | std::views::values;

    co_await Cluster()->co_message_create(dpp::message(channel_id, "Starting tag check"));
    int added = 0, removed = 0, total = 0;

    for (const auto &member: members) {
        if (member.get_user()->is_bot())
            continue;

        const auto user = member.get_user();
        const bool has_role = std::ranges::find(member.get_roles(), role_id) != member.get_roles().end();

        if (CorrectGuild(user->primary_guild) && has_role)
            total++;

        if (CorrectGuild(user->primary_guild) && !has_role) {
            Cluster()->guild_member_add_role(guild_id, user->id, role_id); added++;
            if (send_add_message) {
                auto add_message_f = ReplaceAll(add_message, "{mention}", user->get_mention());
                add_message_f = ReplaceAll(add_message_f, "{username}", user->username);
                Cluster()->message_create(dpp::message(channel_id, add_message_f));
            }
            co_await Cluster()->co_sleep(1);
        } else if (!CorrectGuild(user->primary_guild) && has_role) {
            Cluster()->guild_member_delete_role(guild_id, user->id, role_id); removed++;
            if (send_del_message) {
                auto del_message_f = ReplaceAll(del_message, "{mention}", user->get_mention());
                del_message_f = ReplaceAll(del_message_f, "{username}", user->username);
                Cluster()->message_create(dpp::message(channel_id, del_message_f));
            }
            co_await Cluster()->co_sleep(1);
        }
    }

    total += added;
    co_await Cluster()->co_message_create(dpp::message(channel_id, std::format("Tag check finished. Added: **{}**, Removed: **{}**. New total tag members: **{}**", added, removed, total)));
}

dpp::task<> CCommandDispatcher::TagCheckCommand(const dpp::slashcommand_t &event) {
    if (!HasRole(event.command.member, Config().Get<std::string>("admin_role").value_or("0"))) {
        co_await event.co_reply("No permission");
        co_return;
    }
    co_await OnTagCheckTick();
}

dpp::task<> CCommandDispatcher::ConfigListCommand(const dpp::slashcommand_t &event) {
    if (!HasRole(event.command.member, Config().Get<std::string>("admin_role").value_or("0"))) {
        co_await event.co_reply("No permission");
        co_return;
    }

    const auto vars = Config().ListAll();

    std::string msg = std::format("`{}` config vars:\n", vars.size());
    for (const auto &var_name: vars) {
        auto info = Config().GetInfo(var_name);
        msg += std::format("`{}` ({}) = `{}`)\n", info->name, info->type, info->readonly ? "REDACTED" : info->value);
    }

    co_await event.co_reply(dpp::message(msg).set_flags(dpp::m_ephemeral));
}

dpp::task<> CCommandDispatcher::ConfigGetCommand(const dpp::slashcommand_t &event) {
    if (!HasRole(event.command.member, Config().Get<std::string>("admin_role").value_or("0"))) {
        co_await event.co_reply("No permission");
        co_return;
    }

    auto var_name = std::get<std::string>(event.get_parameter("name"));

    auto var = Config().GetInfo(var_name);

    std::string msg = std::format("`{}` not found", var_name);
    if (var.has_value()) {
        msg = std::format("`{}` ({}) = `{}`)\n", var->name, var->type, var->readonly ? "REDACTED" : var->value);
    }

    co_await event.co_reply(dpp::message(msg).set_flags(dpp::m_ephemeral));
}

dpp::task<> CCommandDispatcher::ConfigSetCommand(const dpp::slashcommand_t &event) {
    if (!HasRole(event.command.member, Config().Get<std::string>("admin_role").value_or("0"))) {
        co_await event.co_reply("No permission");
        co_return;
    }

    auto var_name = std::get<std::string>(event.get_parameter("name"));
    auto var_value = std::get<std::string>(event.get_parameter("value"));

    std::string msg;

    if (const auto result = Config().Set(var_name, var_value); !result.has_value()) {
        msg = std::format("Can't set `{}`: {}", var_name, result.error());
    } else {
        msg = std::format("`{}` was set to `{}`", var_name, var_value);
    }

    co_await event.co_reply(dpp::message(msg).set_flags(dpp::m_ephemeral));
}

dpp::task<> CCommandDispatcher::NotFoundCallback(const dpp::slashcommand_t &event) {
    co_await event.co_reply("Command not found! Command list will be updated soon, hopefully");
}
