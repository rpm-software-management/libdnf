#pragma once


#include <string>


namespace libdnf::comps {


/// @replaces dnf:dnf/comps.py:class:Package
/// @replaces dnf:dnf/comps.py:class:CompsTransPkg
class Package {
    /// @replaces dnf:dnf/comps.py:attribute:Package.name
    std::string getName();

    //std::string getDescription();

    /// @replaces dnf:dnf/comps.py:attribute:Package.ui_name
    std::string getTranslatedName();

    /// @replaces dnf:dnf/comps.py:attribute:Package.ui_description
    std::string getTranslatedDescription();
};


}  // namespace libdnf::comps


/*
dnf:dnf/comps.py:attribute:Package.option_type
*/
