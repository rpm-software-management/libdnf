#pragma once


#include "Object.hpp"
#include "RelatedObjectQuery.hpp"
#include "libdnf/utils/WeakPtr.hpp"
#include "libdnf/utils/sack/Query.hpp"

using ObjectWeakPtr = libdnf::utils::WeakPtr<Object, false>;

template <>
enum class libdnf::utils::sack::Query<ObjectWeakPtr>::Key {
    string,
    cstring,
    boolean,
    int32,
    int64,
    related_object,
};


class ObjectQuery : public libdnf::utils::sack::Query<ObjectWeakPtr> {
public:
    ObjectQuery() {
        register_filter_string(Key::string, [](const ObjectWeakPtr & obj) { return obj->string; });
        register_filter_string(Key::cstring, [](const ObjectWeakPtr & obj) { return obj->cstring; });
        register_filter_bool(Key::boolean, [](const ObjectWeakPtr & obj) { return obj->boolean; });
        register_filter_int64(Key::int32, [](const ObjectWeakPtr & obj) { return obj->int32; });
        register_filter_int64(Key::int64, [](const ObjectWeakPtr & obj) { return obj->int64; });
        register_filter_vector_string(Key::related_object, [](const ObjectWeakPtr & obj) { return obj->related_objects; });
    }

    using libdnf::utils::sack::Query<ObjectWeakPtr>::filter;

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
        return this->filter(key, cmp, patterns);
    }
};
