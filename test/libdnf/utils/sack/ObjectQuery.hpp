#pragma once


#include "Object.hpp"
#include "RelatedObjectQuery.hpp"
#include "libdnf/utils/WeakPtr.hpp"
#include "libdnf/utils/sack/Query.hpp"

using ObjectWeakPtr = libdnf::utils::WeakPtr<Object, false>;

class ObjectQuery : public libdnf::utils::sack::Query<ObjectWeakPtr> {
public:

    static std::string get_string(const ObjectWeakPtr & obj) { return obj->string; }
    static std::string get_cstring(const ObjectWeakPtr & obj) { return std::string(obj->cstring); }
    static bool get_boolean(const ObjectWeakPtr & obj) { return obj->boolean; }
    static int64_t get_int32(const ObjectWeakPtr & obj) { return static_cast<int64_t>(obj->int32); }
    static int64_t get_int64(const ObjectWeakPtr & obj) { return obj->int64; }
    static std::vector<std::string> get_related_object(const ObjectWeakPtr & obj) { return obj->related_objects; }

    using libdnf::utils::sack::Query<ObjectWeakPtr>::filter;

    std::size_t filter(FilterFunctionVectorString * getter, libdnf::utils::sack::QueryCmp cmp, RelatedObjectQuery q) {
        if (getter != get_related_object) {
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
        return filter(getter, cmp, patterns);
    }
};
