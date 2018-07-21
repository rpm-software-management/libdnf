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

#include <iostream>
#include <sstream>
#include <utility>
extern "C" {
#include <solv/solver.h>
}

#include "ModulePackage.hpp"
#include "libdnf/utils/File.hpp"

static void setSovable(Pool *pool, Solvable *solvable, std::string name, std::string stream, std::string version, std::string context, const char * arch)
{
    std::ostringstream ss;
    // create solvable with:
    //   Name: $name:$stream:$version:$context
    //   Version: 0
    //   Arch: $arch
    ss << name << ":" << stream << ":" << version << ":" << context;
    solvable_set_str(solvable, SOLVABLE_NAME, ss.str().c_str());
    solvable_set_str(solvable, SOLVABLE_EVR, "0");
    // TODO Test can be remove when modules will be always with arch
    solvable_set_str(solvable, SOLVABLE_ARCH, arch ? arch : "noarch");
    // create Provide: module($name)
    ss.str(std::string());
    ss << "module(" << name << ")";
    auto depId = pool_str2id(pool, ss.str().c_str(), 1);
    solvable_add_deparray(solvable, SOLVABLE_PROVIDES, depId, -1);

    // create Provide: module($name:$stream)
    ss.str(std::string());
    ss << "module(" << name << ":" << stream << ")";
    depId = pool_str2id(pool, ss.str().c_str(), 1);
    solvable_add_deparray(solvable, SOLVABLE_PROVIDES, depId, -1);

    // create Provide: module($name:$stream:$version)
    ss.str(std::string());
    ss << "module(" << name << ":" << stream << ":" << version << ")";
    depId = pool_str2id(pool, ss.str().c_str(), 1);
    solvable_add_deparray(solvable, SOLVABLE_PROVIDES, depId, -1);
}

ModulePackage::ModulePackage(Pool *pool, Repo *repo, const std::shared_ptr<ModuleMetadata> &metadata)
        : metadata(metadata)
        , state(ModuleState::UNKNOWN)
        , pool(pool)
{
    id = repo_add_solvable(repo);
    Solvable *solvable = pool_id2solvable(pool, id);

    setSovable(pool, solvable, getName(), getStream(), getVersion(), getContext(), metadata->getArchitecture());
    createDependencies(solvable);
}

/**
 * @brief Create stream dependencies based on modulemd metadata.
 */
void ModulePackage::createDependencies(Solvable *solvable) const
{
    std::ostringstream ss;
    Id depId;

    for (const auto &dependency : getModuleDependencies()) {
        for (const auto &requires : dependency->getRequires()) {
            for (const auto &singleRequires : requires) {
                auto moduleName = singleRequires.first;

                ss.str(std::string());
                ss << "module(" << moduleName;

                bool hasStreamRequires = false;
                for (const auto &moduleStream : singleRequires.second) {
                    if (moduleStream.find('-', 0) != std::string::npos) {
                        ss << ":" << moduleStream.substr(1) << ")";
                        depId = pool_str2id(pool, ss.str().c_str(), 1);
                        solvable_add_deparray(solvable, SOLVABLE_CONFLICTS, depId, -1);
                    } else {
                        hasStreamRequires = true;
                        ss << ":" << moduleStream << ")";
                        depId = pool_str2id(pool, ss.str().c_str(), 1);
                        solvable_add_deparray(solvable, SOLVABLE_REQUIRES, depId, 0);
                    }
                }

                if (!hasStreamRequires) {
                    // without stream; just close parenthesis
                    ss << ")";
                    depId = pool_str2id(pool, ss.str().c_str(), 1);
                    solvable_add_deparray(solvable, SOLVABLE_REQUIRES, depId, -1);
                }
            }
        }
    }
}

/**
 * @brief Return module $name.
 *
 * @return std::string
 */
std::string ModulePackage::getName() const
{
    return metadata->getName();
}

/**
 * @brief Return module $name:$stream.
 *
 * @return std::string
 */
std::string ModulePackage::getNameStream() const
{
    std::ostringstream ss;
    ss << getName() << ":" << getStream();
    return ss.str();
}

/**
 * @brief Return module $name:$stream:$version.
 *
 * @return std::string
 */
std::string ModulePackage::getNameStreamVersion() const
{
    std::ostringstream ss;
    ss << getNameStream() << ":" << getVersion();
    return ss.str();
}

/**
 * @brief Return module $stream.
 *
 * @return std::string
 */
std::string ModulePackage::getStream() const
{
    return metadata->getStream();
}

/**
 * @brief Return module $version converted to string.
 *
 * @return std::string
 */
std::string ModulePackage::getVersion() const
{
    return std::to_string(metadata->getVersion());
}

/**
 * @brief Return module $context.
 *
 * @return std::string
 */
std::string ModulePackage::getContext() const
{
    return metadata->getContext();
}

/**
 * @brief Return module $arch.
 *
 * @return std::string
 */
const char * ModulePackage::getArchitecture() const
{
    return metadata->getArchitecture();
}

/**
 * @brief Return module $name:$stream:$version:$context:$arch.
 *
 * @return std::string
 */
