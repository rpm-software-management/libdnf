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


#ifndef LIBDNF_COMPS_CATEGORY_CATEGORY_HPP
#define LIBDNF_COMPS_CATEGORY_CATEGORY_HPP


#include "libdnf/comps/group/group.hpp"

#include <string>
#include <vector>


namespace libdnf::comps {


class Category {
public:
    std::string get_id() const;

    std::string get_name() const;

    std::string get_description() const;

    std::string get_translated_name() const;

    std::string get_translated_description() const;

    std::vector<Group> get_groups() const;

    std::vector<Group> get_groups(bool mandatory_groups, bool optional_groups) const;
};


}  // namespace libdnf::comps


#endif
