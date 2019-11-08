#pragma once


#include <vector>
#include <string>

#include "Package.hpp"


namespace libdnf::comps {


/// class Group
///
/// @replaces dnf:dnf/comps.py:class:Group
class Group {
    std::string getName();
    std::string getDescription();

    /// @replaces dnf:dnf/comps.py:attribute:Group.ui_name
    std::string getTranslatedName();

    /// @replaces dnf:dnf/comps.py:attribute:Group.ui_description
    std::string getTranslatedDescription();

    /// @replaces dnf:dnf/comps.py:attribute:Group.visible
    bool isVisible();

    /// @replaces dnf:dnf/comps.py:method:Group.packages_iter(self)
    std::vector<Package> getPackages();

    /// @replaces dnf:dnf/comps.py:attribute:Group.conditional_packages
    /// @replaces dnf:dnf/comps.py:attribute:Group.default_packages
    /// @replaces dnf:dnf/comps.py:attribute:Group.mandatory_packages
    /// @replaces dnf:dnf/comps.py:attribute:Group.optional_packages
    std::vector<Package> getPackages(bool mandatoryGroups, bool optionalGroups);
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
