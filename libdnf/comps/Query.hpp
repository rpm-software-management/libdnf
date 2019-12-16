#pragma once


#include <string>
#include <vector>


namespace libdnf::comps {


/// @replaces dnf:dnf/comps.py:class:Comps
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
    // lukash: Again, Query in comps?


    // TODO: string, vector<string> ?
    /// @replaces dnf:dnf/comps.py:method:Comps.categories_by_pattern(self, pattern, case_sensitive=False)
    /// @replaces dnf:dnf/comps.py:method:Comps.category_by_pattern(self, pattern, case_sensitive=False)
    /// @replaces dnf:dnf/comps.py:method:Comps.environment_by_pattern(self, pattern, case_sensitive=False)
    /// @replaces dnf:dnf/comps.py:method:Comps.environments_by_pattern(self, pattern, case_sensitive=False)
    /// @replaces dnf:dnf/comps.py:method:Comps.group_by_pattern(self, pattern, case_sensitive=False)
    /// @replaces dnf:dnf/comps.py:method:Comps.groups_by_pattern(self, pattern, case_sensitive=False)
    int filter(int keyname, int cmp_type, const char *match);
    int filter(int keyname, int cmp_type, const char **matches);
    // FILTERS: id, name, translated_name, available=<bool>, installed=<bool>

    /// @replaces dnf:dnf/db/group.py:method:EnvironmentPersistor.get_group_environments(self, group_id)
    // FILTER: Environment.filter(group_id=...).list()

    /// @replaces dnf:dnf/db/group.py:method:GroupPersistor.get_package_groups(self, pkg_name)
    // FILTER: Group.filter(package_name=...).list()


    // to be used as the last argument to return objects of the desired type
    // another option would be to integrate Query as a class method to each type (Django works this way)
    // https://docs.djangoproject.com/en/3.0/topics/db/queries/
    // https://docs.djangoproject.com/en/3.0/ref/models/querysets/
    // https://docs.djangoproject.com/en/3.0/ref/models/expressions/
    // Consider rewriting Query into a template and bind it to the classes? Make sure it works in bindings.

    /// @replaces dnf:dnf/comps.py:attribute:Comps.categories
    /// @replaces dnf:dnf/comps.py:method:Comps.categories_iter(self)
    std::vector<Category> categories() const;

    /// @replaces dnf:dnf/comps.py:attribute:Comps.environments
    /// @replaces dnf:dnf/comps.py:method:Comps.environments_iter(self)
    std::vector<Environment> environments() const;

    /// @replaces dnf:dnf/comps.py:attribute:Comps.groups
    /// @replaces dnf:dnf/comps.py:method:Comps.groups_iter(self)
    /// @replaces dnf:dnf/comps.py:method:CompsQuery.get(self, *patterns)
    std::vector<Group> groups() const;

};


}  // namespace libdnf::comps


/*
dnf:dnf/comps.py:method:CompsQuery.get(self, *patterns)
dnf:dnf/comps.py:attribute:CompsQuery.AVAILABLE
dnf:dnf/comps.py:attribute:CompsQuery.ENVIRONMENTS
dnf:dnf/comps.py:attribute:CompsQuery.GROUPS
dnf:dnf/comps.py:attribute:CompsQuery.INSTALLED
*/
