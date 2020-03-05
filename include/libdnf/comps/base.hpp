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


#ifndef LIBDNF_COMPS_BASE_HPP
#define LIBDNF_COMPS_BASE_HPP

// forward declarations
namespace libdnf::comps {
class Base;
}  // namespace libdnf::comps


#include "goal.hpp"

#include "libdnf/base/base.hpp"
#include "libdnf/comps/environment/environment.hpp"
#include "libdnf/comps/group/group.hpp"
#include "libdnf/comps/group/package.hpp"


namespace libdnf::comps {


/// class Base
///
class Base {
public:
    /// @replaces dnf:dnf/db/group.py:method:GroupPersistor.install(self, obj)
    void install(Group group);

    /// @replaces dnf:dnf/db/group.py:method:GroupPersistor.remove(self, obj)
    void remove(Group group);

    void reinstall(Group group);

    /// @replaces dnf:dnf/db/group.py:method:GroupPersistor.upgrade(self, obj)
    void upgrade(Group group);

    /* ENVIRONMENTS */

    /// @replaces dnf:dnf/db/group.py:method:EnvironmentPersistor.install(self, obj)
    void install(Environment env);

    // lukash: I don't like overloading the methods here. woudn't it be better to have comps::Group and comps::Environment as separate classes with their respective methods?

    /// @replaces dnf:dnf/db/group.py:method:EnvironmentPersistor.remove(self, obj)
    void remove(Environment env);

    void reinstall(Environment env);

    /// @replaces dnf:dnf/db/group.py:method:EnvironmentPersistor.upgrade(self, obj)
    void upgrade(Environment env);

    /* COMMON */

    /// @replaces dnf:dnf/db/group.py:method:GroupPersistor.clean(self)
    /// @replaces dnf:dnf/db/group.py:method:EnvironmentPersistor.clean(self)
    void reset();


    //protected:
    //    friend libdnf::Base;
    explicit Base(libdnf::Base & dnf_base);


private:
    const libdnf::Base & dnf_base;
};


/*
Base::Base(libdnf::Base & dnf_base)
    : dnf_base{dnf_base}
{
}
*/

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
