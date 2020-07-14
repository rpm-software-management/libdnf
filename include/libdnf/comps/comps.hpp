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


#ifndef LIBDNF_COMPS_COMPS_HPP
#define LIBDNF_COMPS_COMPS_HPP


#include <string>

#include "group/group.hpp"
#include "group/sack.hpp"


namespace libdnf::comps {


class Comps {
public:
    // TODO(dmach): load to a new Comps object and merge when it's fully loaded for better transactional behavior
    void load_from_file(const std::string & path);
    void load_installed();
    GroupSack & get_group_sack() { return group_sack; }

private:
    GroupSack group_sack{*this};
};


}  // namespace libdnf::comps


#endif  // LIBDNF_COMPS_COMPS_HPP
