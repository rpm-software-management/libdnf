#ifndef LIBDNF_UTILS_SACK_QUERY_HPP
#define LIBDNF_UTILS_SACK_QUERY_HPP


#include "QueryCmp.hpp"
#include "Set.hpp"
#include "match_int64.hpp"
#include "match_string.hpp"

#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>


namespace libdnf::utils::sack {


/// Query is a Set with filtering capabilities.
// TODO(dmach): consider merging Query and Set? or keep them separate for better user understanding?
template <typename T>
class Query : public Set<T> {
public:
    // forward declaration, each template specialization must define the values
    enum class Key;

    /// Get a single object. Raise an exception if none or multiple objects match the query.
    const T & get() const {
        if (get_data().size() == 1) {
            return *get_data().begin();
        }
        throw std::runtime_error("Query must contain exactly one object.");
    }

    /// List all objects matching the query.
    const std::set<T> & list() const noexcept { return get_data(); }

    std::size_t filter(Key key, QueryCmp cmp, const std::string & pattern);
    std::size_t filter(Key key, QueryCmp cmp, const std::vector<std::string> & patterns);

    std::size_t filter(Key key, QueryCmp cmp, const char * pattern);
    //std::size_t filter(Key key, QueryCmp cmp, const std::vector<char *> pattern);

    std::size_t filter(Key key, QueryCmp cmp, int64_t pattern);
    std::size_t filter(Key key, QueryCmp cmp, const std::vector<int64_t> & patterns);

    std::size_t filter(Key key, QueryCmp cmp, int32_t pattern) { return filter(key, cmp, static_cast<int64_t>(pattern)); }
    std::size_t filter(Key key, QueryCmp cmp, uint32_t pattern) { return filter(key, cmp, static_cast<int64_t>(pattern)); }
    std::size_t filter(Key key, QueryCmp cmp, int16_t pattern) { return filter(key, cmp, static_cast<int64_t>(pattern)); }
    std::size_t filter(Key key, QueryCmp cmp, uint16_t pattern) { return filter(key, cmp, static_cast<int64_t>(pattern)); }
    std::size_t filter(Key key, QueryCmp cmp, int8_t pattern) { return filter(key, cmp, static_cast<int64_t>(pattern)); }
    std::size_t filter(Key key, QueryCmp cmp, uint8_t pattern) { return filter(key, cmp, static_cast<int64_t>(pattern)); }

    std::size_t filter(Key key, QueryCmp cmp, bool pattern);

    // operators; OR at least
    // copy()

    using Set<T>::get_data;

protected:
    using FilterFunctionBool = std::function<bool(const T & obj)>;
    using FilterFunctionCString = std::function<char *(const T & obj)>;
    using FilterFunctionInt64 = std::function<int64_t(const T & obj)>;
    using FilterFunctionString = std::function<std::string(const T & obj)>;
    using FilterFunctionVectorInt64 = std::function<std::vector<int64_t>(const T & obj)>;
    using FilterFunctionVectorString = std::function<std::vector<std::string>(const T & obj)>;

