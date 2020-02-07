#pragma once


#include "libdnf/utils/sack/Query.hpp"
#include "RelatedObject.hpp"
#include "libdnf/utils/WeakPtr.hpp"

using RelatedObjectWeakPtr = libdnf::utils::WeakPtr<RelatedObject, false>;

template <>
enum class libdnf::utils::sack::Query<RelatedObjectWeakPtr>::Key {
    id,
    value,
};


class RelatedObjectQuery : public libdnf::utils::sack::Query<RelatedObjectWeakPtr> {
public:
    explicit RelatedObjectQuery() {
        register_filter_string(Key::id, [](const RelatedObjectWeakPtr & obj) { return obj->id; });
        register_filter_int64(Key::value, [](const RelatedObjectWeakPtr & obj) { return obj->value; });
    }
};
