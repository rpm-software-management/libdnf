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


#ifndef LIBDNF_RPM_PACKAGE_SET_HPP
#define LIBDNF_RPM_PACKAGE_SET_HPP


// forward declarations
namespace libdnf::rpm {
class PackageSet;
}  // namespace libdnf::rpm


#include "sack.hpp"

#include <solv/bitmap.h>
#include <solv/pooltypes.h>

#include <string>


namespace libdnf::rpm {

/// @replaces libdnf:libdnf/sack/packageset.hpp:class:PackageSet
class PackageSet {
public:
    /// @replaces libdnf:libdnf/hy-packageset.h:function:dnf_packageset_new(DnfSack * sack)
    explicit PackageSet(const Sack & sack);

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
    // lukash: so is this a union (operator| ?)

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
    // lukash: btw. I'm not entirely sure we want to get as fancy as using these operators. Maybe methods like "union", "intersection" and "difference" are really better?
    // lukash: Personally I think they're kind of neat and I'd probably like them. Just saying it may be less readable to a newcomer.

protected:
    /// @replaces libdnf:libdnf/hy-packageset.h:function:dnf_packageset_from_bitmap(DnfSack * sack, Map * m)
    explicit PackageSet(const Sack & sack, const Map * m);

    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.has(Id id)
    bool contains(Id id) const;
    // I strongly believe we don't want an Id on the interface :)
    // The method is in private, if it's not important for the interface, we can just remove it for now? Same for the other private methods and constructors, btw., unless we're going to do friend classes (which I'm afraid we're going to need?) and then we should include that in the design.

    /// Return a copy of bitmap representing the package set
    /// @replaces libdnf:libdnf/hy-packageset.h:function:dnf_packageset_get_map(DnfPackageSet * pset)
    /// @replaces libdnf:libdnf/sack/packageset.hpp:method:PackageSet.getMap()
    Map * get_map() const;

private:
    const Sack & sack;
};


/*
PackageSet::PackageSet(const Sack & sack)
    : sack{sack}
{
}
*/


}  // namespace libdnf::rpm

#endif
