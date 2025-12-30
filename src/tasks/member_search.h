#ifndef BOTOUMED_MEMBER_SEARCH_H
#define BOTOUMED_MEMBER_SEARCH_H

#include <dpp/dpp.h>
#include <chrono>
#include <utility>

#include "logger/logger.h"

class MemberSearchTask {
    dpp::cluster &bot;
    dpp::snowflake guild_id;
    const dpp::slashcommand_t &interaction;
    std::vector<dpp::snowflake> members_to_check;

    size_t current_index = 0;
    size_t members_with_messages = 0;
    std::chrono::steady_clock::time_point start_time;

    // Rate limit tracking
    int rate_limit_remaining = -1;
    double rate_limit_reset_after = 0.0;

public:
    MemberSearchTask(dpp::cluster &bot, dpp::snowflake guild_id,
                     const dpp::slashcommand_t &interaction,
                     const std::vector<dpp::snowflake> &members)
        : bot(bot), guild_id(guild_id), interaction(interaction),
          members_to_check(members),
          start_time(std::chrono::steady_clock::now()) {
    }

    dpp::task<void> start() {
        co_await update_progress("Starting member search...");

        for (current_index = 0; current_index < members_to_check.size(); current_index++) {
            dpp::snowflake user_id = members_to_check[current_index];

            bool retry = true;
            while (retry) {
                auto result = co_await search_member(user_id);
                retry = co_await handle_search_result(result, user_id);
            }

            // Update progress every 10 members or on last member
            if ((current_index + 1) % 10 == 0 || current_index + 1 >= members_to_check.size()) {
                co_await update_progress();
            }

            // Add small delay to avoid hammering the API
            if (rate_limit_remaining <= 5 && rate_limit_remaining > 0) {
                co_await bot.co_sleep(1);
            } else {
                // co_await bot.co_sleep(1);
            }
        }

        co_await finish();
    }

private:
    dpp::task<void> update_progress(const std::string &status = "") {
        double progress = (double) (current_index + 1) / members_to_check.size();
        int percentage = (int) (progress * 100);

        // Calculate ETA
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();

        std::string eta_str = "calculating...";
        if (current_index > 0) {
            double avg_time_per_member = (double) elapsed / (current_index + 1);
            int remaining_members = members_to_check.size() - (current_index + 1);
            int eta_seconds = (int) (avg_time_per_member * remaining_members);

            int eta_minutes = eta_seconds / 60;
            int eta_secs = eta_seconds % 60;

            if (eta_minutes > 0) {
                eta_str = std::to_string(eta_minutes) + "m " + std::to_string(eta_secs) + "s";
            } else {
                eta_str = std::to_string(eta_secs) + "s";
            }
        }

        std::string progress_bar = create_progress_bar(progress);

        std::string content = "**Scanning server members for activity**\n\n";
        content += progress_bar + " " + std::to_string(percentage) + "%\n";
        content += "Progress: " + std::to_string(current_index + 1) + "/" +
                std::to_string(members_to_check.size()) + " members\n";
        content += "Active members found: " + std::to_string(members_with_messages) + "\n";
        content += "ETA: " + eta_str + "\n";

        if (!status.empty()) {
            content += "\n*" + status + "*";
        }

        if (rate_limit_remaining >= 0) {
            content += "\nRate limit: " + std::to_string(rate_limit_remaining) + " requests remaining";
        }

        co_await interaction.co_edit_original_response(dpp::message(content));
    }

    static std::string create_progress_bar(double progress, int width = 20) {
        int filled = (int) (progress * width);
        std::string bar = "[";
        for (int i = 0; i < width; i++) {
            if (i < filled) {
                bar += "█";
            } else {
                bar += "░";
            }
        }
        bar += "]";
        return bar;
    }

    dpp::task<dpp::http_request_completion_t> search_member(dpp::snowflake user_id) {
        std::string url = "https://discord.com/api/v10/guilds/" +
                          std::to_string(guild_id) +
                          "/messages/search?author_id=" + std::to_string(user_id) +
                          "&limit=25";

        co_return co_await bot.co_request(url, dpp::m_get, "", "application/json",
                                          {
                                              {"Authorization", std::format("Bot {}", bot.token)}
                                          });
    }

    dpp::task<bool> handle_search_result(const dpp::http_request_completion_t &result, dpp::snowflake user_id) {
        // Extract rate limit headers
        auto headers = result.headers;
        if (headers.contains("x-ratelimit-remaining")) {
            rate_limit_remaining = std::stoi(headers.find("x-ratelimit-remaining")->second);
        }
        if (headers.contains("x-ratelimit-reset-after")) {
            rate_limit_reset_after = std::stod(headers.find("x-ratelimit-reset-after")->second);
        }

        if (result.status == 429) {
            // Rate limited!
            int retry_after = 5; // Default fallback

            try {
                nlohmann::json error_body = nlohmann::json::parse(result.body);
                if (error_body.contains("retry_after")) {
                    retry_after = (int) std::ceil(error_body["retry_after"].get<double>());
                }
            } catch (...) {
                // Use rate_limit_reset_after if available
                if (rate_limit_reset_after > 0) {
                    retry_after = (int) std::ceil(rate_limit_reset_after);
                }
            }

            co_await update_progress("Rate limited! Waiting " + std::to_string(retry_after) + " seconds...");
            co_await bot.co_sleep(retry_after);

            co_return true; // Retry
        }

        if (result.status == 200) {
            try {
                nlohmann::json response = nlohmann::json::parse(result.body);

                if (response.contains("messages") && !response["messages"].empty()) {
                    for (const auto &msg: response["messages"]) {
                        int type = msg[0]["type"].get<int>();

                        // 0=DEFAULT, 19=REPLY, 20=CHAT_INPUT_COMMAND, 21=CONTEXT_MENU_COMMAND
                        if (type == 0 || type == 19 || type == 20 || type == 21) {
                            members_with_messages++;
                            break;
                        }
                    }
                }
            } catch (const std::exception &e) {
                bot.log(dpp::ll_error, "Error parsing response for user " + std::to_string(user_id) + ": " + e.what());
            }
        } else if (result.status == 202) {
            // Search index not ready, we'll just skip this one
            bot.log(dpp::ll_warning, "Search index not ready for user " + std::to_string(user_id));
        } else {
            bot.log(dpp::ll_error,
                    "HTTP error " + std::to_string(result.status) + " for user " + std::to_string(user_id));
            bot.log(dpp::ll_error, result.body);
        }

        co_return false; // Don't retry
    }

    dpp::task<void> finish() const {
        auto end_time = std::chrono::steady_clock::now();
        auto total_time = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

        int minutes = total_time / 60;
        int seconds = total_time % 60;

        std::string time_str = std::to_string(minutes) + "m " + std::to_string(seconds) + "s";

        std::string content = "**✅ Member scan complete!**\n\n";
        content += "Total members checked: " + std::to_string(members_to_check.size()) + "\n";
        content += "Active members found: " + std::to_string(members_with_messages) + "\n";
        content += "Inactive members: " + std::to_string(members_to_check.size() - members_with_messages) + "\n";
        content += "Time taken: " + time_str;

        co_await interaction.co_edit_original_response(dpp::message(content));
    }
};

#endif // BOTOUMED_MEMBER_SEARCH_H
