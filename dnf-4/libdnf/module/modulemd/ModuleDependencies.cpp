/*
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libdnf/utils/utils.hpp"
#include "ModuleDependencies.hpp"

namespace libdnf {

ModuleDependencies::ModuleDependencies() : dependencies(nullptr) {}

ModuleDependencies::ModuleDependencies(ModulemdDependencies *dependencies)
        : dependencies(dependencies)
{}

std::vector<std::map<std::string, std::vector<std::string>>> ModuleDependencies::getRequires() const
{
    if (!dependencies) {
        return {};
    }
    std::vector<std::map<std::string, std::vector<std::string>>> resultRequires;
    // In order to preserve API we return vector of maps but there can ever only be one map
    resultRequires.reserve(1);
    std::map<std::string, std::vector<std::string>> moduleRequirements;

    char** runtimeReqModules = modulemd_dependencies_get_runtime_modules_as_strv(dependencies);

    for (char **iterModule = runtimeReqModules; iterModule && *iterModule; iterModule++) {
        char** runtimeReqStreams = modulemd_dependencies_get_runtime_streams_as_strv(dependencies, *iterModule);
        std::string moduleName = static_cast<char *>(*iterModule);
        auto & moduleStreamVector = moduleRequirements[moduleName];
        for (char **iterStream = runtimeReqStreams; iterStream && *iterStream; iterStream++) {
            moduleStreamVector.emplace_back(*iterStream);
        }
        g_strfreev(runtimeReqStreams);
    }
    resultRequires.push_back(moduleRequirements);
    g_strfreev(runtimeReqModules);

    return resultRequires;
}

}
