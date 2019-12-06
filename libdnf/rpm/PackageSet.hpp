#pragma once


namespace libdnf::rpm {
class PackageSet;
}


#include <string>

#include <solv/bitmap.h>
#include <solv/pooltypes.h>

#include "Sack.hpp"


namespace libdnf::rpm {


/// @replaces libdnf:libdnf/sack/packageset.hpp:class:PackageSet
class PackageSet {
public:
    /// @replaces libdnf:libdnf/hy-packageset.h:function:dnf_packageset_new(DnfSack * sack)
    PackageSet(const Sack & sack);

    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.getSack()
    Sack & get_stack() const;

    /// @replaces libdnf:libdnf/hy-packageset.h:function:dnf_packageset_add(DnfPackageSet * pset, DnfPackage * pkg)
    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.set(DnfPackage * pkg)
    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.set(Id id)
    void add(const Package & pkg);

    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.remove(Id id)
    void remove(const Package & pkg);

    /// Merge given PackageSet into the instance
    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.operator+=(const libdnf::PackageSet & other)
    void update(const PackageSet & other);

    /// @replaces libdnf:libdnf/hy-packageset.h:function:dnf_packageset_has(DnfPackageSet * pset, DnfPackage * pkg)
    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.has(DnfPackage * pkg)
    bool contains(const Package & pkg) const;

    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.empty()
    bool empty() const;

    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.clear()
    void clear();

    /// @replaces libdnf:libdnf/hy-packageset.h:function:dnf_packageset_count(DnfPackageSet * pset)
    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.size()
    std::size_t size() const;

    // SET OPERATIONS

    /// subset: this <= other (are all elements from 'this' part of 'other'?)
    bool operator<=(const PackageSet & other) const;

    /// superset: this >= other (are all elements from 'other' part of 'this'?)
    bool operator>=(const PackageSet & other) const;

    /// union: this | other (elements from both 'this' and 'other')
    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.operator+=(const libdnf::PackageSet & other)
    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.operator+=(const Map * other)
    PackageSet operator|(const PackageSet & other) const;
    void operator|=(const PackageSet & other);

    /// intersection: this & other (elements that are in both 'this' and 'other')
    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.operator/=(const libdnf::PackageSet & other)
    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.operator/=(const Map * other)
    PackageSet operator&(const PackageSet & other) const;
    void operator&=(const PackageSet & other);

    /// difference: this - other (elements that are in 'this' but not 'other')
    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.operator-=(const libdnf::PackageSet & other)
    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.operator-=(const Map * other)
    PackageSet operator-(const PackageSet & other) const;
    void operator-=(const PackageSet & other);

    /// symmetrical difference: this ^ other (elements that are in either 'this' or 'other', but not both)
    PackageSet operator^(const PackageSet & other) const;
    void operator^(const PackageSet & other);

private:
    /// @replaces libdnf:libdnf/hy-packageset.h:function:dnf_packageset_from_bitmap(DnfSack * sack, Map * m)
    PackageSet(const Sack & sack, const Map * m);

    const Sack & sack;

    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.has(Id id)
    bool contains(Id id) const;

    /// Return a copy of bitmap representing the package set
    /// @replaces libdnf:libdnf/hy-packageset.h:function:dnf_packageset_get_map(DnfPackageSet * pset)
    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.getMap()
    Map * get_map() const;
};


PackageSet::PackageSet(const Sack & sack)
    : sack{sack}
{
}


}  // namespace libdnf::rpm
