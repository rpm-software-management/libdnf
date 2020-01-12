#pragma once


#include "Environment.hpp"
#include "Group.hpp"


namespace libdnf::comps {


/// class Goal
///
/// @replaces dnf:dnf/comps.py:class:Solver
/// @replaces dnf:dnf/comps.py:class:TransactionBunch
/// @replaces dnf:dnf/db/group.py:class:GroupPersistor
/// @replaces dnf:dnf/db/group.py:class:EnvironmentPersistor
class Goal {
    // GROUPS

    /// @replaces dnf:dnf/comps.py:attribute:TransactionBunch.install
    /// @replaces dnf:dnf/db/group.py:method:GroupPersistor.install(self, obj)
    void install(Group group);
    // lukash: Ok, so why is there comps::Base::install etc. and the same methods here in Goal? They take the same argument too.

    /// @replaces dnf:dnf/db/group.py:method:GroupPersistor.remove(self, obj)
    void remove(Group group);

    void reinstall(Group group);

    /// @replaces dnf:dnf/db/group.py:method:GroupPersistor.upgrade(self, obj)
    void upgrade(Group group);

    // ENVIRONMENTS

    /// @replaces dnf:dnf/db/group.py:method:EnvironmentPersistor.install(self, obj)
    void install(Environment env);

    /// @replaces dnf:dnf/db/group.py:method:EnvironmentPersistor.remove(self, obj)
    void remove(Environment env);

    void reinstall(Environment env);

    /// @replaces dnf:dnf/db/group.py:method:EnvironmentPersistor.upgrade(self, obj)
    void upgrade(Environment env);

    // COMMON

    void resolve();

    /// @replaces dnf:dnf/db/group.py:method:GroupPersistor.clean(self)
    /// @replaces dnf:dnf/db/group.py:method:EnvironmentPersistor.clean(self)
    /// @replaces dnf:dnf/db/group.py:method:PersistorBase.clean(self)
    void reset();

    // RESULTS

    // TODO: consider listResult([TransactionItemAction])
    // TODO: consider resolve() -> Transaction object

    /// @replaces dnf:dnf/comps.py:attribute:TransactionBunch.install
    /// @replaces dnf:dnf/comps.py:attribute:TransactionBunch.install_opt
    /// @replaces dnf:dnf/db/group.py:attribute:RPMTransaction.install_set
    void list_installs();

    /// @replaces dnf:dnf/comps.py:attribute:TransactionBunch.remove
    /// @replaces dnf:dnf/db/group.py:attribute:RPMTransaction.remove_set
    void list_removals();

    /// @replaces dnf:dnf/comps.py:attribute:TransactionBunch.upgrade
    void list_upgrades();
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
