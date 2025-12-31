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