    void register_filter_bool(Key key, FilterFunctionBool func) { filters_bool[key] = func; }
    void register_filter_cstring(Key key, FilterFunctionCString func) { filters_cstring[key] = func; }
    void register_filter_int64(Key key, FilterFunctionInt64 func) { filters_int64[key] = func; }
    void register_filter_string(Key key, FilterFunctionString func) { filters_string[key] = func; }
    void register_filter_vector_int64(Key key, FilterFunctionVectorInt64 func) { filters_vector_int64[key] = func; }
    void register_filter_vector_string(Key key, FilterFunctionVectorString func) { filters_vector_string[key] = func; }

private:
    std::map<Key, FilterFunctionBool> filters_bool;
    std::map<Key, FilterFunctionCString> filters_cstring;
    std::map<Key, FilterFunctionInt64> filters_int64;
    std::map<Key, FilterFunctionString> filters_string;
    std::map<Key, FilterFunctionVectorInt64> filters_vector_int64;
    std::map<Key, FilterFunctionVectorString> filters_vector_string;
};


template <typename T>
inline std::size_t Query<T>::filter(Key key, QueryCmp cmp, const std::string & pattern) {
    std::size_t filtered = 0;
    FilterFunctionString getter_string = nullptr;
    FilterFunctionVectorString getter_vector_string = nullptr;
    try {
        getter_string = filters_string.at(key);
    } catch (const std::out_of_range & ex) {
        getter_vector_string = filters_vector_string.at(key);
    }

    for (auto it = get_data().begin(); it != get_data().end();) {
        bool match = false;

        if (getter_string) {
            auto value = getter_string(*it);
            match = match_string(value, cmp, pattern);
        } else {
            auto values = getter_vector_string(*it);
            match = match_string(values, cmp, pattern);
        }

        if (match) {
            ++it;
        } else {
            it = get_data().erase(it);
            ++filtered;
        }
    }
    return filtered;
}


template <typename T>
inline std::size_t Query<T>::filter(Key key, QueryCmp cmp, const std::vector<std::string> & patterns) {
    std::size_t filtered = 0;
    FilterFunctionString getter_string = nullptr;
    FilterFunctionVectorString getter_vector_string = nullptr;
    try {
        getter_string = filters_string.at(key);
    } catch (const std::out_of_range & ex) {
        getter_vector_string = filters_vector_string.at(key);
    }

    for (auto it = get_data().begin(); it != get_data().end();) {
        bool match = false;

        if (getter_string) {
            auto value = getter_string(*it);
            match = match_string(value, cmp, patterns);
        } else {
            auto values = getter_vector_string(*it);
            match = match_string(values, cmp, patterns);
        }

        if (match) {
            ++it;
        } else {
            it = get_data().erase(it);
            ++filtered;
        }
    }
    return filtered;
}


template <typename T>
inline std::size_t Query<T>::filter(Key key, QueryCmp cmp, int64_t pattern) {
    std::size_t filtered = 0;
    FilterFunctionInt64 getter_int64 = nullptr;
    FilterFunctionVectorInt64 getter_vector_int64 = nullptr;
    try {
        getter_int64 = filters_int64.at(key);
    } catch (const std::out_of_range & ex) {
        getter_vector_int64 = filters_vector_int64.at(key);
    }

    for (auto it = get_data().begin(); it != get_data().end();) {
        bool match = false;

        if (getter_int64) {
            auto value = getter_int64(*it);
            match = match_int64(value, cmp, pattern);
        } else {
            auto values = getter_vector_int64(*it);
            match = match_int64(values, cmp, pattern);
        }

        if (match) {
            ++it;
        } else {
            it = get_data().erase(it);
            ++filtered;
        }
    }
    return filtered;
}


template <typename T>
inline std::size_t Query<T>::filter(Key key, QueryCmp cmp, const std::vector<int64_t> & patterns) {
    std::size_t filtered = 0;
    FilterFunctionInt64 getter_int64 = nullptr;
    FilterFunctionVectorInt64 getter_vector_int64 = nullptr;
    try {
        getter_int64 = filters_int64.at(key);
    } catch (const std::out_of_range & ex) {
        getter_vector_int64 = filters_vector_int64.at(key);
    }

    for (auto it = get_data().begin(); it != get_data().end();) {
        bool match = false;

        if (getter_int64) {
            auto value = getter_int64(*it);
            match = match_int64(value, cmp, patterns);
        } else {
            auto values = getter_vector_int64(*it);
            match = match_int64(values, cmp, patterns);
        }

        if (match) {
            ++it;
        } else {
            it = get_data().erase(it);
            ++filtered;
        }
    }
    return filtered;
}


template <typename T>
inline std::size_t Query<T>::filter(Key key, QueryCmp cmp, const char * pattern) {
    // TODO(dmach): handle ISNULL
    return filter(key, cmp, std::string(pattern));
}


}  // namespace libdnf::utils::sack


#endif
