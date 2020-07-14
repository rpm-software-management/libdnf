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


#ifndef LIBDNF_COMPS_GROUP_QUERY_HPP
#define LIBDNF_COMPS_GROUP_QUERY_HPP

#include "libdnf/common/sack/query.hpp"
#include "libdnf/comps/group/group.hpp"
#include "libdnf/utils/weak_ptr.hpp"


namespace libdnf::comps {


/// Weak pointer to rpm repository. RepoWeakPtr does not own the repository (ptr_owner = false).
/// Repositories are owned by RepoSack.
using GroupWeakPtr = libdnf::WeakPtr<Group, false>;


class GroupQuery : public libdnf::sack::Query<GroupWeakPtr> {
public:
    using Query<GroupWeakPtr>::Query;

    GroupQuery & ifilter_id(sack::QueryCmp cmp, const std::string & pattern);
    GroupQuery & ifilter_id(sack::QueryCmp cmp, const std::vector<std::string> & patterns);
    GroupQuery & ifilter_uservisible(bool value);
    GroupQuery & ifilter_default(bool value);
    GroupQuery & ifilter_installed(bool value);

private:
    struct F {
        static std::string id(const GroupWeakPtr & obj) { return obj->get_id(); }
        static bool is_uservisible(const GroupWeakPtr & obj) { return obj->get_uservisible(); }
        static bool is_default(const GroupWeakPtr & obj) { return obj->get_default(); }
        static bool is_installed(const GroupWeakPtr & obj) { return obj->get_installed(); }
    };
};


inline GroupQuery & GroupQuery::ifilter_id(sack::QueryCmp cmp, const std::string & pattern) {
    ifilter(F::id, cmp, pattern);
    return *this;
}


inline GroupQuery & GroupQuery::ifilter_id(sack::QueryCmp cmp, const std::vector<std::string> & patterns) {
    ifilter(F::id, cmp, patterns);
    return *this;
}


inline GroupQuery & GroupQuery::ifilter_default(bool value) {
    ifilter(F::is_default, sack::QueryCmp::EQ, value);
    return *this;
}


inline GroupQuery & GroupQuery::ifilter_uservisible(bool value) {
    ifilter(F::is_uservisible, sack::QueryCmp::EQ, value);
    return *this;
}


inline GroupQuery & GroupQuery::ifilter_installed(bool value) {
    ifilter(F::is_installed, sack::QueryCmp::EQ, value);
    return *this;
}


}  // namespace libdnf::comps


#endif  // LIBDNF_COMPS_GROUP_QUERY_HPP
