#ifndef UTILITY_UTILS_H
#define UTILITY_UTILS_H

#include <string>

#include <dpp/coro/task.h>

namespace dpp {
    class snowflake;
    class cluster;
}

inline std::string rtrim(std::string s);
inline std::string ltrim(std::string s);
inline std::string trim(std::string s);

std::string current_date_time();

#endif // UTILITY_UTILS_H
