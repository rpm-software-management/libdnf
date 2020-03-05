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


#ifndef LIBDNF_MODULE_SACK_HPP
#define LIBDNF_MODULE_SACK_HPP

#include <string>


namespace libdnf::module {


// class definitions to mute clang-tidy complaints
// to be removed / implemented
class PackageSet;
class Query;


/// @replaces libdnf:libdnf/module/ModulePackageContainer.hpp:class:ModulePackageContainer
class Sack {
public:
    Query new_query();

    // EXCLUDES

    /// @replaces dnf:dnf/sack.py:method:Sack.get_module_excludes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_get_module_excludes(DnfSack * sack)
    void get_excludes();

    /// @replaces dnf:dnf/sack.py:method:Sack.add_module_excludes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_add_module_excludes(DnfSack * sack, DnfPackageSet * pset)
    void add_excludes();

    /// @replaces dnf:dnf/sack.py:method:Sack.remove_module_excludes()
    void remove_excludes();

    /// @replaces dnf:dnf/sack.py:method:Sack.reset_module_excludes()
    /// @replaces dnf:dnf/sack.py:method:Sack.set_module_excludes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_set_module_excludes(DnfSack * sack, DnfPackageSet * pset)
    void set_excludes();

    // INCLUDES

    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_get_module_includes(DnfSack * sack)
    PackageSet get_includes() const;

    void add_includes(const PackageSet & pset);

    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_remove_module_excludes(DnfSack * sack, DnfPackageSet * pset)
    void remove_includes(const PackageSet & pset);

    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_set_module_includes(DnfSack * sack, DnfPackageSet * pset)
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_reset_module_excludes(DnfSack * sack)
    void set_includes(const PackageSet & pset);

    bool get_use_includes();

    void set_use_includes(bool value);
};


}  // namespace libdnf::module

#endif
