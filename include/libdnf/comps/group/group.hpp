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


#include <map>
#include <set>
#include <string>
#include <vector>


namespace libdnf::comps {


/// @replaces dnf:dnf/comps.py:class:Group
class Group {
public:
    /// Get group id
    std::string get_id() const noexcept { return id; }

    /// Set group id
    void set_id(const std::string & value) { id = value; }

    /// Get group name
    std::string get_name() const noexcept { return name; }

    /// Set group name
    void set_name(const std::string & value) { name = value; }

    /// Get group description
    std::string get_description() const;

    /// Set group description
    void set_description(const std::string & value) { description = value; }

    /// Get translated name of a group based on current locales.
    /// If a translation is not found, return untranslated name.
    ///
    /// @replaces dnf:dnf/comps.py:attribute:Group.ui_name
    std::string get_translated_name() const noexcept { return ""; }

    void set_translated_name(const std::string & lang, const std::string & value) { translated_names.insert({lang, value}); }

    /// Get translated description of a group based on current locales.
    /// If a translation is not found, return untranslated description.
    ///
    /// @replaces dnf:dnf/comps.py:attribute:Group.ui_description
    std::string get_translated_description() const noexcept { return ""; }

    void set_translated_description(const std::string & lang, const std::string & value) { translated_descriptions.insert({lang, value}); }

    /// Determine if group is visible to the users
    ///
    /// @replaces dnf:dnf/comps.py:attribute:Group.visible
    bool get_uservisible() const noexcept { return is_uservisible; }

    /// Set the 'uservisible' flag.
    void set_uservisible(bool value) { is_uservisible = value; }

    /// Determine if group is installed by default
    bool get_default() const noexcept { return is_default; }

    /// Set the 'default' flag.
    void set_default(bool value) { is_default = value; }

    /// @replaces dnf:dnf/comps.py:method:Group.packages_iter(self)
    //std::vector<Package> get_packages() const;

    /// @replaces dnf:dnf/comps.py:attribute:Group.conditional_packages
    /// @replaces dnf:dnf/comps.py:attribute:Group.default_packages
    /// @replaces dnf:dnf/comps.py:attribute:Group.mandatory_packages
    /// @replaces dnf:dnf/comps.py:attribute:Group.optional_packages
    //std::vector<Package> get_packages(bool mandatory_groups, bool optional_groups) const;

    std::set<std::string> get_repos() { return repos; }
    void add_repo(const std::string & repoid) { repos.insert(repoid); }

    /// Determine if group is installed.
    /// If it belongs to the @System repo, return true.
    bool get_installed() const;

    /// Merge a comps Group with another one
    Group & operator+=(const Group & rhs);

private:
    std::string id;
    std::string name;
    std::string description;
    bool is_uservisible = true;
    bool is_default = false;
    std::map<std::string, std::string> translated_names;
    std::map<std::string, std::string> translated_descriptions;

    // list of repos a group comes from
    // installed groups map to [@System]
    std::set<std::string> repos;
};


}  // namespace libdnf::comps


#endif  // LIBDNF_COMPS_GROUP_GROUP_HPP
