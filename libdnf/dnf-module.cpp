/*
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or(at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */
/**
 * SECTION:dnf-module
 * @short_description: Module management
 * @include: dnf-module.h
 * @stability: Unstable
 *
 * High level interface for managing modules.
 */
#include <sstream>

#include "dnf-module.h"
#include "log.hpp"

static auto logger(libdnf::Log::getLogger());

namespace libdnf {

/**
 * dnf_module_dummy
 * @module_list: The list of modules
 *
 * Dummy module method
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.0.0
 **/
bool dnf_module_dummy(const std::vector<std::string> & module_list)
{
    logger->debug("*** called dnf_module_dummy()");

    if (module_list.size() == 0) {
        return false;
    }

    int i = 0;
    for (const auto &module_spec : module_list) {
        std::ostringstream oss;
        oss << "module #" << i++ << " " << module_spec;
        logger->debug(oss.str());
    }

    return true;
}

}
