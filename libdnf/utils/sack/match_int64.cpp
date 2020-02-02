#include "libdnf/utils/sack/match_int64.hpp"

#include <stdexcept>


namespace libdnf::utils::sack {


bool match_int64(int64_t value, QueryCmp cmp, int64_t pattern) {
    bool result = false;
    switch (cmp) {
        case QueryCmp::EQ:
            result = value == pattern;
            break;
        case QueryCmp::NEQ:
            result = value != pattern;
            break;
        case QueryCmp::LT:
            result = value < pattern;
            break;
        case QueryCmp::LTE:
            result = value <= pattern;
            break;
        case QueryCmp::GT:
            result = value > pattern;
            break;
        case QueryCmp::GTE:
            result = value >= pattern;
            break;
        case QueryCmp::IEXACT:
        case QueryCmp::GLOB:
        case QueryCmp::IGLOB:
        case QueryCmp::REGEX:
        case QueryCmp::IREGEX:
        case QueryCmp::CONTAINS:
        case QueryCmp::ICONTAINS:
        case QueryCmp::STARTSWITH:
        case QueryCmp::ISTARTSWITH:
        case QueryCmp::ENDSWITH:
        case QueryCmp::IENDSWITH:
        case QueryCmp::ISNULL:
            throw std::runtime_error("Unsupported operator");
            break;
        case QueryCmp::NOT:
        case QueryCmp::ICASE:
            throw std::runtime_error("Operator flag cannot be used standalone");
            break;
    }
    return result;
}


bool match_int64(int64_t value, QueryCmp cmp, const std::vector<int64_t> & patterns) {
    bool result = false;
    for (auto & pattern : patterns) {
        if (match_int64(value, cmp, pattern)) {
            result = true;
            break;
        }
    }
    return result;
}


bool match_int64(const std::vector<int64_t> & values, QueryCmp cmp, int64_t pattern) {
    bool result = false;
    for (auto & value : values) {
        if (match_int64(value, cmp, pattern)) {
            result = true;
            break;
        }
    }
    return result;
}


bool match_int64(const std::vector<int64_t> & values, QueryCmp cmp, const std::vector<int64_t> & patterns) {
    bool result = false;
    bool found = false;
    for (auto & value : values) {
        for (auto & pattern : patterns) {
            if (match_int64(value, cmp, pattern)) {
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


