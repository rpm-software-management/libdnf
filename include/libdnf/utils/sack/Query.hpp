#ifndef LIBDNF_UTILS_SACK_QUERY_HPP
#define LIBDNF_UTILS_SACK_QUERY_HPP


#include "QueryCmp.hpp"
#include "Set.hpp"

#include <fnmatch.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <regex>
#include <set>
#include <string>
#include <variant>


namespace libdnf::utils::sack {


void tolower(std::string & s) {
    std::for_each(s.begin(), s.end(), [](char & c) { c = static_cast<char>(::tolower(c)); });
}


void toupper(std::string & s) {
    std::for_each(s.begin(), s.end(), [](char & c) { c = static_cast<char>(::toupper(c)); });
}



/// Query is a Set with filtering capabilities.
// TODO(dmach): consider merging Query and Set? or keep them separate for better user understanding?
template <typename T>
class Query : public Set<T> {
public:
    using FilterValueTypes = std::variant<std::string, char *, bool, int64_t, int32_t, uint32_t, int16_t, uint16_t>;
    using FilterFunction = std::function<FilterValueTypes(T * obj)>;

    Query();

    /// Get a single object. Raise an exception if none or multiple objects match the query.
    T get();

    /// List all objects matching the query.
    std::set<T *> list() { return get_data(); }

    void filter(const std::string & key, QueryCmp cmp, const std::string & value);
    void filter(const std::string & key, QueryCmp cmp, const std::vector<std::string> & values);

    void filter(const std::string & key, QueryCmp cmp, const char * value);

//    void filter(const std::string & key, QueryCmp cmp, int32_t value);
//    void filter(const std::string & key, QueryCmp cmp, std::vector<int32_t> values);

    void filter(const std::string & key, QueryCmp cmp, int64_t value);
    void filter(const std::string & key, QueryCmp cmp, int32_t value);

//     { filter(key, cmp, static_cast<int64_t>(value)); }
//    void filter(const std::string & key, QueryCmp cmp, uint32_t value) { filter(key, cmp, static_cast<int64_t>(value)); }
//    void filter(const std::string & key, QueryCmp cmp, int16_t value) { filter(key, cmp, static_cast<int64_t>(value)); }
//    void filter(const std::string & key, QueryCmp cmp, uint16_t value) { filter(key, cmp, static_cast<int64_t>(value)); }
//    void filter(const std::string & key, QueryCmp cmp, int8_t value) { filter(key, cmp, static_cast<int64_t>(value)); }
//    void filter(const std::string & key, QueryCmp cmp, uint8_t value) { filter(key, cmp, static_cast<int64_t>(value)); }

    // void filter(const std::string & key, QueryCmp cmp, std::vector<int64_t> values);

    void filter(const std::string & key, QueryCmp cmp, bool value);

