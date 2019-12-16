#pragma once


#include <string>


namespace libdnf::comps {


// TODO: isn't it more a package dependency rather than a package?


/// @replaces dnf:dnf/comps.py:class:Package
/// @replaces dnf:dnf/comps.py:class:CompsTransPkg
class Package {
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
