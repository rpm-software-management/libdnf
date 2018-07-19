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

#include <algorithm>
#include <set>
extern "C" {
#include <solv/poolarch.h>
}

#include "ModulePackageContainer.hpp"
#include "ModulePackageMaker.hpp"

#include "libdnf/utils/utils.hpp"
#include "libdnf/utils/File.hpp"

ModulePackageContainer::ModulePackageContainer(const std::shared_ptr<Pool> &pool, const std::string &arch)
        : pool(pool)
{
    pool_setarch(pool.get(), arch.c_str());
}

void ModulePackageContainer::add(const std::shared_ptr<ModulePackage> &package)
{
    modules.insert(std::make_pair(package->getId(), package));
}

void ModulePackageContainer::add(const std::vector<std::shared_ptr<ModulePackage>> &packages)
{
    for (const auto &package : packages) {
        add(package);
    }
}

void ModulePackageContainer::add(const std::map<Id, std::shared_ptr<ModulePackage>> &packages)
{
    modules.insert(std::begin(packages), std::end(packages));
}

std::shared_ptr<ModulePackage> ModulePackageContainer::getModulePackage(Id id)
{
    return modules[id];
}

void ModulePackageContainer::enable(const std::string &name, const std::string &stream)
{
    for (const auto &iter : modules) {
        auto modulePackage = iter.second;
        if (modulePackage->getName() == name && modulePackage->getStream() == stream) {
            modulePackage->enable();
        }
    }
}

std::vector<std::shared_ptr<ModulePackage>> ModulePackageContainer::getActiveModulePackages(const std::map<std::string, std::string> &defaultStreams)
{
    std::vector<std::shared_ptr<ModulePackage>> packages;

    for (const auto &iter : modules) {
        auto module = iter.second;

        bool hasDefaultStream;
        try {
            hasDefaultStream = defaultStreams.at(module->getName()) == module->getStream();
        } catch (std::out_of_range &exception) {
            hasDefaultStream = false;
            // TODO logger.debug(exception.what())
        }

        if (module->isEnabled() || hasDefaultStream) {
            packages.push_back(module);
        }
    }
    return getActiveModulePackages(packages);
}

std::vector<std::shared_ptr<ModulePackage>> ModulePackageContainer::getActiveModulePackages(const std::vector<std::shared_ptr<ModulePackage>> &modulePackages)
{
    std::vector<std::shared_ptr<ModulePackage>> packages;
    auto ids = moduleSolve(modulePackages);

    packages.reserve(ids->size());
    for (int i = 0; i < ids->size(); ++i) {
        Id id = (*ids)[i];
        auto solvable = pool_id2solvable(pool.get(), id);
        // TODO use Goal::listInstalls() to not requires filtering out Platform
        if (strcmp(solvable->repo->name, HY_SYSTEM_REPO_NAME) == 0)
            continue;
        packages.push_back(modules[id]);
    }
    return packages;
}

std::vector<std::shared_ptr<ModulePackage>> ModulePackageContainer::getModulePackages()
{
    std::vector<std::shared_ptr<ModulePackage>> values;

    std::transform(std::begin(modules), std::end(modules), std::back_inserter(values),
                   [](const std::map<Id, std::shared_ptr<ModulePackage>>::value_type &pair){ return pair.second; });

    return values;
}


