#pragma once


// foward declarations
namespace libdnf::rpm {
class Sack;
}  // namespace libdnf::rpm


#include <string>

#include "Base.hpp"
#include "Package.hpp"
#include "PackageSet.hpp"
#include "Query.hpp"


namespace libdnf::rpm {

/// @replaces dnf:dnf/sack.py:class:Sack
class Sack {
public:
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_new()
    Sack(Base & rpmBase);
    // lukash: I'm wondering whether we want the constructor to be private and have a friend Base class and a Base::get_sack()? Or a public Sack constructor taking Base? That might be a bit better if we don't run into a unique_ptr<Base> vs Base& mess?

    /// @replaces dnf:dnf/sack.py:method:Sack.query(self, flags=0)
    Query new_query();

    // EXCLUDES

    /// @replaces dnf:dnf/sack.py:method:Sack.get_excludes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_get_excludes(DnfSack * sack)
    PackageSet get_excludes();

    /// @replaces dnf:dnf/sack.py:method:Sack.add_excludes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_add_excludes(DnfSack * sack, DnfPackageSet * pset)
    void add_excludes(const PackageSet & pset);

    /// @replaces dnf:dnf/sack.py:method:Sack.remove_excludes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_remove_excludes(DnfSack * sack, DnfPackageSet * pset)
    void remove_excludes(const PackageSet & pset);

    /// @replaces dnf:dnf/sack.py:method:Sack.reset_excludes()
    /// @replaces dnf:dnf/sack.py:method:Sack.set_excludes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_reset_excludes(DnfSack * sack)
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_set_excludes(DnfSack * sack, DnfPackageSet * pset)
    void set_excludes(const PackageSet & pset);

    // INCLUDES

    /// @replaces dnf:dnf/sack.py:method:Sack.get_includes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_get_includes(DnfSack * sack)
    PackageSet get_includes() const;

    /// @replaces dnf:dnf/sack.py:method:Sack.add_includes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_add_includes(DnfSack * sack, DnfPackageSet * pset)
    void add_includes(const PackageSet & pset);

    /// @replaces dnf:dnf/sack.py:method:Sack.remove_includes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_remove_includes(DnfSack * sack, DnfPackageSet * pset)
    void remove_includes(const PackageSet & pset);

    /// @replaces dnf:dnf/sack.py:method:Sack.reset_includes()
    /// @replaces dnf:dnf/sack.py:method:Sack.set_includes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_reset_includes(DnfSack * sack)
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_set_includes(DnfSack * sack, DnfPackageSet * pset)
    void set_includes(const PackageSet & pset);

    /// @replaces dnf:dnf/sack.py:method:Sack.get_use_includes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_get_use_includes(DnfSack * sack, const char * reponame, gboolean * enabled)
    bool get_use_includes();

    /// @replaces dnf:dnf/sack.py:method:Sack.set_use_includes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_set_use_includes(DnfSack * sack, const char * reponame, gboolean enabled)
    void set_use_includes(bool value);

protected:
    const Base & rpm_base;
};


Sack::Sack(Base & rpm_base)
    : rpm_base{rpm_base}
{
}

}  // namespace libdnf::rpm
