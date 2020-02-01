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


namespace libdnf::utils::sack {


inline void tolower(std::string & s) {
    std::for_each(s.begin(), s.end(), [](char & c) { c = static_cast<char>(::tolower(c)); });
}


inline void toupper(std::string & s) {
    std::for_each(s.begin(), s.end(), [](char & c) { c = static_cast<char>(::toupper(c)); });
}



/// Query is a Set with filtering capabilities.
// TODO(dmach): consider merging Query and Set? or keep them separate for better user understanding?
template <typename T>
class Query : public Set<T> {
public:
    using FilterFunctionBool = std::function<bool(T * obj)>;
    using FilterFunctionInt64 = std::function<int64_t(T * obj)>;
    using FilterFunctionString = std::function<std::string(T * obj)>;
    using FilterFunctionCString = std::function<char * (T * obj)>;
    enum class Key;

    /// Get a single object. Raise an exception if none or multiple objects match the query.
    T get();

    /// List all objects matching the query.
    std::set<T *> list() { return get_data(); }

    void filter(Key key, QueryCmp cmp, const std::string & value);
    void filter(Key key, QueryCmp cmp, const std::vector<std::string> & value);

    void filter(Key key, QueryCmp cmp, const char * value);

    void filter(Key key, QueryCmp cmp, int64_t value);
    //void filter(Key key, QueryCmp cmp, std::vector<int64_t> values);

    void filter(Key key, QueryCmp cmp, int32_t value) { filter(key, cmp, static_cast<int64_t>(value)); }
    void filter(Key key, QueryCmp cmp, uint32_t value) { filter(key, cmp, static_cast<int64_t>(value)); }
    void filter(Key key, QueryCmp cmp, int16_t value) { filter(key, cmp, static_cast<int64_t>(value)); }
    void filter(Key key, QueryCmp cmp, uint16_t value) { filter(key, cmp, static_cast<int64_t>(value)); }
    void filter(Key key, QueryCmp cmp, int8_t value) { filter(key, cmp, static_cast<int64_t>(value)); }
    void filter(Key key, QueryCmp cmp, uint8_t value) { filter(key, cmp, static_cast<int64_t>(value)); }

    void filter(Key key, QueryCmp cmp, bool value);

    // operators; OR at least
    // copy()

protected:
    void register_filter_bool(Key key, FilterFunctionBool func) { filters_bool[key] = func; }
    void register_filter_int64(Key key, FilterFunctionInt64 func) { filters_int64[key] = func; }
    void register_filter_string(Key key, FilterFunctionString func) { filters_string[key] = func; }
    void register_filter_cstring(Key key, FilterFunctionCString func) { filters_cstring[key] = func; }
    using Set<T>::get_data;

private:
    std::map<Key, FilterFunctionBool> filters_bool;
    std::map<Key, FilterFunctionInt64> filters_int64;
    std::map<Key, FilterFunctionString> filters_string;
    std::map<Key, FilterFunctionCString> filters_cstring;
};


template <typename T>
inline void Query<T>::filter(Key key, QueryCmp cmp, const std::string & value) {
    auto getter = filters_string.at(key);

    for (auto it = get_data().begin(); it != get_data().end(); ) {
        std::string it_value = getter(*it);

        bool match = false;
        switch (cmp) {
            case QueryCmp::EXACT:
                match = it_value == value;
                break;
            case QueryCmp::NEQ:
                match = it_value != value;
                break;
            case QueryCmp::IEXACT:
                tolower(it_value);
                // tolower(value);
                match = it_value == value;
                break;
            case QueryCmp::GLOB:
               match = fnmatch(value.c_str(), it_value.c_str(), FNM_EXTMATCH) == 0;
               break;
            case QueryCmp::IGLOB:
               match = fnmatch(value.c_str(), it_value.c_str(), FNM_CASEFOLD | FNM_EXTMATCH) == 0;
               break;
            case QueryCmp::REGEX:
                {
                    std::regex re(value);
                    match = std::regex_match(it_value, re);
                }
                break;
            case QueryCmp::IREGEX:
                {
                    std::regex re(value, std::regex::icase);
                    match = std::regex_match(it_value, re);
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

        if (match) {
            ++it;
        } else {
            it = get_data().erase(it);
        }
    }
}


template <typename T>
inline void Query<T>::filter(Key key, QueryCmp cmp, const std::vector<std::string> & value) {
    auto getter = filters_string.at(key);

    for (auto it = get_data().begin(); it != get_data().end(); ) {
        std::string it_value = getter(*it);

        bool match = false;
        switch (cmp) {
            case QueryCmp::EXACT:
                match = std::find(value.begin(), value.end(), it_value) != value.end();
                break;
            case QueryCmp::NEQ:
                match = std::find(value.begin(), value.end(), it_value) == value.end();
                break;
            case QueryCmp::IEXACT:
                tolower(it_value);
                for (auto i : value) {
                    tolower(i);
                    if (i == it_value) {
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

        if (match) {
            ++it;
        } else {
            it = get_data().erase(it);
        }
    }
}


template <typename T>
inline void Query<T>::filter(Key key, QueryCmp cmp, const char * value) {
    // TODO(dmach): handle ISNULL
    filter(key, cmp, std::string(value));
}


template <typename T>
inline void Query<T>::filter(Key key, QueryCmp cmp, int64_t value) {
    auto getter = filters_int64.at(key);

    for (auto it = get_data().begin(); it != get_data().end(); ) {
        int64_t it_value = getter(*it);

        if (cmp == QueryCmp::EQ && !(it_value == value)) {
            it = get_data().erase(it);
        } else if (cmp == QueryCmp::LT && !(it_value < value)) {
            it = get_data().erase(it);
        } else if (cmp == QueryCmp::LTE && !(it_value <= value)) {
            it = get_data().erase(it);
        } else if (cmp == QueryCmp::GT && !(it_value > value)) {
            it = get_data().erase(it);
        } else if (cmp == QueryCmp::GTE && !(it_value >= value)) {
            it = get_data().erase(it);
        } else {
            ++it;
        }
    }
}


}  // namespace libdnf::utils::sack


#endif
