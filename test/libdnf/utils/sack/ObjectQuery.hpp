#pragma once


#include "libdnf/utils/sack/Query.hpp"
#include "Object.hpp"


using ObjectQuery = libdnf::utils::sack::Query<Object>;


template <>
enum class ObjectQuery::Key {
    string,
    cstring,
    boolean,
    int32,
    int64,
};


template <>
void libdnf::utils::sack::Query<Object>::initialize_filters() {
    add_filter(Key::string, [](Object * obj) { return obj->string; });
    add_filter(Key::cstring, [](Object * obj) { return obj->cstring; });
    add_filter(Key::boolean, [](Object * obj) { return obj->boolean; });
    add_filter(Key::int32, [](Object * obj) { return obj->int32; });
    add_filter(Key::int64, [](Object * obj) { return obj->int64; });
}
