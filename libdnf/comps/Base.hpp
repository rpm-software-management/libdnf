#pragma once


// forward declarations
namespace libdnf::comps {
class Base;
}  // namespace libdnf::comps


#include "../base/base.hpp"

#include "Environment.hpp"
#include "Goal.hpp"
#include "Group.hpp"
#include "Package.hpp"
#include "Query.hpp"


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
    Base(libdnf::Base & dnf_base);


protected:
    const libdnf::Base & dnf_base;
};


Base::Base(libdnf::Base & dnf_base)
    : dnf_base{dnf_base}
{
}


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
