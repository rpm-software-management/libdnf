/*
Copyright (C) 2018-2020 Red Hat, Inc.

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


#include "package.hpp"
#include "package_sack.hpp"
#include "package_set_iterator.hpp"

#include <memory>


namespace libdnf::solv {

class SolvMap;

}  // namespace libdnf::rpm::solv

namespace libdnf::rpm {

class PackageSetIterator;
class PackageQuery;
class Transaction;


/// @replaces libdnf:sack/packageset.hpp:struct:PackageSet
class PackageSet {
public:
    struct UsedDifferentSack : public RuntimeError {
        using RuntimeError::RuntimeError;
        const char * get_domain_name() const noexcept override { return "libdnf::rpm::PackageSet"; }
        const char * get_name() const noexcept override { return "UsedDifferentSack"; }
        const char * get_description() const noexcept override { return "PackageSet exception"; }
    };

    /// @replaces libdnf:hy-packageset.h:function:dnf_packageset_new(DnfSack * sack)
    explicit PackageSet(const PackageSackWeakPtr & sack);

    /// @replaces libdnf:hy-packageset.h:function:dnf_packageset_clone(DnfPackageSet * pset)
    PackageSet(const PackageSet & pset);

    PackageSet(PackageSet && pset) noexcept;

    /// @replaces libdnf:hy-packageset.h:function:dnf_packageset_free(DnfPackageSet * pset)
    ~PackageSet();

    PackageSet & operator=(const PackageSet & src);
    PackageSet & operator=(PackageSet && src);

    using iterator = PackageSetIterator;
    iterator begin() const;
    iterator end() const;

    /// @replaces libdnf:sack/packageset.hpp:method:PackageSet.operator+=(const libdnf::PackageSet & other)
    PackageSet & operator|=(const PackageSet & other);

    /// @replaces libdnf:sack/packageset.hpp:method:PackageSet.operator-=(const libdnf::PackageSet & other)
    PackageSet & operator-=(const PackageSet & other);

    /// @replaces libdnf:sack/packageset.hpp:method:PackageSet.operator/=(const libdnf::PackageSet & other)
    PackageSet & operator&=(const PackageSet & other);

    /// @replaces libdnf:sack/packageset.hpp:method:PackageSet.clear()
    void clear() noexcept;

    /// @replaces libdnf:sack/packageset.hpp:method:PackageSet.empty()
    bool empty() const noexcept;

    /// @replaces libdnf:sack/packageset.hpp:method:PackageSet.set(DnfPackage * pkg)
    /// @replaces libdnf:hy-packageset.h:function:dnf_packageset_add(DnfPackageSet * pset, DnfPackage * pkg)
    void add(const Package & pkg);

    /// @replaces libdnf:sack/packageset.hpp:method:PackageSet.has(DnfPackage * pkg)
    bool contains(const Package & pkg) const noexcept;

    void remove(const Package & pkg);

    /// @replaces libdnf:sack/packageset.hpp:method:PackageSet.getSack()
    PackageSackWeakPtr get_sack() const;

    /// @replaces libdnf:sack/packageset.hpp:method:PackageSet.size()
    /// @replaces libdnf:hy-packageset.h:function:dnf_packageset_count(DnfPackageSet * pset)
    size_t size() const noexcept;

    void swap(PackageSet & other) noexcept;

private:
    friend PackageSetIterator;
    friend PackageQuery;
    friend Transaction;
    friend class libdnf::base::Transaction;
    friend libdnf::Goal;
    friend libdnf::Swdb;
    PackageSet(const PackageSackWeakPtr & sack, libdnf::solv::SolvMap & solv_map);
    class Impl;
    std::unique_ptr<Impl> p_impl;
};


}  // namespace libdnf::rpm


#endif  // LIBDNF_RPM_PACKAGE_SET_HPP
