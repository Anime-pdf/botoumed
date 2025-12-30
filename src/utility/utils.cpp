#include "utils.h"

#include <sstream>
#include <iomanip>

#include <dpp/dpp.h>

#include "logger/logger.h"

inline std::string rtrim(std::string s) {
    s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
    return s;
}

inline std::string ltrim(std::string s) {
    s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
    return s;
}

inline std::string trim(std::string s) {
    return ltrim(rtrim(s));
}

std::string current_date_time() {
#ifdef _WIN32
    std::time_t curr_time = time(nullptr);
    return trim(std::ctime(&curr_time));
#else
    auto t = std::time(nullptr);
    struct tm timedata;
    localtime_r(&t, &timedata);
    std::stringstream s;
    s << std::put_time(&timedata, "%Y-%m-%d %H:%M:%S");
    return trim(s.str());
#endif
}

dpp::task<bool> MemberHasRealMessages(dpp::cluster *pCluster, dpp::snowflake guildId, dpp::snowflake memberId) {
    const std::string url = std::format("https://discord.com/api/v10/guilds/{}/messages/search?author_id={}&limit={}",
                                        guildId.str(),
                                        memberId.str(),
                                        25);

    const dpp::http_request_completion_t &result =
            co_await pCluster->co_request(
                url,
                dpp::m_get, "", "application/json",
                {
                    {"Authorization", std::format("Bot {}", pCluster->token)}
                });

    if (result.status == 200) {
        nlohmann::json response = nlohmann::json::parse(result.body);

        if (!response.contains("messages") || response["messages"].empty()) {
            co_return false;
        }

        for (const auto &msg: response["messages"]) {
            int type = msg[0]["type"].get<int>();

            // 0 = DEFAULT, 19 = REPLY, 20 = CHAT_INPUT_COMMAND, 21 = CONTEXT_MENU_COMMAND
            if (type == 0 || type == 19 || type == 20 || type == 21) {
                co_return true;
            }
        }

        g_Logger.Log("User {} only has system messages (joins/boosts/etc)", memberId.str());
    } else if (result.status == 202) {
        g_Logger.Log("Search index not ready");
    } else {
        g_Logger.Log("Error({}): {}", result.status, result.body);
    }

    co_return false;
}
