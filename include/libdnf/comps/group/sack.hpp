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


#ifndef LIBDNF_COMPS_GROUP_SACK_HPP
#define LIBDNF_COMPS_GROUP_SACK_HPP


#include "group.hpp"
#include "query.hpp"

#include "libdnf/common/sack/sack.hpp"
#include "libdnf/comps/comps.hpp"

#include <memory>


namespace libdnf::comps {


class Comps;


class GroupSack : public libdnf::sack::Sack<Group, GroupQuery> {
public:
    explicit GroupSack(Comps & comps) : comps(&comps) {}

    /// Create a new Group and store in in the GroupSack
    GroupWeakPtr new_group();

    /// Move an existing Group object to the GroupSack
    void add_group(std::unique_ptr<Group> && group) { add_item(std::move(group)); }

private:
    Comps * comps;
};


inline GroupWeakPtr GroupSack::new_group() {
    auto group = std::make_unique<Group>();
    return add_item_with_return(std::move(group));
}


}  // namespace libdnf::comps


#endif  // LIBDNF_COMPS_GROUP_SACK_HPP
