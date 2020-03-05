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


#ifndef LIBDNF_COMPS_GROUP_PACKAGE_HPP
#define LIBDNF_COMPS_GROUP_PACKAGE_HPP

#include <string>


namespace libdnf::comps {


// TODO(dmach): isn't it more a package dependency rather than a package?


/// @replaces dnf:dnf/comps.py:class:Package
/// @replaces dnf:dnf/comps.py:class:CompsTransPkg
class Package {
public:
    // lukash: Why is there Package in comps?
    /// @replaces dnf:dnf/comps.py:attribute:Package.name
    std::string get_name() const;

    //std::string getDescription();

    /// @replaces dnf:dnf/comps.py:attribute:Package.ui_name
    std::string get_translated_name() const;

    /// @replaces dnf:dnf/comps.py:attribute:Package.ui_description
    std::string get_translated_description() const;
};


}  // namespace libdnf::comps


/*
dnf:dnf/comps.py:attribute:Package.option_type
*/

#endif
