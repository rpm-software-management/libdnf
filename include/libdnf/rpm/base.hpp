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


#ifndef LIBDNF_RPM_BASE_HPP
#define LIBDNF_RPM_BASE_HPP

// forward declarations
namespace libdnf::rpm {
class Base;
}  // namespace libdnf::rpm


#include "goal.hpp"
#include "package.hpp"
#include "query.hpp"
#include "sack.hpp"

#include "libdnf/base/base.hpp"


namespace libdnf::rpm {


/// class Base
///
class Base {
public:
    explicit Base(libdnf::Base & dnf_base);

    /// @replaces dnf:dnf/base.py:attribute:Base.sack
    /// @replaces libdnf:libdnf/dnf-context.h:function:dnf_context_get_sack(DnfContext * context)
    Sack get_sack() const;

    /// @replaces libdnf:libdnf/dnf-context.h:function:dnf_context_get_goal(DnfContext * context)
    void get_goal();

private:
    const libdnf::Base & dnf_base;
};


/*
Base::Base(libdnf::Base & dnf_base)
    : dnf_base{dnf_base}
{
}
*/


}  // namespace libdnf::rpm

#endif
