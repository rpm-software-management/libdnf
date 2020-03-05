/*
Copyright (C) 2020 Red Hat, Inc.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/


#ifndef LIBDNF_COMPS_CATEGORY_QUERY_HPP
#define LIBDNF_COMPS_CATEGORY_QUERY_HPP


#include "category.hpp"

#include "libdnf/utils/sack/query.hpp"


template <>
enum class libdnf::utils::sack::Query<libdnf::comps::Category>::Key {
    id,
    name,
    description,
    translated_name,
    translated_description,
};


namespace libdnf::comps {


class CategoryQuery : public libdnf::utils::sack::Query<Category> {
    CategoryQuery();
};


inline CategoryQuery::CategoryQuery() {
    register_filter_string(Key::id, [](Category * obj) { return obj->get_id(); });
    register_filter_string(Key::name, [](Category * obj) { return obj->get_name(); });
    register_filter_string(Key::description, [](Category * obj) { return obj->get_description(); });
    register_filter_string(Key::translated_name, [](Category * obj) { return obj->get_translated_name(); });
    register_filter_string(
        Key::translated_description, [](Category * obj) { return obj->get_translated_description(); });
}


}  // namespace libdnf::comps


#endif
