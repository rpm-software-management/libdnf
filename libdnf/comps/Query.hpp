#pragma once


#include <string>


namespace libdnf::comps {


/// @replaces dnf:dnf/comps.py:class:CompsQuery
/// @replaces dnf:dnf/db/group.py:method:EnvironmentPersistor.search_by_pattern(self, pattern)
/// @replaces dnf:dnf/db/group.py:method:GroupPersistor.search_by_pattern(self, pattern)
/// @replaces dnf:dnf/db/group.py:method:PersistorBase.search_by_pattern(self, pattern)
/// @replaces dnf:dnf/db/group.py:method:EnvironmentPersistor.get(self, obj_id)
/// @replaces dnf:dnf/db/group.py:method:GroupPersistor.get(self, obj_id)
/// @replaces dnf:dnf/db/group.py:method:PersistorBase.get(self, obj_id)
/// @replaces dnf:dnf/comps.py:attribute:CompsQuery.AVAILABLE
/// @replaces dnf:dnf/comps.py:attribute:CompsQuery.ENVIRONMENTS
/// @replaces dnf:dnf/comps.py:attribute:CompsQuery.GROUPS
/// @replaces dnf:dnf/comps.py:attribute:CompsQuery.INSTALLED
class Query {
};


}  // namespace libdnf::comps


/*
dnf:dnf/comps.py:method:CompsQuery.get(self, *patterns)
dnf:dnf/comps.py:attribute:CompsQuery.AVAILABLE
dnf:dnf/comps.py:attribute:CompsQuery.ENVIRONMENTS
dnf:dnf/comps.py:attribute:CompsQuery.GROUPS
dnf:dnf/comps.py:attribute:CompsQuery.INSTALLED
*/
