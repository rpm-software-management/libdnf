#pragma once


#include <string>


namespace libdnf::comps {


/// @replaces dnf:dnf/comps.py:class:Package
/// @replaces dnf:dnf/comps.py:class:CompsTransPkg
class Package {
    /// @replaces dnf:dnf/comps.py:attribute:Package.name
    std::string get_name();

    //std::string getDescription();

    /// @replaces dnf:dnf/comps.py:attribute:Package.ui_name
    std::string get_translated_name();

    /// @replaces dnf:dnf/comps.py:attribute:Package.ui_description
    std::string get_translated_description();
};


}  // namespace libdnf::comps


/*
dnf:dnf/comps.py:attribute:Package.option_type
*/