    // operators; OR at least
    // copy()

protected:
    void initialize_filters() {}
    void add_filter(std::string key, FilterFunction func) { filters[key] = func; }
    using Set<T>::get_data;

private:
    std::map<std::string, FilterFunction> filters;
};


template <typename T>
Query<T>::Query() {
    initialize_filters();
}



template <typename T>
inline void Query<T>::filter(const std::string & key, QueryCmp cmp, const std::string & value) {
    auto getter = filters.at(key);

    for (auto it = get_data().begin(); it != get_data().end(); ) {
        auto it_variant = getter(*it);
        auto it_value = std::get_if<std::string>(&it_variant);

        if (it_value == NULL) {
            // TODO(dmach): consider raising an error rather than excluding invalid matches
            it = get_data().erase(it);
            continue;
        }

        bool match = false;
        switch (cmp) {
            case QueryCmp::EXACT:
                match = *it_value == value;
                break;
            case QueryCmp::IEXACT:
                tolower(*it_value);
                // tolower(value);
                match = *it_value == value;
                break;
            case QueryCmp::GLOB:
               match = fnmatch(value.c_str(), (*it_value).c_str(), FNM_EXTMATCH) == 0;
               break;
            case QueryCmp::IGLOB:
               match = fnmatch(value.c_str(), (*it_value).c_str(), FNM_CASEFOLD | FNM_EXTMATCH) == 0;
               break;
            case QueryCmp::REGEX:
                {
                    std::regex re(value);
                    match = std::regex_match(*it_value, re);
                }
                break;
            case QueryCmp::IREGEX:
                {
                    std::regex re(value, std::regex::icase);
                    match = std::regex_match(*it_value, re);
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
            case QueryCmp::EQ:
            case QueryCmp::LT:
            case QueryCmp::LTE:
            case QueryCmp::GT:
            case QueryCmp::GTE:
                throw std::runtime_error("Unsupported operator");
                break;
        }

        if (match) {
            ++it;
        } else {
            it = get_data().erase(it);
        }
    }
}


template <typename T>
inline void Query<T>::filter(const std::string & key, QueryCmp cmp, const std::vector<std::string> & value) {
    auto getter = filters.at(key);

    for (auto it = get_data().begin(); it != get_data().end(); ) {
        auto it_variant = getter(*it);
        auto it_value = std::get_if<std::string>(&it_variant);

        if (it_value == NULL) {
            // TODO(dmach): consider raising an error rather than excluding invalid matches
            it = get_data().erase(it);
            continue;
        }

        bool match = false;
        switch (cmp) {
            case QueryCmp::EXACT:
                match = std::find(value.begin(), value.end(), *it_value) != value.end();
                break;
            case QueryCmp::IEXACT:
                tolower(*it_value);
                for (auto i : value) {
                    tolower(i);
                    if (i == *it_value) {
                        match = true;
                        break;
                    }
                }
                break;
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
                throw std::runtime_error("Not implemented yet");
                break;
            case QueryCmp::ISNULL:
            case QueryCmp::EQ:
            case QueryCmp::LT:
            case QueryCmp::LTE:
            case QueryCmp::GT:
            case QueryCmp::GTE:
                throw std::runtime_error("Unsupported operator");
                break;
        }

        if (match) {
            ++it;
        } else {
            it = get_data().erase(it);
        }
    }
}


template <typename T>
inline void Query<T>::filter(const std::string & key, QueryCmp cmp, const char * value) {
    // TODO(dmach): handle ISNULL
    filter(key, cmp, std::string(value));
}


template <typename T>
inline void Query<T>::filter(const std::string & key, QueryCmp cmp, int64_t value) {
    auto getter = filters.at(key);

    for (auto it = get_data().begin(); it != get_data().end(); ) {
        auto it_variant = getter(*it);
        auto it_value = std::get_if<int64_t>(&it_variant);

        if (it_value == NULL) {
            // TODO(dmach): consider raising an error rather than excluding invalid matches
            it = get_data().erase(it);
            continue;
        }

        if (cmp == QueryCmp::EQ && !(*it_value == value)) {
            it = get_data().erase(it);
        } else if (cmp == QueryCmp::LT && !(*it_value < value)) {
            it = get_data().erase(it);
        } else if (cmp == QueryCmp::LTE && !(*it_value <= value)) {
            it = get_data().erase(it);
        } else if (cmp == QueryCmp::GT && !(*it_value > value)) {
            it = get_data().erase(it);
        } else if (cmp == QueryCmp::GTE && !(*it_value >= value)) {
            it = get_data().erase(it);
        } else {
            ++it;
        }
    }
}


template <typename T>
inline void Query<T>::filter(const std::string & key, QueryCmp cmp, int32_t value) {
    auto getter = filters.at(key);

    for (auto it = get_data().begin(); it != get_data().end(); ) {
        auto it_variant = getter(*it);
        auto it_value = std::get_if<int32_t>(&it_variant);

        if (it_value == NULL) {
            // TODO(dmach): consider raising an error rather than excluding invalid matches
            it = get_data().erase(it);
            continue;
        }

        if (cmp == QueryCmp::EQ && !(*it_value == value)) {
            it = get_data().erase(it);
        } else if (cmp == QueryCmp::LT && !(*it_value < value)) {
            it = get_data().erase(it);
        } else if (cmp == QueryCmp::LTE && !(*it_value <= value)) {
            it = get_data().erase(it);
        } else if (cmp == QueryCmp::GT && !(*it_value > value)) {
            it = get_data().erase(it);
        } else if (cmp == QueryCmp::GTE && !(*it_value >= value)) {
            it = get_data().erase(it);
        } else {
            ++it;
        }
    }
}


}  // namespace libdnf::utils::sack


#endif
