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


#ifndef LIBDNF_COMPS_GROUP_QUERY_HPP
#define LIBDNF_COMPS_GROUP_QUERY_HPP

#include "group.hpp"

#include "libdnf/utils/sack/query.hpp"

#include <string>
#include <vector>


template <>
enum class libdnf::utils::sack::Query<libdnf::comps::Group>::Key {
    id,
    name,
    description,
    translated_name,
    translated_description,
};


namespace libdnf::comps {


/// @replaces dnf:dnf/comps.py:class:Comps
/// @replaces dnf:dnf/comps.py:class:CompsQuery
/// @replaces dnf:dnf/db/group.py:method:GroupPersistor.search_by_pattern(self, pattern)
/// @replaces dnf:dnf/db/group.py:method:GroupPersistor.get(self, obj_id)
/// @replaces dnf:dnf/comps.py:attribute:CompsQuery.AVAILABLE
/// @replaces dnf:dnf/comps.py:attribute:CompsQuery.GROUPS
/// @replaces dnf:dnf/comps.py:attribute:CompsQuery.INSTALLED
/// @replaces dnf:dnf/comps.py:method:Comps.group_by_pattern(self, pattern, case_sensitive=False)
/// @replaces dnf:dnf/comps.py:method:Comps.groups_by_pattern(self, pattern, case_sensitive=False)
/// @replaces dnf:dnf/comps.py:attribute:Comps.groups
/// @replaces dnf:dnf/comps.py:method:Comps.groups_iter(self)
/// @replaces dnf:dnf/comps.py:method:CompsQuery.get(self, *patterns)
class GroupQuery : public libdnf::utils::sack::Query<Group> {
    GroupQuery();
};


inline GroupQuery::GroupQuery() {
    register_filter_string(Key::id, [](Group * obj) { return obj->get_id(); });
    register_filter_string(Key::name, [](Group * obj) { return obj->get_name(); });
    register_filter_string(Key::description, [](Group * obj) { return obj->get_description(); });
    register_filter_string(Key::translated_name, [](Group * obj) { return obj->get_translated_name(); });
    register_filter_string(Key::translated_description, [](Group * obj) { return obj->get_translated_description(); });
}


// TODO(dmach): filter groups by included packages
/// @replaces dnf:dnf/db/group.py:method:GroupPersistor.get_package_groups(self, pkg_name)
// FILTER: Group.filter(package_name=...).list()


}  // namespace libdnf::comps


#endif
