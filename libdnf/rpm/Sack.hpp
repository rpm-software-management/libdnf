#pragma once


namespace libdnf::rpm {
class Sack;
}


#include <string>

#include "PackageSet.hpp"
#include "Query.hpp"


namespace libdnf::rpm {


/// @replaces dnf:dnf/sack.py:class:Sack
class Sack {
public:
    /// @replaces dnf:dnf/sack.py:method:Sack.query(self, flags=0)
    Query newQuery();

    // EXCLUDES

    /// @replaces dnf:dnf/sack.py:method:Sack.get_excludes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_get_excludes(DnfSack * sack)
    PackageSet getExcludes();

    /// @replaces dnf:dnf/sack.py:method:Sack.add_excludes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_add_excludes(DnfSack * sack, DnfPackageSet * pset)
    void addExcludes(const PackageSet & pset);

    /// @replaces dnf:dnf/sack.py:method:Sack.remove_excludes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_remove_excludes(DnfSack * sack, DnfPackageSet * pset)
    void removeExcludes(const PackageSet & pset);

    /// @replaces dnf:dnf/sack.py:method:Sack.reset_excludes()
    /// @replaces dnf:dnf/sack.py:method:Sack.set_excludes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_reset_excludes(DnfSack * sack)
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_set_excludes(DnfSack * sack, DnfPackageSet * pset)
    void setExcludes(const PackageSet & pset);

    // INCLUDES

    /// @replaces dnf:dnf/sack.py:method:Sack.get_includes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_get_includes(DnfSack * sack)
    PackageSet getIncludes() const;

    /// @replaces dnf:dnf/sack.py:method:Sack.add_includes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_add_includes(DnfSack * sack, DnfPackageSet * pset)
    void addIncludes(const PackageSet & pset);

    /// @replaces dnf:dnf/sack.py:method:Sack.remove_includes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_remove_includes(DnfSack * sack, DnfPackageSet * pset)
    void removeIncludes(const PackageSet & pset);

    /// @replaces dnf:dnf/sack.py:method:Sack.reset_includes()
    /// @replaces dnf:dnf/sack.py:method:Sack.set_includes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_reset_includes(DnfSack * sack)
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_set_includes(DnfSack * sack, DnfPackageSet * pset)
    void setIncludes(const PackageSet & pset);

    /// @replaces dnf:dnf/sack.py:method:Sack.get_use_includes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_get_use_includes(DnfSack * sack, const char * reponame, gboolean * enabled)
    bool getUseIncludes();

    /// @replaces dnf:dnf/sack.py:method:Sack.set_use_includes()
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_set_use_includes(DnfSack * sack, const char * reponame, gboolean enabled)
    void setUseIncludes(bool value);

private:
    /// @replaces libdnf:libdnf/dnf-sack.h:function:dnf_sack_new()
    Sack(Base & rpmBase);
    const Base & rpmBase;
};


Sack::Sack(Base & rpmBase)
    : rpmBase{rpmBase}
{
}

}  // namespace libdnf::rpm
