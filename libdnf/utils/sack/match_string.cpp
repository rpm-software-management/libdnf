#include "libdnf/utils/sack/match_string.hpp"

#include <fnmatch.h>

//#include <algorithm>
//#include <cstdint>
//#include <functional>
#include <regex>
#include <stdexcept>
//#include <set>


namespace libdnf::utils::sack {


inline std::string tolower(const std::string & s) {
    std::string result = s;
    std::for_each(result.begin(), result.end(), [](char & c) { c = static_cast<char>(::tolower(c)); });
    return result;
}


bool match_string(const std::string & value, QueryCmp cmp, const std::string & pattern) {
    bool result = false;
    switch (cmp) {
        case QueryCmp::EXACT:
                result = value == pattern;
                break;
            case QueryCmp::NEQ:
                result = value != pattern;
                break;
            case QueryCmp::IEXACT:
                result = tolower(value) == tolower(pattern);
                break;
            case QueryCmp::GLOB:
               result = fnmatch(pattern.c_str(), value.c_str(), FNM_EXTMATCH) == 0;
               break;
            case QueryCmp::IGLOB:
               result = fnmatch(pattern.c_str(), value.c_str(), FNM_CASEFOLD | FNM_EXTMATCH) == 0;
               break;
            case QueryCmp::REGEX:
                {
                    std::regex re(pattern);
                    result = std::regex_match(value, re);
                }
                break;
            case QueryCmp::IREGEX:
                {
                    std::regex re(pattern, std::regex::icase);
                    result = std::regex_match(value, re);
                }
                break;
            case QueryCmp::CONTAINS:
            case QueryCmp::ICONTAINS:
            case QueryCmp::STARTSWITH:
            case QueryCmp::ISTARTSWITH:
            case QueryCmp::ENDSWITH:
            case QueryCmp::IENDSWITH:
                throw std::runtime_error("Not implemented yet");
                break;
            case QueryCmp::ISNULL:
            case QueryCmp::LT:
            case QueryCmp::LTE:
            case QueryCmp::GT:
            case QueryCmp::GTE:
                throw std::runtime_error("Unsupported operator");
                break;
            case QueryCmp::NOT:
            case QueryCmp::ICASE:
                throw std::runtime_error("Operator flag cannot be used standalone");
                break;
        }
    return result;
}


bool match_string(const std::string & value, QueryCmp cmp, const std::vector<std::string> & patterns) {
    bool result = false;
    for (auto & pattern : patterns) {
        if (match_string(value, cmp, pattern)) {
            result = true;
            break;
        }
    }
    return result;
}


bool match_string(const std::vector<std::string> & values, QueryCmp cmp, const std::string & pattern) {
    bool result = false;
    for (auto & value : values) {
        if (match_string(value, cmp, pattern)) {
            result = true;
            break;
        }
    }
    return result;
}


bool match_string(const std::vector<std::string> & values, QueryCmp cmp, const std::vector<std::string> & patterns) {
    bool result = false;
    bool found = false;
    for (auto & value : values) {
        for (auto & pattern : patterns) {
            if (match_string(value, cmp, pattern)) {
                result = true;
                found = true;
                break;
            }
        }
        if (found) {
            break;
        }
    }
    return result;
}


}  // namespace libdnf::utils::sack
