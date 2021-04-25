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
#include <fnmatch.h>
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

    /**
     * @brief create solvable with:
     *   Name: $name:$stream:$context
     *   Version: $version
     *   Arch: $arch (If arch is not defined, set "noarch")
     *   Provides: module($name)
     *   Provides: module($name:$stream)
     */
    static void setSovable(Pool * pool, Solvable *solvable, std::string & name,
    std::string & stream, std::string & version, std::string & context, const char * arch)
{
    std::ostringstream ss;
    //   Name: $name:$stream:$context
    ss << name << ":" << stream << ":" << context;
    solvable_set_str(solvable, SOLVABLE_NAME, ss.str().c_str());
    solvable_set_str(solvable, SOLVABLE_EVR, version.c_str());
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
}


static std::pair<std::string, std::string> parsePlatform(const std::string & platform)
{
    auto index = platform.find(':');
    if (index == std::string::npos) {
        return {};
    }
    return make_pair(platform.substr(0,index), platform.substr(index+1));
}

ModulePackage::ModulePackage(DnfSack * moduleSack, LibsolvRepo * repo,
    ModulemdModuleStream * mdStream, const std::string & repoID)
        : mdStream(mdStream)
        , moduleSack(moduleSack)
        , repoID(repoID)
{
    if (mdStream != nullptr) {
        g_object_ref(mdStream);
    }
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

ModulePackage::ModulePackage(const ModulePackage & mpkg)
        : mdStream(mpkg.mdStream)
        , moduleSack(mpkg.moduleSack)
        , repoID(mpkg.repoID)
        , id(mpkg.id)
{
    if (mdStream != nullptr) {
        g_object_ref(mdStream);
    }
}

ModulePackage & ModulePackage::operator=(const ModulePackage & mpkg)
{
    if (this != &mpkg) {
        if (mdStream != nullptr) {
            g_object_unref(mdStream);
        }
        mdStream = mpkg.mdStream;
        if (mdStream != nullptr) {
            g_object_ref(mdStream);
        }
        moduleSack = mpkg.moduleSack;
        repoID = mpkg.repoID;
        id = mpkg.id;
    }
    return *this;
}

ModulePackage::~ModulePackage()
{
    if (mdStream != nullptr) {
        g_object_unref(mdStream);
    }
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

std::vector<std::string> ModulePackage::getRequires(ModulemdModuleStream * mdStream, bool removePlatform)
{
    std::vector<std::string> dependencies_result;

    GPtrArray * cDependencies = modulemd_module_stream_v2_get_dependencies((ModulemdModuleStreamV2 *) mdStream);

    for (unsigned int i = 0; i < cDependencies->len; i++) {
        auto dependencies = static_cast<ModulemdDependencies *>(g_ptr_array_index(cDependencies, i));
        if (!dependencies) {
            continue;
        }
        char** runtimeReqModules = modulemd_dependencies_get_runtime_modules_as_strv(dependencies);

        for (char **iterModule = runtimeReqModules; iterModule && *iterModule; iterModule++) {
            char ** runtimeReqStreams = modulemd_dependencies_get_runtime_streams_as_strv(dependencies, *iterModule);
            auto moduleName = static_cast<char *>(*iterModule);
            if (removePlatform && strcmp(moduleName, "platform") == 0) {
                g_strfreev(runtimeReqStreams);
                continue;
            }
            std::ostringstream ss;
            std::vector<std::string> requiredStreams;
            for (char **iterStream = runtimeReqStreams; iterStream && *iterStream; iterStream++) {
                requiredStreams.push_back(*iterStream);
            }
            if (requiredStreams.empty()) {
                dependencies_result.emplace_back(moduleName);
            } else {
                std::sort(requiredStreams.begin(), requiredStreams.end());
                ss << moduleName << ":" << "[" << *requiredStreams.begin();
                for (auto iter = std::next(requiredStreams.begin()); iter != requiredStreams.end(); ++iter) {
                    ss << "," << *iter;
                }
                ss << "]";
                dependencies_result.emplace_back(std::move(ss.str()));
            }
            g_strfreev(runtimeReqStreams);
        }
        g_strfreev(runtimeReqModules);
    }

    return dependencies_result;
}

std::string ModulePackage::getYaml() const
{
    ModulemdModuleIndex * i = modulemd_module_index_new();
    modulemd_module_index_add_module_stream(i, mdStream, NULL);
    gchar *cStrYaml = modulemd_module_index_dump_to_string(i, NULL);
    std::string yaml = std::string(cStrYaml);
    g_free(cStrYaml);
    g_object_unref(i);
    return yaml;
}

bool ModulePackage::getStaticContext() const
{
    return modulemd_module_stream_v2_is_static_context((ModulemdModuleStreamV2 *) mdStream);
}

/**
 * @brief Return module $name.
 *
 * @return const char *
 */
const char * ModulePackage::getNameCStr() const
{
    return modulemd_module_stream_get_module_name(mdStream);
}

std::string ModulePackage::getName() const
{
    auto name = modulemd_module_stream_get_module_name(mdStream);
    return name ? name : "";
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
    return modulemd_module_stream_get_stream_name(mdStream);
}

std::string ModulePackage::getStream() const
{
    auto stream = modulemd_module_stream_get_stream_name(mdStream);
    return stream ? stream : "";
}

/**
 * @brief Return module $version converted to string.
 *
 * @return std::string
 */
std::string ModulePackage::getVersion() const
{
    return std::to_string(modulemd_module_stream_get_version(mdStream));
}

/**
 * @brief Return module $version.
 *
 * @return Long Long
 */
long long ModulePackage::getVersionNum() const
{
    return modulemd_module_stream_get_version(mdStream);
}

/**
 * @brief Return module $context.
 *
 * @return std::string
 */
const char * ModulePackage::getContextCStr() const
{
    return modulemd_module_stream_get_context(mdStream);
}

std::string ModulePackage::getContext() const
{
    auto context = modulemd_module_stream_get_context(mdStream);
    return context ? context : "";
}


/**
 * @brief Return module $arch.
 *
 * @return std::string
 */
const char * ModulePackage::getArchCStr() const
{
    return modulemd_module_stream_get_arch(mdStream);
}

std::string ModulePackage::getArch() const
{
    auto arch = modulemd_module_stream_get_arch(mdStream);
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
    return modulemd_module_stream_v2_get_summary((ModulemdModuleStreamV2 *) mdStream, NULL);
}

/**
 * @brief Return module $description.
 *
 * @return std::string
 */
std::string ModulePackage::getDescription() const
{
    return modulemd_module_stream_v2_get_description((ModulemdModuleStreamV2 *) mdStream, NULL);
}

/**
 * @brief Return list of RPM NEVRAs in a module.
 *
 * @return std::vector<std::string>
 */
std::vector<std::string> ModulePackage::getArtifacts() const
{
    std::vector<std::string> result_rpms;
    char ** rpms = modulemd_module_stream_v2_get_rpm_artifacts_as_strv((ModulemdModuleStreamV2 *) mdStream);

    for (char **iter = rpms; iter && *iter; iter++) {
        result_rpms.emplace_back(std::string(*iter));
    }

    g_strfreev(rpms);
    return result_rpms;
}

std::vector<ModuleProfile>
ModulePackage::getProfiles(const std::string &name) const
{
    std::vector<ModuleProfile> result_profiles;

    //TODO(amatej): replace with
    //char ** profiles = modulemd_module_stream_v2_search_profiles((ModulemdModuleStreamV2 *) mdStream, profileNameCStr);
    char ** profiles = modulemd_module_stream_v2_get_profile_names_as_strv((ModulemdModuleStreamV2 *) mdStream);

    auto profileNameCStr = name.c_str();
    gboolean glob = hy_is_glob_pattern(profileNameCStr);
    for (char **iter = profiles; iter && *iter; iter++) {
        std::string keyStr = static_cast<char *>(*iter);
        if (glob && fnmatch(profileNameCStr, static_cast<char *>(*iter), 0) == 0) {
            result_profiles.push_back(ModuleProfile(modulemd_module_stream_v2_get_profile((ModulemdModuleStreamV2 *) mdStream, *iter)));
        } else if (strcmp(profileNameCStr, static_cast<char *>(*iter)) == 0) {
            result_profiles.push_back(ModuleProfile(modulemd_module_stream_v2_get_profile((ModulemdModuleStreamV2 *) mdStream, *iter)));
        }
    }

    g_strfreev(profiles);
    return result_profiles;
}

ModuleProfile
ModulePackage::getDefaultProfile() const
{
    //TODO(amatej): replace with
    //char ** profiles = modulemd_module_stream_v2_search_profiles((ModulemdModuleStreamV2 *) mdStream, profileNameCStr);
    char ** profiles = modulemd_module_stream_v2_get_profile_names_as_strv((ModulemdModuleStreamV2 *) mdStream);
    if (g_strv_length (profiles) == 1) {
        return ModuleProfile(modulemd_module_stream_v2_get_profile((ModulemdModuleStreamV2 *) mdStream, profiles[0]));
    }

    for (char **iter = profiles; iter && *iter; iter++) {
        auto profile = ModuleProfile(modulemd_module_stream_v2_get_profile((ModulemdModuleStreamV2 *) mdStream, *iter));
        if (profile.isDefault()) {
            return profile;
        }
    }

    throw std::runtime_error("No default profile found for " + getFullIdentifier());
}

/**
 * @brief Return list of ModuleProfiles.
 *
 * @return std::vector<ModuleProfile>
 */
std::vector<ModuleProfile> ModulePackage::getProfiles() const
{
    std::vector<ModuleProfile> result_profiles;
    char ** profiles = modulemd_module_stream_v2_get_profile_names_as_strv((ModulemdModuleStreamV2 *) mdStream);

    for (char **iter = profiles; iter && *iter; iter++) {
        result_profiles.push_back(ModuleProfile(modulemd_module_stream_v2_get_profile((ModulemdModuleStreamV2 *) mdStream, *iter)));
    }

    g_strfreev(profiles);
    return result_profiles;
}

/**
 * @brief Return list of ModuleDependencies.
 *
 * @return std::vector<ModuleDependencies>
 */
std::vector<ModuleDependencies> ModulePackage::getModuleDependencies() const
{
    std::vector<ModuleDependencies> dependencies;

    GPtrArray * cDependencies = modulemd_module_stream_v2_get_dependencies((ModulemdModuleStreamV2 *) mdStream);

    for (unsigned int i = 0; i < cDependencies->len; i++) {
        dependencies.emplace_back(static_cast<ModulemdDependencies *>(g_ptr_array_index(cDependencies, i)));
    }

    return dependencies;
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
        availableQuery.available();
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
            baseQuery.installed();
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

std::string ModulePackage::getNameStream(ModulemdModuleStream * mdStream)
{
    std::ostringstream ss;
    auto name = modulemd_module_stream_get_module_name(mdStream);
    auto stream = modulemd_module_stream_get_stream_name(mdStream);
    ss << (name ? name : "") << ":" << (stream ? stream : "");
    return ss.str();
}

}
