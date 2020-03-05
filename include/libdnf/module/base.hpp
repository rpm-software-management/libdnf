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


#ifndef LIBDNF_MODULE_BASE_HPP
#define LIBDNF_MODULE_BASE_HPP

#include "sack.hpp"

#include "libdnf/base/base.hpp"


namespace libdnf::module {


/// @replaces dnf:dnf/module/module_base.py:class:ModuleBase
class Base {
public:
    explicit Base(libdnf::Base * dnf_base);

    void get_sack() const;

    void get_goal() const;

private:
    const libdnf::Base * dnf_base;
};


/*
Base::Base(libdnf::Base * dnf_base)
    : dnf_base{dnf_base}
{
}
*/


}  // namespace libdnf::module


/*
dnf:dnf/module/module_base.py:method:ModuleBase.disable(self, module_specs)
dnf:dnf/module/module_base.py:method:ModuleBase.enable(self, module_specs)
dnf:dnf/module/module_base.py:method:ModuleBase.get_modules(self, module_spec)
dnf:dnf/module/module_base.py:method:ModuleBase.install(self, module_specs, strict=True)
dnf:dnf/module/module_base.py:method:ModuleBase.remove(self, module_specs)
dnf:dnf/module/module_base.py:method:ModuleBase.reset(self, module_specs)
dnf:dnf/module/module_base.py:method:ModuleBase.upgrade(self, module_specs)
*/

#endif
