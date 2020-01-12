#ifndef LIBDNF_MODULE_SACK_HPP
#define LIBDNF_MODULE_SACK_HPP

#include <string>


namespace libdnf::module {


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
