#pragma once


#include "libdnf/utils/sack/Query.hpp"
#include "RelatedObject.hpp"


template <>
enum class libdnf::utils::sack::Query<RelatedObject>::Key {
    id,
    value,
};


class RelatedObjectQuery : public libdnf::utils::sack::Query<RelatedObject> {
public:
    explicit RelatedObjectQuery() {
        register_filter_string(Key::id, [](RelatedObject * obj) { return obj->id; });
        register_filter_int64(Key::value, [](RelatedObject * obj) { return obj->value; });
    }
};
