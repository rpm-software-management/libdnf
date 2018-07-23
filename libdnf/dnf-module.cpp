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
#include "nsvcap.hpp"

static auto logger(libdnf::Log::getLogger());

namespace {

void dnf_module_parse_spec(const std::string specStr, libdnf::Nsvcap & parsed)
{
    std::ostringstream oss;
    oss << "Parsing module_spec=" << specStr;
    logger->debug(oss.str());

    parsed.clear();

    for (std::size_t i = 0;
         HY_MODULE_FORMS_MOST_SPEC[i] != _HY_MODULE_FORM_STOP_;
         ++i) {
        if (parsed.parse(specStr.c_str(), HY_MODULE_FORMS_MOST_SPEC[i])) {
            std::ostringstream().swap(oss);
            oss << "N:S:V:C:A/P = ";
            oss << parsed.getName() << ":";
            oss << parsed.getStream() << ":";
            oss << parsed.getVersion() << ":";
            oss << parsed.getArch() << "/";
            oss << parsed.getProfile();

            logger->debug(oss.str());
            return;
        }
    }

    std::ostringstream().swap(oss);
    oss << "Invalid spec '";
    oss << specStr << "'";
    logger->debug(oss.str());
    throw ModulePackageContainer::Exception(oss.str());
}

}

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

bool dnf_module_enable(const std::vector<std::string> & module_list)
{
    ModuleExceptionList exList;

    if (module_list.empty())
        exList.add("module list cannot be empty");

    for (const auto & spec : module_list) {
        Nsvcap specParsed;
        std::ostringstream oss;

        oss << "Parsing spec '" << spec << "'";
        logger->debug(oss.str());

        try {
            dnf_module_parse_spec(spec, specParsed);
        } catch (ModulePackageContainer::Exception &e) {
            exList.add(e);
            continue;
        }

        std::ostringstream().swap(oss);
        oss << "Name = " << specParsed.getName() << "\n";
        oss << "Stream = " << specParsed.getStream() << "\n";
        oss << "Profile = " << specParsed.getProfile() << "\n";
        oss << "Version = " << specParsed.getVersion() << "\n";
        logger->debug(oss.str());
    }

    if (!exList.empty())
        throw exList;

    return true;
}

}
