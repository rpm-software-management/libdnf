#pragma once


#include "libdnf/utils/sack/Query.hpp"
#include "Object.hpp"


using ObjectQuery = libdnf::utils::sack::Query<Object>;


template <>
void libdnf::utils::sack::Query<Object>::initialize_filters() {
    add_filter("string", [](Object * obj) { return obj->string; });
    add_filter("cstring", [](Object * obj) { return obj->cstring; });
    add_filter("boolean", [](Object * obj) { return obj->boolean; });
    add_filter("int32", [](Object * obj) { return obj->int32; });
    add_filter("int64", [](Object * obj) { return obj->int64; });
}
