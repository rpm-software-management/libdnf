#pragma once


#include "libdnf/utils/sack/Query.hpp"
#include "Object.hpp"


template <>
enum class libdnf::utils::sack::Query<Object>::Key {
    string,
    cstring,
    boolean,
    int32,
    int64,
};


class ObjectQuery : public libdnf::utils::sack::Query<Object> {
public:
    ObjectQuery() {
        register_filter_string(Key::string, [](Object * obj) { return obj->string; });
        register_filter_string(Key::cstring, [](Object * obj) { return obj->cstring; });
        register_filter_bool(Key::boolean, [](Object * obj) { return obj->boolean; });
        register_filter_int64(Key::int32, [](Object * obj) { return obj->int32; });
        register_filter_int64(Key::int64, [](Object * obj) { return obj->int64; });
    }
};


/*
template <>
void libdnf::utils::sack::Query<Object>::initialize_filters() {
void libdnf::utils::sack::Query<Object>::initialize_filters() {
    add_filter(Key::string, [](Object * obj) { return obj->string; });
    add_filter(Key::cstring, [](Object * obj) { return obj->cstring; });
    add_filter(Key::boolean, [](Object * obj) { return obj->boolean; });
    add_filter(Key::int32, [](Object * obj) { return obj->int32; });
    add_filter(Key::int64, [](Object * obj) { return obj->int64; });
}
*/
