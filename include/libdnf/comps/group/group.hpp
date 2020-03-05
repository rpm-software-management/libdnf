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


#ifndef LIBDNF_COMPS_GROUP_GROUP_HPP
#define LIBDNF_COMPS_GROUP_GROUP_HPP


#include "package.hpp"

#include <string>
#include <vector>


namespace libdnf::comps {


/// @replaces dnf:dnf/comps.py:class:Group
class Group {
public:
    std::string get_id() const;

    std::string get_name() const;

    std::string get_description() const;

    /// @replaces dnf:dnf/comps.py:attribute:Group.ui_name
    std::string get_translated_name() const;

    /// @replaces dnf:dnf/comps.py:attribute:Group.ui_description
    std::string get_translated_description() const;

    /// @replaces dnf:dnf/comps.py:attribute:Group.visible
    bool is_visible() const;

    /// @replaces dnf:dnf/comps.py:method:Group.packages_iter(self)
    std::vector<Package> get_packages() const;

    /// @replaces dnf:dnf/comps.py:attribute:Group.conditional_packages
    /// @replaces dnf:dnf/comps.py:attribute:Group.default_packages
    /// @replaces dnf:dnf/comps.py:attribute:Group.mandatory_packages
    /// @replaces dnf:dnf/comps.py:attribute:Group.optional_packages
    std::vector<Package> get_packages(bool mandatory_groups, bool optional_groups) const;

    // lukash: reference to Base is missing here, are we sure it won't be necessary for the methods?
};


}  // namespace libdnf::comps


/*
dnf:dnf/db/group.py:class:GroupPersistor
dnf:dnf/db/group.py:method:GroupPersistor.get(self, obj_id)
dnf:dnf/db/group.py:method:GroupPersistor.get_package_groups(self, pkg_name)
dnf:dnf/db/group.py:method:GroupPersistor.is_removable_pkg(self, pkg_name)
dnf:dnf/db/group.py:method:GroupPersistor.new(self, obj_id, name, translated_name, pkg_types)
dnf:dnf/db/group.py:method:GroupPersistor.search_by_pattern(self, pattern)

dnf:dnf/db/group.py:method:EnvironmentPersistor.get(self, obj_id)
dnf:dnf/db/group.py:method:EnvironmentPersistor.get_group_environments(self, group_id)
dnf:dnf/db/group.py:method:EnvironmentPersistor.is_removable_group(self, group_id)
dnf:dnf/db/group.py:method:EnvironmentPersistor.new(self, obj_id, name, translated_name, pkg_types)
dnf:dnf/db/group.py:method:EnvironmentPersistor.search_by_pattern(self, pattern)
*/


#endif
