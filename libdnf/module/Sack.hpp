#pragma once


#include <string>


namespace libdnf::module {


/// @replaces libdnf:libdnf/module/ModulePackageContainer.hpp:class:ModulePackageContainer
class Sack {
public:
    Query newQuery();

    // EXCLUDES

    /// @replaces dnf:dnf/sack.py:method:Sack.get_module_excludes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_get_module_excludes(DnfSack * sack)
    void getExcludes();

    /// @replaces dnf:dnf/sack.py:method:Sack.add_module_excludes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_add_module_excludes(DnfSack * sack, DnfPackageSet * pset)
    void addExcludes();

    /// @replaces dnf:dnf/sack.py:method:Sack.remove_module_excludes()
    void removeExcludes();

    /// @replaces dnf:dnf/sack.py:method:Sack.reset_module_excludes()
    /// @replaces dnf:dnf/sack.py:method:Sack.set_module_excludes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_set_module_excludes(DnfSack * sack, DnfPackageSet * pset)
    void setExcludes();

    // INCLUDES

    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_get_module_includes(DnfSack * sack)
    PackageSet getIncludes() const;

    void addIncludes(const PackageSet & pset);

    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_remove_module_excludes(DnfSack * sack, DnfPackageSet * pset)
    void removeIncludes(const PackageSet & pset);

    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_set_module_includes(DnfSack * sack, DnfPackageSet * pset)
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_reset_module_excludes(DnfSack * sack)
    void setIncludes(const PackageSet & pset);

    bool getUseIncludes();

    void setUseIncludes(bool value);


};


}  // namespace libdnf::module
