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
#include <numeric>
#include <sstream>
#include <utility>

extern "C" {
#include <solv/pool_parserpmrichdep.h>
}

#include "ModulePackage.hpp"
#include "modulemd/ModuleProfile.hpp"
#include "libdnf/utils/File.hpp"
#include "libdnf/dnf-sack-private.hpp"
#include "libdnf/repo/Repo-private.hpp"
#include "libdnf/sack/query.hpp"
#include "libdnf/log.hpp"
#include "../utils/bgettext/bgettext-lib.h"
#include "tinyformat/tinyformat.hpp"

#include <memory>


namespace libdnf {

static void setSovable(Pool * pool, Solvable *solvable, std::string name,
    std::string stream, std::string version, std::string context, const char * arch)
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


static std::pair<std::string, std::string> parsePlatform(const std::string & platform)
{
    auto index = platform.find(':');
    if (index == std::string::npos) {
        return {};
    }
    return make_pair(platform.substr(0,index), platform.substr(index+1));
    }

ModulePackage::~ModulePackage() = default;

ModulePackage::ModulePackage(DnfSack * moduleSack, LibsolvRepo * repo,
    ModuleMetadata && metadata,  const std::string & repoID)
        : metadata(std::move(metadata))
        , moduleSack(moduleSack)
        , repoID(repoID)
{
    Pool * pool = dnf_sack_get_pool(moduleSack);
    id = repo_add_solvable(repo);
    Solvable *solvable = pool_id2solvable(pool, id);

    setSovable(pool, solvable, getName(), getStream(), getVersion(), getContext(), getArchCStr());
    createDependencies(solvable);
    HyRepo hyRepo = static_cast<HyRepo>(repo->appdata);
    libdnf::repoGetImpl(hyRepo)->needs_internalizing = 1;
    dnf_sack_set_provides_not_ready(moduleSack);
    dnf_sack_set_considered_to_update(moduleSack);
}

/**
 * @brief Create stream dependencies based on modulemd metadata.
 */
void ModulePackage::createDependencies(Solvable *solvable) const
{
    Id depId;
    Pool * pool = dnf_sack_get_pool(moduleSack);

    for (const auto &dependency : getModuleDependencies()) {
        for (const auto &requires : dependency.getRequires()) {
            for (const auto &singleRequires : requires) {
                auto moduleName = singleRequires.first;
                std::vector<std::string> requiresStream;
                for (const auto &moduleStream : singleRequires.second) {
                    if (moduleStream.find('-', 0) != std::string::npos) {
                        std::ostringstream ss;
                        ss << "module(" << moduleName << ":" << moduleStream.substr(1) << ")";
                        depId = pool_str2id(pool, ss.str().c_str(), 1);
                        solvable_add_deparray(solvable, SOLVABLE_CONFLICTS, depId, 0);
                    } else {
                        std::string reqFormated = "module(" + moduleName + ":" + moduleStream + ")";
                        requiresStream.push_back(std::move(reqFormated));
                    }
                }
                if (requiresStream.empty()) {
                    std::ostringstream ss;
                    ss << "module(" << moduleName << ")";
                    depId = pool_str2id(pool, ss.str().c_str(), 1);
                    solvable_add_deparray(solvable, SOLVABLE_REQUIRES, depId, -1);
                } else if (requiresStream.size() == 1) {
                    auto & requireFormated = requiresStream[0];
                    depId = pool_str2id(pool, requireFormated.c_str(), 1);
                    solvable_add_deparray(solvable, SOLVABLE_REQUIRES, depId, -1);
                } else {
                    std::ostringstream ss;
                    ss << "(";
                    ss << std::accumulate(std::next(requiresStream.begin()),
                                            requiresStream.end(), requiresStream[0],
                                            [](std::string & a, std::string & b)
                                            { return a + " or " + b; });
                    ss << ")";
                    depId = pool_parserpmrichdep(pool, ss.str().c_str());
                    if (!depId)
                        throw std::runtime_error("Cannot parse module requires");
                    solvable_add_deparray(solvable, SOLVABLE_REQUIRES, depId, -1);
                }
            }
        }
    }
}

/**
 * @brief Return module $name.
 *
 * @return const char *
 */
const char * ModulePackage::getNameCStr() const
{
    return metadata.getName();
}

std::string ModulePackage::getName() const
{
    auto name = metadata.getName();
    return name ? name : "";
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

const std::string & ModulePackage::getRepoID() const
{
    return repoID;
}

/**
 * @brief Return module $stream.
 *
 * @return const char *
 */
const char * ModulePackage::getStreamCStr() const
{
    return metadata.getStream();
}

std::string ModulePackage::getStream() const
{
    auto stream = metadata.getStream();
    return stream ? stream : "";
}

/**
 * @brief Return module $version converted to string.
 *
 * @return std::string
 */
std::string ModulePackage::getVersion() const
{
    return std::to_string(metadata.getVersion());
}

/**
 * @brief Return module $version.
 *
 * @return Long Long
 */
long long ModulePackage::getVersionNum() const
{
    return metadata.getVersion();
}

/**
 * @brief Return module $context.
 *
 * @return std::string
 */
const char * ModulePackage::getContextCStr() const
{
    return metadata.getContext();
}

std::string ModulePackage::getContext() const
{
    auto context = metadata.getContext();
    return context ? context : "";
}


/**
 * @brief Return module $arch.
 *
 * @return std::string
 */
const char * ModulePackage::getArchCStr() const
{
    return metadata.getArchitecture();
}

std::string ModulePackage::getArch() const
{
    auto arch = metadata.getArchitecture();
    return arch ? arch : "";
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
       << getArch();
    return ss.str();
}

/**
 * @brief Return module $summary.
 *
 * @return std::string
 */
std::string ModulePackage::getSummary() const
{
    return metadata.getSummary();
}

/**
 * @brief Return module $description.
 *
 * @return std::string
 */
std::string ModulePackage::getDescription() const
{
    return metadata.getDescription();
}

/**
 * @brief Return list of RPM NEVRAs in a module.
 *
 * @return std::vector<std::string>
 */
std::vector<std::string> ModulePackage::getArtifacts() const
{
    return metadata.getArtifacts();
}

std::vector<ModuleProfile>
ModulePackage::getProfiles(const std::string &name) const
{
    return metadata.getProfiles(name);
}

/**
 * @brief Return list of ModuleProfiles.
 *
 * @return std::vector<std::shared_ptr<ModuleProfile>>
 */
std::vector<ModuleProfile> ModulePackage::getProfiles() const
{
    return metadata.getProfiles();
}

/**
 * @brief Return list of ModuleDependencies.
 *
 * @return std::vector<std::shared_ptr<ModuleDependencies>>
 */
std::vector<ModuleDependencies> ModulePackage::getModuleDependencies() const
{
    return metadata.getDependencies();
}

/**
 * @brief Add conflict with a module stream represented as a ModulePackage.
 */
void ModulePackage::addStreamConflict(const ModulePackage * package)
{
    Pool * pool = dnf_sack_get_pool(moduleSack);
    std::ostringstream ss;
    Solvable *solvable = pool_id2solvable(pool, id);

    ss << "module(" + package->getNameStream() + ")";
    auto depId = pool_str2id(pool, ss.str().c_str(), 1);
    solvable_add_deparray(solvable, SOLVABLE_CONFLICTS, depId, 0);
}

static std::pair<std::string, std::string> getPlatformStream(const std::string &osReleasePath)
{
    auto file = File::newFile(osReleasePath);
    file->open("r");
    std::string line;
    while (file->readLine(line)) {
        if (line.find("PLATFORM_ID") != std::string::npos) {
            auto equalCharPosition = line.find('=');
            if (equalCharPosition == std::string::npos)
                throw std::runtime_error("Invalid format (missing '=') of PLATFORM_ID in "
                + osReleasePath);
            auto startPosition = line.find('"', equalCharPosition + 1);
            if (startPosition == std::string::npos) {
                throw std::runtime_error("Invalid format (missing '\"' in value) of PLATFORM_ID in "
                + osReleasePath);
            }
            auto colonCharPosition = line.find(':', equalCharPosition + 1);
            if (colonCharPosition == std::string::npos) {
                throw std::runtime_error("Invalid format (missing ':' in value) of PLATFORM_ID in "
                + osReleasePath);
            }
            auto endPosition = line.find('"', colonCharPosition + 1);
            if (endPosition == std::string::npos) {
                throw std::runtime_error("Invalid format (missing '\"' in value) of PLATFORM_ID in "
                + osReleasePath);
            }
            return make_pair(
                line.substr(startPosition + 1, colonCharPosition - startPosition -1),
                line.substr(colonCharPosition + 1, endPosition - colonCharPosition -1));
        }
    }
    return {};
}

Id
ModulePackage::createPlatformSolvable(DnfSack * moduleSack, const std::string & osReleasePath,
    const std::string install_root, const char * platformModule)
{
    std::vector<std::string> paths;
    paths.push_back(osReleasePath);
    return createPlatformSolvable(nullptr, moduleSack, paths, install_root, platformModule);
}

Id
ModulePackage::createPlatformSolvable(DnfSack * sack, DnfSack * moduleSack,
        const std::vector<std::string> & osReleasePaths, const std::string install_root,
        const char *  platformModule)
{
    std::pair<std::string, std::string> parsedPlatform;
    std::string name;
    std::string stream;
    if (platformModule) {
        parsedPlatform = parsePlatform(platformModule);
        if (!parsedPlatform.first.empty() && !parsedPlatform.second.empty()) {
            name = parsedPlatform.first;
            stream = parsedPlatform.second;
        } else {
            throw std::runtime_error(
                tfm::format(_("Invalid format of Platform module: %s"), platformModule));
        }
    } else if (sack) {
        Query baseQuery(sack);
        baseQuery.addFilter(HY_PKG_PROVIDES, HY_EQ, "system-release");
        baseQuery.addFilter(HY_PKG_LATEST, HY_EQ, 1);
        baseQuery.apply();
        Query availableQuery(baseQuery);
        availableQuery.addFilter(HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
        auto platform = availableQuery.getStringsFromProvide("base-module");
        auto platformSize = platform.size();
        if (platformSize == 1) {
            parsedPlatform = parsePlatform(*platform.begin());
        } else if (platformSize > 1) {
            auto logger(Log::getLogger());
            logger->debug(_("Multiple module platforms provided by available packages\n"));
        }
        if (!parsedPlatform.first.empty() && !parsedPlatform.second.empty()) {
            name = parsedPlatform.first;
            stream = parsedPlatform.second;
        } else {
            baseQuery.addFilter(HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
            platform = baseQuery.getStringsFromProvide("base-module");
            platformSize = platform.size();
            if (platformSize == 1) {
                parsedPlatform = parsePlatform(*platform.begin());
            } else if (platformSize > 1) {
                auto logger(Log::getLogger());
                logger->debug(_("Multiple module platforms provided by installed packages\n"));
            }
            if (!parsedPlatform.first.empty() && !parsedPlatform.second.empty()) {
                name = parsedPlatform.first;
                stream = parsedPlatform.second;
            }
        }
    }

    if (name.empty() || stream.empty()) {
        for (auto & osReleasePath: osReleasePaths) {
            std::string path;
            if (install_root == "/") {
                path = osReleasePath;
            } else {
                if (install_root.back() == '/') {
                    path = install_root.substr(0, install_root.size() -1);
                } else {
                    path = install_root;
                }
                path += osReleasePath;
            }
            std::pair<std::string, std::string> platform;
            try {
                platform = getPlatformStream(path);
            } catch (const std::exception & except) {
                auto logger(Log::getLogger());
                logger->debug(tfm::format(_("Detection of Platform Module in %s failed: %s"),
                                          osReleasePath, std::string(except.what())));
            }
            if (!platform.first.empty() && !platform.second.empty()) {
                name = platform.first;
                stream = platform.second;
                break;
            } else {
                auto logger(Log::getLogger());
                logger->debug(tfm::format(_("Missing PLATFORM_ID in %s"), osReleasePath));
            }
        }
    }
    if (name.empty() || stream.empty()) {
        throw std::runtime_error(_("No valid Platform ID detected"));
    }
    std::string version = "0";
    std::string context = "00000000";

    Pool * pool = dnf_sack_get_pool(moduleSack);
    HyRepo hrepo = hy_repo_create(HY_SYSTEM_REPO_NAME);
    auto repoImpl = libdnf::repoGetImpl(hrepo);
    LibsolvRepo *repo = repo_create(pool, HY_SYSTEM_REPO_NAME);
    repo->appdata = hrepo;
    repoImpl->libsolvRepo = repo;
    repoImpl->needs_internalizing = 1;
    Id id = repo_add_solvable(repo);
    Solvable *solvable = pool_id2solvable(pool, id);
    setSovable(pool, solvable, name, stream, version, context, "noarch");
    repoImpl->needs_internalizing = 1;
    dnf_sack_set_provides_not_ready(moduleSack);
    dnf_sack_set_considered_to_update(moduleSack);
    pool_set_installed(pool, repo);
    return id;
}

}
