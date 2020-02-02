#pragma once


#include "libdnf/utils/sack/Query.hpp"
#include "libdnf/utils/sack/QueryCmp.hpp"
#include "Object.hpp"


template <>
enum class libdnf::utils::sack::Query<Object>::Key {
    string,
    cstring,
    boolean,
    int32,
    int64,
    related_object,
};


class ObjectQuery : public libdnf::utils::sack::Query<Object> {
public:
    ObjectQuery() {
        register_filter_string(Key::string, [](Object * obj) { return obj->string; });
        register_filter_string(Key::cstring, [](Object * obj) { return obj->cstring; });
        register_filter_bool(Key::boolean, [](Object * obj) { return obj->boolean; });
        register_filter_int64(Key::int32, [](Object * obj) { return obj->int32; });
        register_filter_int64(Key::int64, [](Object * obj) { return obj->int64; });
        register_filter_vector_string(Key::related_object, [](Object * obj) { return obj->related_objects; });
    }

    using libdnf::utils::sack::Query<Object>::filter;

    std::size_t filter(Key key, libdnf::utils::sack::QueryCmp cmp, RelatedObjectQuery q) {
        if (key != Key::related_object) {
            throw std::runtime_error("Invalid key");
        }

        if (cmp != libdnf::utils::sack::QueryCmp::EXACT) {
            throw std::runtime_error("Invalid cmp operator");
        }

        // extract keys from the objects in the query
        std::vector<std::string> patterns;
        for (auto & i : q.get_data()) {
            patterns.push_back(i->id);
        }

        // compare the extracted keys with the related_objects content
        return filter(key, cmp, patterns);
    }
};