std::string ModulePackage::getFullIdentifier() const
{
    std::ostringstream ss;
    ss << getName() << ":" << getStream() << ":" << getVersion() << ":" << getContext() << ":"
       << getArchitecture();
    return ss.str();
}

/**
 * @brief Return module $summary.
 *
 * @return std::string
 */
std::string ModulePackage::getSummary() const
{
    return metadata->getSummary();
}

/**
 * @brief Return module $description.
 *
 * @return std::string
 */
std::string ModulePackage::getDescription() const
{
    return metadata->getDescription();
}

/**
 * @brief Return list of RPM NEVRAs in a module.
 *
 * @return std::vector<std::string>
 */
std::vector<std::string> ModulePackage::getArtifacts() const
{
    return metadata->getArtifacts();
}

/**
 * @brief Return a module profile by name.
 *
 * @return std::shared_ptr<ModuleProfile>
 */
std::shared_ptr<Profile> ModulePackage::getProfile(const std::string &name) const
{
    return metadata->getProfile(name);
}

/**
 * @brief Return list of ModuleProfiles.
 *
 * @return std::vector<std::shared_ptr<ModuleProfile>>
 */
std::vector<std::shared_ptr<ModuleProfile>> ModulePackage::getProfiles() const
{
    return metadata->getProfiles();
}

/**
 * @brief Return list of ModuleDependencies.
 *
 * @return std::vector<std::shared_ptr<ModuleDependencies>>
 */
std::vector<std::shared_ptr<ModuleDependencies>> ModulePackage::getModuleDependencies() const
{
    return metadata->getDependencies();
}

/**
 * @brief Is a ModulePackage part of an enabled stream?
 *
 * @return bool
 */
bool ModulePackage::isEnabled()
{
    return state == ModuleState::ENABLED;
}

/**
 * @brief Mark ModulePackage as part of an enabled stream.
 */
void ModulePackage::enable()
{
    state = ModuleState::ENABLED;
}

/**
 * @brief Add conflict with a module stream represented as a ModulePackage.
 */
void ModulePackage::addStreamConflict(const std::shared_ptr<ModulePackage> &package)
{
    std::ostringstream ss;
    Solvable *solvable = pool_id2solvable(pool, id);

    ss << "module(" + package->getNameStream() + ")";
    auto depId = pool_str2id(pool, ss.str().c_str(), 1);
    solvable_add_deparray(solvable, SOLVABLE_CONFLICTS, depId, -1);
}

static std::string getPlatformStream(const std::string &osReleasePath)
{
    auto file = libdnf::File::newFile(osReleasePath);
    file->open("r");
    std::string line;
    while (file->readLine(line)) {
        if (line.find("PLATFORM_ID") != std::string::npos) {
            auto equalCharPosition = line.find('=');
            if (equalCharPosition == std::string::npos)
                throw std::runtime_error("Invalid format (missing '=') of PLATFORM_ID in " + osReleasePath);

            auto colonCharPosition = line.find(':', equalCharPosition + 1);
            if (colonCharPosition == std::string::npos) {
                throw std::runtime_error("Invalid format (missing ':' in value) of PLATFORM_ID in " + osReleasePath);
            }
            auto endPosition = line.find('"', colonCharPosition + 1);
            if (endPosition == std::string::npos) {
                throw std::runtime_error("Invalid format (missing '\"' in value) of PLATFORM_ID in " + osReleasePath);
            }
            return line.substr(colonCharPosition + 1, endPosition - colonCharPosition -1);
        }
    }

    throw std::runtime_error("Missing PLATFORM_ID in " + osReleasePath);
}

Id createPlatformSolvable(Pool *pool, const std::string &osReleasePath)
{
    Repo *repo = repo_create(pool, HY_SYSTEM_REPO_NAME);
    Id id = repo_add_solvable(repo);
    Solvable *solvable = pool_id2solvable(pool, id);

    std::string name = "platform";
    std::string stream = getPlatformStream(osReleasePath);
    std::string version = "0";
    std::string context = "00000000";
    setSovable(pool, solvable, name, stream, version, context, "noarch");

    return id;
}

std::unique_ptr<libdnf::IdQueue> moduleSolve(const std::vector<std::shared_ptr<ModulePackage>> & modules)
{
    if (modules.empty()) {
        return {};
    }
    Pool * pool = modules[0]->getPool();
    pool_createwhatprovides(pool);
    std::vector<Id> solvedIds;
    libdnf::IdQueue job;
    for (const auto &module : modules) {
        std::ostringstream ss;
        ss << "module(" << module->getName() << ":" << module->getStream() << ":" << module->getVersion() << ")";
        Id dep = pool_str2id(pool, ss.str().c_str(), 1);
        job.pushBack(SOLVER_SOLVABLE_PROVIDES | SOLVER_INSTALL | SOLVER_WEAK, dep);
    }
    auto solver = solver_create(pool);
    solver_solve(solver, job.getQueue());
    auto transaction = solver_create_transaction(solver);
    // TODO Use Goal to allow debuging

    std::unique_ptr<libdnf::IdQueue> installed(new libdnf::IdQueue);
    transaction_installedresult(transaction, installed->getQueue());
    solver_free(solver);
    return installed;
}
