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
#include <sstream>

extern "C" {
#include <solv/poolarch.h>
#include <solv/solver.h>
}

#include "ModulePackageContainer.hpp"
#include "libdnf/utils/filesystem.hpp"
#include "libdnf/utils/utils.hpp"
#include "libdnf/utils/File.hpp"
#include "libdnf/dnf-sack-private.hpp"
#include "libdnf/hy-query.h"
#include "libdnf/hy-types.h"
#include <functional>
#include <../sack/query.hpp>
#include "../log.hpp"
#include "libdnf/conf/ConfigParser.hpp"
#include "libdnf/conf/OptionStringList.hpp"
#include "libdnf/goal/Goal.hpp"
#include "libdnf/repo/Repo-private.hpp"
#include "libdnf/sack/selector.hpp"
#include "libdnf/conf/Const.hpp"

#include "bgettext/bgettext-lib.h"
#include "tinyformat/tinyformat.hpp"
#include "modulemd/ModuleMetadata.hpp"
#include "modulemd/ModuleProfile.hpp"


namespace {

/// Requires resolved goal
/// Takes listInstalls() from goal and keep solvables with the solvable-name (<name>:<stream>:<context>) in query 
void goal2name_query(libdnf::Goal & goal, libdnf::Query & query)
{
    auto pool = dnf_sack_get_pool(goal.getSack());
    auto installList = goal.listInstalls();
    std::vector<const char *> module_names;
    Id id = -1;
    while ((id = installList.next(id)) != -1) {
        Solvable * s = pool_id2solvable(pool, id);
        const char * name = pool_id2str(pool, s->name);
        module_names.push_back(name);
    }
    module_names.push_back(nullptr);
    query.addFilter(HY_PKG_NAME, HY_EQ, module_names.data());
}

}

namespace std {

template<>
struct default_delete<DIR> {
    void operator()(DIR * ptr) noexcept { closedir(ptr); }
};

}

namespace libdnf {

static constexpr auto EMPTY_STREAM = "";
static constexpr auto EMPTY_PROFILES = "";
static constexpr auto DEFAULT_STATE = "";

static const char * ENABLE_MULTIPLE_STREAM_EXCEPTION =
_("Cannot enable multiple streams for module '%s'");

static const std::string EMPTY_RESULT;

static std::string
stringFormater(std::string imput)
{
    return imput.empty() ? "*" : imput;
}

static ModulePackageContainer::ModuleState
fromString(const std::string &str) {
    if (str == "1" || str == "true" || str == "enabled")
        return ModulePackageContainer::ModuleState::ENABLED;
    if (str == "0" || str == "false" || str == "disabled")
        return ModulePackageContainer::ModuleState::DISABLED;

    return ModulePackageContainer::ModuleState::UNKNOWN;
}

static std::string
toString(const ModulePackageContainer::ModuleState &state) {
    switch (state) {
        case ModulePackageContainer::ModuleState::ENABLED:
            return "enabled";
        case ModulePackageContainer::ModuleState::DISABLED:
            return "disabled";
        case ModulePackageContainer::ModuleState::DEFAULT:
            return "";
        default:
            return "";
    }
}

static std::string getFileContent(const std::string &filePath)
{
    auto yaml = File::newFile(filePath);

    yaml->open("r");
    const auto &yamlContent = yaml->getContent();
    yaml->close();

    return yamlContent;
}

/**
 * @brief Get names in given directory with surfix ".yaml"
 *
 * @param dirPath p_dirPath: Directory
 * @return std::vector< std::string > Sorted vector with all entries in directory that ends ".yaml"
 */
static std::vector<std::string> getYamlFilenames(const char * dirPath)
{
    struct dirent * ent;
    std::unique_ptr<DIR> dir(opendir(dirPath));
    std::vector<std::string> fileNames;
    if (dir) {
        DIR * dirPtr = dir.get();
        while ((ent = readdir(dirPtr)) != NULL) {
            auto filename = ent->d_name;
            auto fileNameLen = strlen(filename);
            if (fileNameLen < 10 || strcmp(filename + fileNameLen - 5, ".yaml")) {
                continue;
            }
            fileNames.push_back(filename);
        }
    }
    std::sort(fileNames.begin(), fileNames.end());
    return fileNames;
}

static bool
stringStartWithLowerComparator(const std::string & first, const std::string & pattern)
{
    return first.compare(0, pattern.size(), pattern) < 0;
}

class ModulePackageContainer::Impl {
public:
    Impl();
    ~Impl();
    std::pair<std::vector<std::vector<std::string>>, ModulePackageContainer::ModuleErrorType> moduleSolve(
        const std::vector<ModulePackage *> & modules, bool debugSolver);
    bool insert(const std::string &moduleName, const char *path);
    std::vector<ModulePackage *> getLatestActiveEnabledModules();

private:
    friend struct ModulePackageContainer;
    class ModulePersistor;
    std::unique_ptr<ModulePersistor> persistor;
    std::map<Id, std::unique_ptr<ModulePackage>> modules;
    DnfSack * moduleSack;
    std::unique_ptr<PackageSet> activatedModules;
    std::string installRoot;
    std::string persistDir;
    ModuleMetadata moduleMetadata;

    std::map<std::string, std::string> moduleDefaults;
    bool isEnabled(const std::string &name, const std::string &stream);
};

class ModulePackageContainer::Impl::ModulePersistor {
public:
    ~ModulePersistor() = default;

    const std::string & getStream(const std::string &name);
    const std::vector<std::string> & getProfiles(const std::string &name);
    /**
     * @brief Can throw NoModuleException
     */
    const ModuleState & getState(const std::string &name);

    std::map<std::string, std::string> getEnabledStreams();
    std::vector<std::string> getDisabledModules();
    std::map<std::string, std::string> getDisabledStreams();
    std::vector<std::string> getResetModules();

    std::map<std::string, std::string> getResetStreams();
    std::map<std::string, std::pair<std::string, std::string>> getSwitchedStreams();
    std::map<std::string, std::vector<std::string>> getInstalledProfiles();
    std::map<std::string, std::vector<std::string>> getRemovedProfiles();

    std::vector<std::string> getAllModuleNames();
    bool changeStream(const std::string &name, const std::string &stream);
    bool addProfile(const std::string &name, const std::string &profile);
    bool removeProfile(const std::string &name, const std::string &profile);
    bool changeState(const std::string &name, ModuleState state);

    bool insert(const std::string &moduleName, const char *path);
    void rollback();
    void save(const std::string &installRoot, const std::string &modulesPath);

private:
    friend class Impl;
    friend struct ModulePackageContainer;
    struct Config {
        std::string stream;
        std::vector<std::string> profiles;
        ModuleState state;
        bool locked;
        int streamChangesNum;
    };
    std::pair<ConfigParser, struct Config> & getEntry(const std::string & moduleName);
    bool update(const std::string &name);
    void reset(const std::string &name);

    std::map<std::string, std::pair<ConfigParser, struct Config>> configs;
};

ModulePackageContainer::EnableMultipleStreamsException::EnableMultipleStreamsException(
    const std::string & moduleName)
: Exception(tfm::format(ENABLE_MULTIPLE_STREAM_EXCEPTION, moduleName)) {}

ModulePackageContainer::ModulePackageContainer(bool allArch, std::string installRoot,
    const char * arch, const char * persistDir) : pImpl(new Impl)
{
    if (allArch) {
        dnf_sack_set_all_arch(pImpl->moduleSack, TRUE);
    } else {
        dnf_sack_set_arch(pImpl->moduleSack, arch, NULL);
    }
    if (persistDir) {
        g_autofree gchar * dir = g_build_filename(persistDir, "modulefailsafe", NULL);
        pImpl->persistDir = dir;
    } else {
        g_autofree gchar * dir = g_build_filename(installRoot.c_str(), PERSISTDIR, "modulefailsafe",
                                                  NULL);
        pImpl->persistDir = dir;
    }

    Pool * pool = dnf_sack_get_pool(pImpl->moduleSack);
    HyRepo hrepo = hy_repo_create("available");
    auto repoImpl = libdnf::repoGetImpl(hrepo);
    LibsolvRepo *repo = repo_create(pool, "available");
    repo->appdata = hrepo;
    repoImpl->libsolvRepo = repo;
    repoImpl->needs_internalizing = 1;
    pImpl->installRoot = installRoot;
    g_autofree gchar * path = g_build_filename(pImpl->installRoot.c_str(),
                                              "/etc/dnf/modules.d", NULL);
    std::unique_ptr<DIR> dir(opendir(path));
    if (dir) {
        struct dirent * ent;
        /* Load "*.module" files into module persistor */
        DIR * dirPtr = dir.get();
        while ((ent = readdir(dirPtr)) != NULL) {
            auto filename = ent->d_name;
            auto fileNameLen = strlen(filename);
            if (fileNameLen < 8 || strcmp(filename + fileNameLen - 7, ".module")) {
                continue;
            }
            std::string name(filename, fileNameLen - 7);
            pImpl->persistor->insert(name, path);
        }
    }
}

ModulePackageContainer::~ModulePackageContainer() = default;

ModulePackageContainer::Impl::Impl() : persistor(new ModulePersistor), moduleSack(dnf_sack_new()) {}

ModulePackageContainer::Impl::~Impl()
{
    g_object_unref(moduleSack);
}

void
ModulePackageContainer::add(DnfSack * sack)
{
    Pool * pool = dnf_sack_get_pool(sack);
    LibsolvRepo * r;
    Id id;

    FOR_REPOS(id, r) {
        HyRepo hyRepo = static_cast<HyRepo>(r->appdata);
        auto modules_fn = hyRepo->getMetadataPath(MD_TYPE_MODULES);
        if (modules_fn.empty()) {
            continue;
        }
        std::string yamlContent = getFileContent(modules_fn);
        auto repoName = hyRepo->getId();
        add(yamlContent, repoName);
        // update defaults from repo
        try {
            pImpl->moduleMetadata.addMetadataFromString(yamlContent, 0);
        } catch (const ModulePackageContainer::ResolveException & exception) {
            throw ModulePackageContainer::ConflictException(
                tfm::format(_("Conflicting defaults with repo '%s': %s"), repoName,
                            exception.what()));
        }
    }
}

void ModulePackageContainer::addDefaultsFromDisk()
{
    g_autofree gchar * dirPath = g_build_filename(
            pImpl->installRoot.c_str(), "/etc/dnf/modules.defaults.d/", NULL);

    for (const auto &file : filesystem::getDirContent(dirPath)) {
        std::string yamlContent = getFileContent(file);
        pImpl->moduleMetadata.addMetadataFromString(yamlContent, 1000);
    }
}

void ModulePackageContainer::moduleDefaultsResolve()
{
    pImpl->moduleMetadata.resolveAddedMetadata();
    pImpl->moduleDefaults = pImpl->moduleMetadata.getDefaultStreams();
}

void
ModulePackageContainer::add(const std::string &fileContent, const std::string & repoID)
{
    Pool * pool = dnf_sack_get_pool(pImpl->moduleSack);

    ModuleMetadata md;
    md.addMetadataFromString(fileContent, 0);
    md.resolveAddedMetadata();

    LibsolvRepo * r;
    Id id;

    FOR_REPOS(id, r) {
        if (strcmp(r->name, "available") == 0) {
            g_autofree gchar * path = g_build_filename(pImpl->installRoot.c_str(),
                                                      "/etc/dnf/modules.d", NULL);
            std::vector<ModulePackage *> packages = md.getAllModulePackages(pImpl->moduleSack, r, repoID);
            for(auto const& modulePackagePtr: packages) {
                std::unique_ptr<ModulePackage> modulePackage(modulePackagePtr);
                pImpl->modules.insert(std::make_pair(modulePackage->getId(), std::move(modulePackage)));
                pImpl->persistor->insert(modulePackagePtr->getName(), path);
            }

            return;
        }
    }
}

Id
ModulePackageContainer::addPlatformPackage(const std::string& osReleasePath,
    const char* platformModule)
{
    return ModulePackage::createPlatformSolvable(pImpl->moduleSack, osReleasePath,
        pImpl->installRoot, platformModule);
}

Id
ModulePackageContainer::addPlatformPackage(DnfSack * sack,
    const std::vector<std::string> & osReleasePath,
    const char* platformModule)
{
    return ModulePackage::createPlatformSolvable(sack, pImpl->moduleSack, osReleasePath,
        pImpl->installRoot, platformModule);
}

void ModulePackageContainer::createConflictsBetweenStreams()
{
    // TODO Use Query for filtering
    for (const auto &iter : pImpl->modules) {
        const auto &modulePackage = iter.second;

        for (const auto &innerIter : pImpl->modules) {
            if (modulePackage->getName() == innerIter.second->getName()
                && modulePackage->getStream() != innerIter.second->getStream()) {
                modulePackage->addStreamConflict(innerIter.second.get());
            }
        }
    }
}

bool ModulePackageContainer::empty() const noexcept
{
    return pImpl->modules.empty();
}

ModulePackage * ModulePackageContainer::getModulePackage(Id id)
{
    return pImpl->modules.at(id).get();
}

std::vector<ModulePackage *>
ModulePackageContainer::requiresModuleEnablement(const PackageSet & packages)
{
    auto activatedModules = pImpl->activatedModules.get();
    if (!activatedModules) {
        return {};
    }
    std::vector<ModulePackage *> output;
    Query baseQuery(packages.getSack());
    baseQuery.addFilter(HY_PKG, HY_EQ, &packages);
    baseQuery.apply();
    Query testQuery(baseQuery);
    Id moduleId = -1;
    while ((moduleId = activatedModules->next(moduleId)) != -1) {
        auto module = getModulePackage(moduleId);
        if (isEnabled(module)) {
            continue;
        }
        auto includeNEVRAs = module->getArtifacts();
        std::vector<const char *> includeNEVRAsCString(includeNEVRAs.size() + 1);
        transform(includeNEVRAs.begin(), includeNEVRAs.end(), includeNEVRAsCString.begin(),
                  std::mem_fn(&std::string::c_str));
        testQuery.queryUnion(baseQuery);
        testQuery.addFilter(HY_PKG_NEVRA_STRICT, HY_EQ, includeNEVRAsCString.data());
        if (testQuery.empty()) {
            continue;
        }
        output.push_back(module);
    }
    return output;
}


/**
 * @brief Is a ModulePackage part of an enabled stream?
 *
 * @return bool
 */
bool ModulePackageContainer::Impl::isEnabled(const std::string &name, const std::string &stream)
{
    try {
        return persistor->getState(name) == ModuleState::ENABLED &&
            persistor->getStream(name) == stream;
    } catch (NoModuleException &) {
        return false;
    }
}

/**
 * @brief Is a ModulePackage part of an enabled stream?
 *
 * @return bool
 */
bool ModulePackageContainer::isEnabled(const std::string &name, const std::string &stream)
{
    return pImpl->isEnabled(name, stream);
}

bool ModulePackageContainer::isEnabled(const ModulePackage * module)
{
    return pImpl->isEnabled(module->getName(), module->getStream());
}

/**
 * @brief Is a ModulePackage part of a disabled module?
 *
 * @return bool
 */
bool ModulePackageContainer::isDisabled(const std::string &name)
{
    try {
        return pImpl->persistor->getState(name) == ModuleState::DISABLED;
    } catch (NoModuleException &) {
        return false;
    }
}

bool ModulePackageContainer::isDisabled(const ModulePackage * module)
{
    return isDisabled(module->getName());
}

std::vector<std::string> ModulePackageContainer::getDefaultProfiles(std::string moduleName,
    std::string moduleStream)
{
    return pImpl->moduleMetadata.getDefaultProfiles(moduleName, moduleStream);
}

const std::string & ModulePackageContainer::getDefaultStream(const std::string &name) const
{
    auto it = pImpl->moduleDefaults.find(name);
    if (it == pImpl->moduleDefaults.end()) {
        return EMPTY_RESULT;
    }
    return it->second;
}

const std::string & ModulePackageContainer::getEnabledStream(const std::string &name)
{
    return pImpl->persistor->getStream(name);
}

/**
 * @brief Mark ModulePackage as part of an enabled stream.
 */
bool
ModulePackageContainer::enable(const std::string &name, const std::string & stream, const bool count)
{
    if (count) {
        pImpl->persistor->getEntry(name).second.streamChangesNum++;
    }
    bool changed = pImpl->persistor->changeStream(name, stream);
    if (pImpl->persistor->changeState(name, ModuleState::ENABLED)) {
        changed = true;
    }
    if (changed) {
        auto & profiles = pImpl->persistor->getEntry(name).second.profiles;
        profiles.clear();
    }
    return changed;
}

bool
ModulePackageContainer::enable(const ModulePackage * module, const bool count)
{
    return enable(module->getName(), module->getStream(), count);
}

/**
 * @brief Mark module as not part of an enabled stream.
 */
void ModulePackageContainer::disable(const std::string & name, const bool count)
{
    if (count) {
        pImpl->persistor->getEntry(name).second.streamChangesNum++;
    }

    pImpl->persistor->changeState(name, ModuleState::DISABLED);
    pImpl->persistor->changeStream(name, "");
    auto & profiles = pImpl->persistor->getEntry(name).second.profiles;
    profiles.clear();
}

void ModulePackageContainer::disable(const ModulePackage * module, const bool count)
{
    disable(module->getName(), count);
}

/**
 * @brief Reset module state so it's no longer enabled or disabled.
 */
void ModulePackageContainer::reset(const std::string & name, const bool count)
{
    if (count) {
        pImpl->persistor->getEntry(name).second.streamChangesNum++;
    }
    pImpl->persistor->changeState(name, ModuleState::UNKNOWN);

    pImpl->persistor->changeStream(name, "");
    auto & profiles = pImpl->persistor->getEntry(name).second.profiles;
    profiles.clear();
}

void ModulePackageContainer::reset(const ModulePackage * module, const bool count)
{
    reset(module->getName(), count);
}

/**
 * @brief Are there any changes to be saved?
 */
bool ModulePackageContainer::isChanged()
{
    if (!getEnabledStreams().empty()) {
        return true;
    }
    if (!getDisabledModules().empty()) {
        return true;
    }
    if (!getResetModules().empty()) {
        return true;
    }
    if (!getSwitchedStreams().empty()) {
        return true;
    }
    if (!getInstalledProfiles().empty()) {
        return true;
    }
    if (!getRemovedProfiles().empty()) {
        return true;
    }
    return false;
}

void ModulePackageContainer::install(const std::string &name, const std::string &stream,
    const std::string &profile)
{
    for (const auto &iter : pImpl->modules) {
        auto modulePackage = iter.second.get();
        if (modulePackage->getName() == name && modulePackage->getStream() == stream) {
            install(modulePackage, profile);
        }
    }
}

void ModulePackageContainer::install(const ModulePackage * module, const std::string &profile)
{
    if (pImpl->persistor->getStream(module->getName()) == module->getStream())
        pImpl->persistor->addProfile(module->getName(), profile);
}

void ModulePackageContainer::uninstall(const std::string &name, const std::string &stream,
    const std::string &profile)
{
    for (const auto &iter : pImpl->modules) {
        auto modulePackage = iter.second.get();
        if (modulePackage->getName() == name && modulePackage->getStream() == stream) {
            uninstall(modulePackage, profile);
        }
    }
}

void ModulePackageContainer::uninstall(const ModulePackage * module, const std::string &profile)
{
    if (pImpl->persistor->getStream(module->getName()) == module->getStream())
        pImpl->persistor->removeProfile(module->getName(), profile);
}

std::pair<std::vector<std::vector<std::string>>, ModulePackageContainer::ModuleErrorType>
ModulePackageContainer::Impl::moduleSolve(const std::vector<ModulePackage *> & modules,
    bool debugSolver)
{
    if (modules.empty()) {
        activatedModules.reset();
        return std::make_pair(std::vector<std::vector<std::string>>(),
                              ModulePackageContainer::ModuleErrorType::NO_ERROR);
    }
    dnf_sack_recompute_considered(moduleSack);
    dnf_sack_make_provides_ready(moduleSack);
    Goal goal(moduleSack);
    Goal goalWeak(moduleSack);
    for (const auto &module : modules) {
        std::ostringstream ss;
        auto name = module->getName();
        ss << "module(" << name << ":" << module->getStream() << ")";
        Selector selector(moduleSack);
        bool optional = persistor->getState(name) == ModuleState::DEFAULT;
        selector.set(HY_PKG_PROVIDES, HY_EQ, ss.str().c_str());
        goal.install(&selector, optional);
        goalWeak.install(&selector, true);
    }
    auto ret = goal.run(static_cast<DnfGoalActions>(DNF_IGNORE_WEAK | DNF_FORCE_BEST));
    if (debugSolver) {
        goal.writeDebugdata("debugdata/modules");
    }
    std::vector<std::vector<std::string>> problems;
    auto problemType = ModulePackageContainer::ModuleErrorType::NO_ERROR;
    if (ret) {
        problems = goal.describeAllProblemRules(false);
        ret = goal.run(DNF_FORCE_BEST);
        if (ret) {
            // Conflicting modules has to be removed otherwice it could result than one of them will
            // be active
            auto conflictingPkgs = goal.listConflictPkgs(DNF_PACKAGE_STATE_AVAILABLE);
            dnf_sack_add_excludes(moduleSack, conflictingPkgs.get());
            ret = goalWeak.run(DNF_NONE);
            if (ret) {
                auto logger(Log::getLogger());
                logger->critical("Modularity filtering totally broken\n");
                problemType = ModulePackageContainer::ModuleErrorType::CANNOT_RESOLVE_MODULES;
                activatedModules.reset();
            } else {
                problemType = ModulePackageContainer::ModuleErrorType::ERROR;
                Query query(moduleSack, Query::ExcludeFlags::IGNORE_EXCLUDES);
                goal2name_query(goalWeak, query);
                activatedModules.reset(new PackageSet(*query.runSet()));
            }
        } else {
            problemType = ModulePackageContainer::ModuleErrorType::ERROR_IN_DEFAULTS;
            Query query(moduleSack, Query::ExcludeFlags::IGNORE_EXCLUDES);
            goal2name_query(goal, query);
            activatedModules.reset(new PackageSet(*query.runSet()));
        }
    } else {
        Query query(moduleSack, Query::ExcludeFlags::IGNORE_EXCLUDES);
        goal2name_query(goal, query);
        activatedModules.reset(new PackageSet(*query.runSet()));
    }
    return make_pair(problems, problemType);
}

std::vector<ModulePackage *>
ModulePackageContainer::query(Nsvcap& moduleNevra)
{
    return query(moduleNevra.getName(), moduleNevra.getStream(), moduleNevra.getVersion(),
                 moduleNevra.getContext(), moduleNevra.getArch());
}

std::vector<ModulePackage *>
ModulePackageContainer::query(std::string subject)
{
    // Alternatively a search using module provides could be performed
    std::vector<ModulePackage *> result;
    Query query(pImpl->moduleSack, Query::ExcludeFlags::IGNORE_EXCLUDES);
    // platform modules are installed and not in modules std::Map.
    query.available();
    std::ostringstream ss;
    ss << subject << "*";
    query.addFilter(HY_PKG_NAME, HY_GLOB, ss.str().c_str());
    auto pset = query.runSet();
    Id moduleId = -1;
    while ((moduleId = pset->next(moduleId)) != -1) {
        result.push_back(pImpl->modules.at(moduleId).get());
    }
    return result;
}

std::vector<ModulePackage *>
ModulePackageContainer::query(std::string name, std::string stream, std::string version,
    std::string context, std::string arch)
{
    // Alternatively a search using module provides could be performed
    std::vector<ModulePackage *> result;
    Query query(pImpl->moduleSack, Query::ExcludeFlags::IGNORE_EXCLUDES);
    // platform modules are installed and not in modules std::Map.
    query.available();
    std::ostringstream ss;
    ss << stringFormater(name) << ":" << stringFormater(stream);
    ss << ":" << stringFormater(context);
    query.addFilter(HY_PKG_NAME, HY_GLOB, ss.str().c_str());
    if (!arch.empty()) {
        query.addFilter(HY_PKG_ARCH, HY_GLOB, arch.c_str());
    }
    if (!version.empty()) {
        query.addFilter(HY_PKG_VERSION, HY_GLOB, version.c_str());
    }
    auto pset = query.runSet();
    Id moduleId = -1;
    while ((moduleId = pset->next(moduleId)) != -1) {
        result.push_back(pImpl->modules.at(moduleId).get());
    }
    return result;
}

void ModulePackageContainer::enableDependencyTree(std::vector<ModulePackage *> & modulePackages)
{
    if (!pImpl->activatedModules) {
        return;
    }
    PackageSet toEnable(pImpl->moduleSack);
    PackageSet enabled(pImpl->moduleSack);
    for (auto & modulePackage: modulePackages) {
        if (!isModuleActive(modulePackage)) {
            continue;
        }
        Query query(pImpl->moduleSack);
        query.addFilter(HY_PKG, HY_EQ, pImpl->activatedModules.get());
        auto pkg = dnf_package_new(pImpl->moduleSack, modulePackage->getId());
        auto requires = dnf_package_get_requires(pkg);
        query.addFilter(HY_PKG_PROVIDES, requires);
        auto set = query.runSet();
        toEnable += *set;
        delete requires;
        g_object_unref(pkg);
        enable(modulePackage);
        enabled.set(modulePackage->getId());
    }
    toEnable -= enabled;
    while (!toEnable.empty()) {
        Id moduleId = -1;
        while ((moduleId = toEnable.next(moduleId)) != -1) {
            enable(pImpl->modules.at(moduleId).get());
            enabled.set(moduleId);
            Query query(pImpl->moduleSack);
            query.addFilter(HY_PKG, HY_EQ, pImpl->activatedModules.get());
            query.addFilter(HY_PKG, HY_NEQ, &enabled);
            auto pkg = dnf_package_new(pImpl->moduleSack, moduleId);
            auto requires = dnf_package_get_requires(pkg);
            query.addFilter(HY_PKG_PROVIDES, requires);
            auto set = query.runSet();
            toEnable += *set;
            delete requires;
            g_object_unref(pkg);
        }
        toEnable -= enabled;
    }
}

ModulePackageContainer::ModuleState
ModulePackageContainer::getModuleState(const std::string& name)
{
    try {
        return pImpl->persistor->getState(name);
    } catch (NoModuleException &) {
        return ModuleState::UNKNOWN;
    }
}

std::set<std::string> ModulePackageContainer::getInstalledPkgNames()
{
    auto moduleNames = pImpl->persistor->getAllModuleNames();
    std::set<std::string> pkgNames;
    for (auto & moduleName: moduleNames) {
        auto stream = getEnabledStream(moduleName);
        if (stream.empty()) {
            continue;
        }
        auto profilesInstalled = getInstalledProfiles(moduleName);
        if (profilesInstalled.empty()) {
            continue;
        }
        std::string nameStream(moduleName);
        nameStream += ":";
        nameStream += stream;
        auto modules = query(nameStream);
        const ModulePackage * latest = nullptr;
        for (const ModulePackage * module: modules) {
            if (isModuleActive(module->getId())) {
                if (!latest) {
                    latest = module;
                } else {
                    if (module->getVersion() > latest->getVersion()) {
                        latest = module;
                    }
                }
            }
        }
        if (!latest) {
            for (auto module: modules) {
                if (!latest) {
                    latest = module;
                } else {
                    if (module->getVersion() > latest->getVersion()) {
                        latest = module;
                    }
                }
            }
        }
        if (!latest) {
            continue;
        }
        for (auto & profile: profilesInstalled) {
            auto profiles = latest->getProfiles(profile);
            for (auto & profile: profiles) {
                auto pkgs = profile.getContent();
                for (auto pkg: pkgs) {
                    pkgNames.insert(pkg);
                }
            }
        }
    }
    return pkgNames;
}

std::string
ModulePackageContainer::getReport()
{
    std::string report;

    auto installedProfiles = getInstalledProfiles();
    if (!installedProfiles.empty()) {
        report += _("Installing module profiles:\n");
        for (auto & item: installedProfiles) {
            for (auto & profile:item.second) {
                report += "    ";
                report += item.first;
                report += ":";
                report += profile;
                report += "\n";
            }
        }
        report += "\n";
    }

    auto removedProfiles = getRemovedProfiles();
    if (!removedProfiles.empty()) {
        report += _("Disabling module profiles:\n");
        for (auto & item: removedProfiles) {
            for (auto & profile:item.second) {
                report += "    ";
                report += item.first;
                report += ":";
                report += profile;
                report += "\n";
            }
        }
        report += "\n";
    }

    auto enabled = getEnabledStreams();
    if (!enabled.empty()) {
        report += _("Enabling module streams:\n");
        for (auto & item: enabled) {
            report += "    ";
            report += item.first;
            report += ":";
            report += item.second;
            report += "\n";
        }
        report += "\n";
    }

    auto switchedStreams = getSwitchedStreams();
    if (!switchedStreams.empty()) {
        std::string switchedReport;
        switchedReport += _("Switching module streams:\n");
        for (auto & item: switchedStreams) {
            switchedReport += "    ";
            switchedReport += item.first;
            switchedReport += ":";
            switchedReport += item.second.first;
            switchedReport += " > ";
            switchedReport += item.first;
            switchedReport += ":";
            switchedReport += item.second.second;
            switchedReport += "\n";
        }
        report += switchedReport;
        report += "\n";
    }

    auto disabled = getDisabledModules();
    if (!disabled.empty()) {
        report += _("Disabling modules:\n");
        for (auto & name: disabled) {
            report += "    ";
            report += name;
            report += "\n";
        }
        report += "\n";
    }

    auto reset = getResetModules();
    if (!reset.empty()) {
        report += _("Resetting modules:\n");
        for (auto & name: reset) {
            report += "    ";
            report += name;
            report += "\n";
        }
        report += "\n";
    }
    return report;
}

static bool
modulePackageLatestPerRepoSorter(DnfSack * sack, const ModulePackage * first, const ModulePackage * second)
{
    if (first->getRepoID() != second->getRepoID())
        return first->getRepoID() < second->getRepoID();
    int cmp = g_strcmp0(first->getNameCStr(), second->getNameCStr());
    if (cmp != 0)
        return cmp < 0;
    cmp = dnf_sack_evr_cmp(sack, first->getStreamCStr(), second->getStreamCStr());
    if (cmp != 0)
        return cmp < 0;
    cmp = g_strcmp0(first->getArchCStr(), second->getArchCStr());
    if (cmp != 0)
        return cmp < 0;
    return first->getVersionNum() > second->getVersionNum();
}

static bool
modulePackageLatestSorter(DnfSack * sack, const ModulePackage * first, const ModulePackage * second)
{
    int cmp = g_strcmp0(first->getNameCStr(), second->getNameCStr());
    if (cmp != 0)
        return cmp < 0;
    cmp = dnf_sack_evr_cmp(sack, first->getStreamCStr(), second->getStreamCStr());
    if (cmp != 0)
        return cmp < 0;
    cmp = g_strcmp0(first->getArchCStr(), second->getArchCStr());
    if (cmp != 0)
        return cmp < 0;
    return first->getVersionNum() > second->getVersionNum();
}

std::vector<std::vector<std::vector<ModulePackage *>>>
ModulePackageContainer::getLatestModulesPerRepo(ModuleState moduleFilter,
    std::vector<ModulePackage *> modulePackages)
{
    if (modulePackages.empty()) {
        return {};
    }
    if (moduleFilter == ModuleState::ENABLED) {
        std::vector<ModulePackage *> enabled;
        for (auto package: modulePackages) {
            if (isEnabled(package)) {
                enabled.push_back(package);
            }
        }
        modulePackages = enabled;
    } else if (moduleFilter == ModuleState::DISABLED) {
        std::vector<ModulePackage *> disabled;
        for (auto package: modulePackages) {
            if (isDisabled(package)) {
                disabled.push_back(package);
            }
        }
        modulePackages = disabled;
    } else if (moduleFilter == ModuleState::INSTALLED) {
        std::vector<ModulePackage *> installed;
        for (auto package: modulePackages) {
            if ((!getInstalledProfiles(package->getName()).empty()) && isEnabled(package)) {
                installed.push_back(package);
            }
        }
        modulePackages = installed;
    }
    if (modulePackages.empty()) {
        return {};
    }
    auto & packageFirst = modulePackages[0];
    std::vector<std::vector<std::vector<ModulePackage *>>> output;
    auto sack = pImpl->moduleSack;
    std::sort(modulePackages.begin(), modulePackages.end(),
              [sack](const ModulePackage * first, const ModulePackage * second)
              {return modulePackageLatestPerRepoSorter(sack, first, second);});
    auto vectorSize = modulePackages.size();
    output.push_back(
        std::vector<std::vector<ModulePackage *>>{std::vector<ModulePackage *> {packageFirst}});
    int repoIndex = 0;
    int nameStreamArchIndex = 0;
    auto repoID = packageFirst->getRepoID();
    auto name = packageFirst->getNameCStr();
    auto stream = packageFirst->getStreamCStr();
    auto arch = packageFirst->getArchCStr();
    auto version = packageFirst->getVersionNum();

    for (unsigned int index = 1; index < vectorSize; ++index) {
        auto & package = modulePackages[index];
        if (repoID != package->getRepoID()) {
            repoID = package->getRepoID();
            name = package->getNameCStr();
            stream = package->getStreamCStr();
            arch = package->getArchCStr();
            version = package->getVersionNum();
            output.push_back(std::vector<std::vector<ModulePackage *>>{std::vector<ModulePackage *> {package}});
            ++repoIndex;
            nameStreamArchIndex = 0;
            continue;
        }
        if (g_strcmp0(package->getNameCStr(), name) != 0 ||
            g_strcmp0(package->getStreamCStr(), stream) != 0 ||
            g_strcmp0(package->getArchCStr(), arch) != 0) {
            name = package->getNameCStr();
            stream = package->getStreamCStr();
            arch = package->getArchCStr();
            version = package->getVersionNum();
            output[repoIndex].push_back(std::vector<ModulePackage *> {package});
            ++nameStreamArchIndex;
            continue;
        }
        if (version == package->getVersionNum()) {
            output[repoIndex][nameStreamArchIndex].push_back(package);
        }
    }
    return output;
}


std::pair<std::vector<std::vector<std::string>>, ModulePackageContainer::ModuleErrorType>
ModulePackageContainer::resolveActiveModulePackages(bool debugSolver)
{
    dnf_sack_reset_excludes(pImpl->moduleSack);
    std::vector<ModulePackage *> packages;

    PackageSet excludes(pImpl->moduleSack);
    // Use only Enabled or Default modules for transaction
    for (const auto &iter : pImpl->modules) {
        auto module = iter.second.get();
        auto moduleState = pImpl->persistor->getState(module->getName());
        if (moduleState == ModuleState::DISABLED) {
            excludes.set(module->getId());
            continue;
        }

        bool hasDefaultStream;
        hasDefaultStream = getDefaultStream(module->getName()) == module->getStream();
        if (isDisabled(module)) {
            // skip disabled modules
            continue;
        } else if (isEnabled(module)) {
            packages.push_back(module);
        } else if (hasDefaultStream) {
            if (moduleState != ModuleState::ENABLED) {
                pImpl->persistor->changeState(module->getName(), ModuleState::DEFAULT);
                packages.push_back(module);
            }
        }
    }
    dnf_sack_add_excludes(pImpl->moduleSack, &excludes);
    auto problems = pImpl->moduleSolve(packages, debugSolver);
    return problems;
}

bool ModulePackageContainer::isModuleActive(Id id)
{
    if (pImpl->activatedModules) {
        return pImpl->activatedModules->has(id);
    }
    return false;
}

bool ModulePackageContainer::isModuleActive(const ModulePackage * modulePackage)
{
    if (pImpl->activatedModules) {
        return pImpl->activatedModules->has(modulePackage->getId());
    }
    return false;
}

std::vector<ModulePackage *> ModulePackageContainer::getModulePackages()
{
    std::vector<ModulePackage *> values;
    const auto & modules = pImpl->modules;
    std::transform(
        std::begin(modules), std::end(modules), std::back_inserter(values),
        [](const std::map<Id, std::unique_ptr<ModulePackage>>::value_type & pair){ return pair.second.get(); });

    return values;
}

void ModulePackageContainer::save()
{
    pImpl->persistor->save(pImpl->installRoot, "/etc/dnf/modules.d");
}

void ModulePackageContainer::rollback()
{
    pImpl->persistor->rollback();
}

std::map<std::string, std::string> ModulePackageContainer::getEnabledStreams()
{
    return pImpl->persistor->getEnabledStreams();
}

std::vector<std::string> ModulePackageContainer::getDisabledModules()
{
    return pImpl->persistor->getDisabledModules();
}

std::map<std::string, std::string> ModulePackageContainer::getDisabledStreams()
{
    return pImpl->persistor->getDisabledStreams();
}

std::vector<std::string> ModulePackageContainer::getResetModules()
{
    return pImpl->persistor->getResetModules();
}

std::map<std::string, std::string> ModulePackageContainer::getResetStreams()
{
    return pImpl->persistor->getResetStreams();
}

std::map<std::string, std::pair<std::string, std::string>>
ModulePackageContainer::getSwitchedStreams()
{
    return pImpl->persistor->getSwitchedStreams();
}

std::map<std::string, std::vector<std::string>> ModulePackageContainer::getInstalledProfiles()
{
    return pImpl->persistor->getInstalledProfiles();
}

std::vector<std::string> ModulePackageContainer::getInstalledProfiles(std::string moduleName)
{
    return pImpl->persistor->getProfiles(moduleName);
}

std::map<std::string, std::vector<std::string>> ModulePackageContainer::getRemovedProfiles()
{
    return pImpl->persistor->getRemovedProfiles();
}
const std::string &
ModulePackageContainer::Impl::ModulePersistor::getStream(const std::string & name)
{
    return getEntry(name).second.stream;
}

inline std::pair<ConfigParser, struct ModulePackageContainer::Impl::ModulePersistor::Config> &
ModulePackageContainer::Impl::ModulePersistor::getEntry(const std::string & moduleName)
{
    try {
        auto & entry = configs.at(moduleName);
        return entry;
    } catch (std::out_of_range &) {
        throw NoModuleException(moduleName);
    }
}

std::vector<std::string> ModulePackageContainer::Impl::ModulePersistor::getAllModuleNames()
{
    std::vector<std::string> output;
    output.reserve(configs.size());
    for (auto & item: configs) {
        output.push_back(item.first);
    }
    return output;
}

bool
ModulePackageContainer::Impl::ModulePersistor::changeStream(const std::string &name,
    const std::string &stream)
{
    const auto &updatedValue = configs.at(name).second.stream;
    if (updatedValue == stream)
        return false;
    const auto &originValue = configs.at(name).first.getValue(name, "stream");
    if (originValue != updatedValue && configs.at(name).second.streamChangesNum > 1) {
        throw EnableMultipleStreamsException(name);
    }
    getEntry(name).second.stream = stream;
    return true;
}

const std::vector<std::string> &
ModulePackageContainer::Impl::ModulePersistor::getProfiles(const std::string &name)
{
    return getEntry(name).second.profiles;
}

bool ModulePackageContainer::Impl::ModulePersistor::addProfile(
    const std::string &name, const std::string &profile)
{
    auto & profiles = getEntry(name).second.profiles;
    const auto &it = std::find(std::begin(profiles), std::end(profiles), profile);
    if (it != std::end(profiles))
        return false;

    profiles.push_back(profile);
    return true;
}

bool ModulePackageContainer::Impl::ModulePersistor::removeProfile(
    const std::string &name, const std::string &profile)
{
    auto &profiles = getEntry(name).second.profiles;

    for (auto it = profiles.begin(); it != profiles.end(); it++) {
        if (*it == profile) {
            profiles.erase(it);
            return true;
        }
    }

    return false;
}

const ModulePackageContainer::ModuleState &
ModulePackageContainer::Impl::ModulePersistor::getState(const std::string &name)
{
    return getEntry(name).second.state;
}

bool ModulePackageContainer::Impl::ModulePersistor::changeState(
    const std::string &name, ModuleState state)
{
    if (getEntry(name).second.state == state)
        return false;

    getEntry(name).second.state = state;
    return true;
}

static inline void
initConfig(ConfigParser & parser, const std::string & name)
{
    parser.addSection(name);
    parser.setValue(name, "name", name);
    parser.setValue(name, "stream", EMPTY_STREAM);
    parser.setValue(name, "profiles", EMPTY_PROFILES);
    parser.setValue(name, "state", DEFAULT_STATE);
}

static inline void
parseConfig(ConfigParser &parser, const std::string &name, const char *path)
{
    auto logger(Log::getLogger());

    try {
        const auto fname = name + ".module";
        g_autofree gchar * cfn = g_build_filename(path, fname.c_str(), NULL);
        parser.read(cfn);

        /* FIXME: init empty config or throw error? */
        if (!parser.hasOption(name, "stream") ||    /* stream = <stream_name> */
            !parser.hasOption(name, "profiles") ||  /* profiles = <list of profiles> */
            (!parser.hasOption(name, "state") && !parser.hasOption(name, "enabled"))) {
            logger->debug("Invalid config file for " + name);
            initConfig(parser, name);
            return;
        }

        /* Old config files might not have an option 'name' */
        parser.setValue(name, "name", name);

        /* Replace old 'enabled' format by 'state' */
        if (parser.hasOption(name, "enabled")) {
            parser.setValue(name, "state", parser.getValue(name, "enabled"));
            parser.removeOption(name, "enabled");
        }
    } catch (const ConfigParser::CantOpenFile &) {
        /* No module config file present. Fill values in */
        initConfig(parser, name);
        return;
    }
}

bool ModulePackageContainer::Impl::ModulePersistor::insert(
    const std::string &moduleName, const char *path)
{
    /* There can only be one config file per module */
    if (configs.find(moduleName) != configs.end()) {
        return false;
    }

    auto newEntry = configs.emplace(moduleName, std::make_pair(ConfigParser{}, Config()));
    auto & parser = newEntry.first->second.first;
    auto & newConfig = newEntry.first->second.second;

    parseConfig(parser, moduleName, path);

    OptionStringList slist{std::vector<std::string>()};
    const auto &plist = parser.getValue(moduleName, "profiles");
    newConfig.profiles = std::move(slist.fromString(plist));

    newConfig.state = fromString(parser.getValue(moduleName, "state"));
    newConfig.stream = parser.getValue(moduleName, "stream");
    newConfig.streamChangesNum = 0;

    return true;
}

bool ModulePackageContainer::Impl::ModulePersistor::update(const std::string & name)
{
    bool changed = false;
    auto & parser = getEntry(name).first;

    const auto & state = toString(getState(name));
    if (!parser.hasOption(name, "state") || parser.getValue(name, "state") != state) {
        parser.setValue(name, "state", state);
        changed = true;
    }

    const auto & stream = getStream(name);
    if (!parser.hasOption(name, "stream") || parser.getValue(name, "stream") != stream) {
        parser.setValue(name, "stream", stream);
        changed = true;
    }

    OptionStringList profiles{getProfiles(name)};
    if (!parser.hasOption(name, "profiles") ||
        OptionStringList(parser.getValue(name, "profiles")).getValue() != profiles.getValue()) {
        parser.setValue(name, "profiles", profiles.getValueString());
        changed = true;
    }

    return changed;
}

void ModulePackageContainer::Impl::ModulePersistor::reset(const std::string & name)
{
    auto & entry = getEntry(name);
    auto & parser = entry.first;

    entry.second.stream = parser.getValue(name, "stream");
    entry.second.state = fromString(parser.getValue(name, "state"));
    OptionStringList slist{std::vector<std::string>()};
    entry.second.profiles = slist.fromString(parser.getValue(name, "profiles"));
}

void ModulePackageContainer::Impl::ModulePersistor::save(
    const std::string &installRoot, const std::string &modulesPath)
{
    g_autofree gchar * dirname = g_build_filename(
        installRoot.c_str(), modulesPath.c_str(), "/", NULL);
    makeDirPath(std::string(dirname));

    for (auto &iter : configs) {
        const auto &name = iter.first;

        if (update(name)) {
            g_autofree gchar * fname = g_build_filename(installRoot.c_str(),
                    modulesPath.c_str(), (name + ".module").c_str(), NULL);
            iter.second.first.write(std::string(fname), false);
        }
    }
}

void ModulePackageContainer::Impl::ModulePersistor::rollback(void)
{
    for (auto &iter : configs) {
        const auto &name = iter.first;
        reset(name);
    }
}

std::map<std::string, std::string>
ModulePackageContainer::Impl::ModulePersistor::getEnabledStreams()
{
    std::map<std::string, std::string> enabled;

    for (const auto &it : configs) {
        const auto &name = it.first;
        const auto &newVal = it.second.second.state;
        const auto &oldVal = fromString(it.second.first.getValue(name, "state"));

        if (oldVal != ModuleState::ENABLED && newVal == ModuleState::ENABLED) {
            enabled.emplace(name, it.second.second.stream);
        }
    }

    return enabled;
}

std::vector<std::string>
ModulePackageContainer::Impl::ModulePersistor::getDisabledModules()
{
    std::vector<std::string> disabled;

    for (const auto & it : configs) {
        const auto & name = it.first;
        const auto & newVal = it.second.second.state;
        const auto & oldVal = fromString(it.second.first.getValue(name, "state"));
        if (oldVal != ModuleState::DISABLED && newVal == ModuleState::DISABLED) {
            disabled.emplace_back(name);
        }
    }

    return disabled;
}

std::map<std::string, std::string>
ModulePackageContainer::Impl::ModulePersistor::getDisabledStreams()
{
    std::map<std::string, std::string> disabled;

    for (const auto &it : configs) {
        const auto &name = it.first;
        const auto &newVal = it.second.second.state;
        const auto &oldVal = fromString(it.second.first.getValue(name, "state"));
        if (oldVal != ModuleState::DISABLED && newVal == ModuleState::DISABLED) {
            disabled.emplace(name, it.second.first.getValue(name, "stream"));
        }
    }

    return disabled;
}


std::vector<std::string>
ModulePackageContainer::Impl::ModulePersistor::getResetModules()
{
    std::vector<std::string> result;

    for (const auto & it : configs) {
        const auto & name = it.first;
        const auto & newVal = it.second.second.state;
        const auto & oldVal = fromString(it.second.first.getValue(name, "state"));
        // when resetting module state, UNKNOWN and DEFAULT are treated equally,
        // because they are both represented as 'state=' in the config file
        // and the only difference is internal state based on module defaults
        if (oldVal == ModuleState::UNKNOWN || oldVal == ModuleState::DEFAULT) {
            continue;
        }
        if (newVal == ModuleState::UNKNOWN || newVal == ModuleState::DEFAULT) {
            result.emplace_back(name);
        }
    }

    return result;
}

std::map<std::string, std::string>
ModulePackageContainer::Impl::ModulePersistor::getResetStreams()
{
    std::map<std::string, std::string> result;

    for (const auto &it : configs) {
        const auto &name = it.first;
        const auto &newVal = it.second.second.state;
        const auto &oldVal = fromString(it.second.first.getValue(name, "state"));
        // when resetting module state, UNKNOWN and DEFAULT are treated equally,
        // because they are both represented as 'state=' in the config file
        // and the only difference is internal state based on module defaults
        if (oldVal == ModuleState::UNKNOWN || oldVal == ModuleState::DEFAULT) {
            continue;
        }
        if (newVal == ModuleState::UNKNOWN || newVal == ModuleState::DEFAULT) {
            result.emplace(name, it.second.first.getValue(name, "stream"));
        }
    }

    return result;
}

std::map<std::string, std::pair<std::string, std::string>>
ModulePackageContainer::Impl::ModulePersistor::getSwitchedStreams()
{
    std::map<std::string, std::pair<std::string, std::string>> switched;

    for (const auto &it : configs) {
        const auto &name = it.first;
        const auto &oldVal = it.second.first.getValue(name, "stream");
        const auto &newVal = it.second.second.stream;
        // Do not report enabled stream as switched
        if (oldVal.empty()) {
            continue;
        }
        // Do not report disabled stream as switched
        if (newVal.empty()) {
            continue;
        }
        if (oldVal != newVal) {
            switched.emplace(name, std::make_pair(oldVal, newVal));
        }
    }

    return switched;
}

std::map<std::string, std::vector<std::string>>
ModulePackageContainer::Impl::ModulePersistor::getInstalledProfiles()
{
    std::map<std::string, std::vector<std::string>> profiles;
    for (auto & it : configs) {
        OptionStringList slist{std::vector<std::string>()};
        const auto & name = it.first;
        const auto & parser = it.second.first;
        auto & newProfiles = it.second.second.profiles;

        auto vprof = slist.fromString(parser.getValue(name, "profiles"));
        std::sort(vprof.begin(), vprof.end());
        std::sort(newProfiles.begin(), newProfiles.end());
        std::vector<std::string> profDiff;
        std::set_difference(newProfiles.begin(), newProfiles.end(),
                            vprof.begin(), vprof.end(),
                            std::back_inserter(profDiff));

        if (profDiff.size() > 0) {
            profiles.emplace(name, std::move(profDiff));
        }
    }

    return profiles;
}

std::map<std::string, std::vector<std::string>>
ModulePackageContainer::Impl::ModulePersistor::getRemovedProfiles()
{
    std::map<std::string, std::vector<std::string>> profiles;

    for (auto & it : configs) {
        OptionStringList slist{std::vector<std::string>()};
        const auto & name = it.first;
        const auto & parser = it.second.first;
        auto & newProfiles = it.second.second.profiles;

        auto vprof = slist.fromString(parser.getValue(name, "profiles"));
        std::sort(vprof.begin(), vprof.end());
        std::sort(newProfiles.begin(), newProfiles.end());
        std::vector<std::string> profDiff;
        std::set_difference(vprof.begin(), vprof.end(),
                            newProfiles.begin(), newProfiles.end(),
                            std::back_inserter(profDiff));
        if (profDiff.size() > 0) {
            profiles.emplace(name, std::move(profDiff));
        }
    }

    return profiles;
}

void ModulePackageContainer::loadFailSafeData()
{
    auto persistor = pImpl->persistor->configs;
    
    std::map<std::string, std::pair<std::string, bool>> enabledStreams;
    for (auto & nameConfig: persistor) {
        if (nameConfig.second.second.state == ModuleState::ENABLED) {
            auto & stream = nameConfig.second.second.stream;
            if (!stream.empty()) {
                enabledStreams.emplace(nameConfig.first, std::make_pair(stream, false));
            }
        }
    }
    for (auto & modulePair: pImpl->modules) {
        auto module = modulePair.second.get();
        auto it = enabledStreams.find(module->getName());
        if (it != enabledStreams.end() && it->second.first == module->getStream()) {
            it->second.second = true;
        }
    }
    auto fileNames = getYamlFilenames(pImpl->persistDir.c_str());
    auto begin = fileNames.begin();
    auto end = fileNames.end();
    for (auto & pair: enabledStreams) {
        if (!pair.second.second) {
            // load from disk
            std::ostringstream ss;
            ss << pair.first << ":" << pair.second.first << ":";
            bool loaded = false;
            auto searchPrefix = ss.str();
            auto low = std::lower_bound(begin, end, searchPrefix, stringStartWithLowerComparator);
            for (; low != end && string::startsWith((*low), searchPrefix); ++low) {
                g_autofree gchar * file = g_build_filename(
                    pImpl->persistDir.c_str(), low->c_str(), NULL);
                try {
                    auto yamlContent = getFileContent(file);
                    add(yamlContent, LIBDNF_MODULE_FAIL_SAFE_REPO_NAME);
                    loaded = true;
                } catch (const std::exception &) {
                    auto logger(Log::getLogger());
                    logger->debug(tfm::format(
                        _("Unable to load modular Fail-Safe data at '%s'"), file));
                }
            }
            if (!loaded) {
                auto logger(Log::getLogger());
                logger->debug(tfm::format(
                            _("Unable to load modular Fail-Safe data for module '%s:%s'"),
                                          pair.first, pair.second.first));
            }
        }
    }
}

std::vector<ModulePackage *> ModulePackageContainer::Impl::getLatestActiveEnabledModules()
{
    std::vector<ModulePackage *> activeModules;
    Id moduleId = -1;
    while ((moduleId = activatedModules->next(moduleId)) != -1) {
        auto modulePackage = modules.at(moduleId).get();
        if (isEnabled(modulePackage->getName(), modulePackage->getStream())) {
            activeModules.push_back(modulePackage);
        }
    }
    if (activeModules.empty()) {
        return {};
    }
    auto sack = moduleSack;
    std::sort(activeModules.begin(), activeModules.end(),
              [sack](const ModulePackage * first, const ModulePackage * second)
              {return modulePackageLatestSorter(sack, first, second);});

    auto packageFirst = activeModules[0];
    std::vector<ModulePackage *> latest;
    auto vectorSize = activeModules.size();
    latest.push_back(packageFirst);
    auto name = packageFirst->getNameCStr();
    auto stream = packageFirst->getStreamCStr();
    auto arch = packageFirst->getArchCStr();

    for (unsigned int index = 1; index < vectorSize; ++index) {
        auto & package = activeModules[index];
        if (g_strcmp0(package->getNameCStr(), name) != 0 ||
            g_strcmp0(package->getStreamCStr(), stream) != 0 ||
            g_strcmp0(package->getArchCStr(), arch) != 0) {
            name = package->getNameCStr();
            stream = package->getStreamCStr();
            arch = package->getArchCStr();
            latest.push_back(package);
        }
    }
    return latest;
}

void ModulePackageContainer::updateFailSafeData()
{
    auto fileNames = getYamlFilenames(pImpl->persistDir.c_str());

    if (pImpl->activatedModules) {
        std::vector<ModulePackage *> latest = pImpl->getLatestActiveEnabledModules();

        auto begin = fileNames.begin();
        auto end = fileNames.end();
        if (g_mkdir_with_parents(pImpl->persistDir.c_str(), 0755) == -1) {
            const char * errTxt = strerror(errno);
            auto logger(Log::getLogger());
            logger->debug(tfm::format(
                _("Unable to create directory \"%s\" for modular Fail Safe data: %s"),
                                      pImpl->persistDir.c_str(), errTxt));
        }

        // Update FailSafe data
        for (auto modulePackage: latest) {
            std::ostringstream ss;
            ss << modulePackage->getNameStream();
            ss << ":" << modulePackage->getArch() << ".yaml";
            auto fileName = ss.str();
            if (modulePackage->getRepoID() == LIBDNF_MODULE_FAIL_SAFE_REPO_NAME) {
                continue;
            }
            g_autofree gchar * filePath = g_build_filename(pImpl->persistDir.c_str(), fileName.c_str(), NULL);
            if (!updateFile(filePath, modulePackage->getYaml().c_str())) {
                auto logger(Log::getLogger());
                logger->debug(tfm::format(_("Unable to save a modular Fail Safe data to '%s'"), filePath));
            }
        }
    }

    // Remove files from not enabled modules
    for (unsigned int index = 0; index < fileNames.size(); ++index) {
        auto fileName = fileNames[index];
        auto first = fileName.find(":");
        if (first == std::string::npos || first == 0) {
            continue;
        }
        std::string moduleName = fileName.substr(0, first);
        auto second = fileName.find(":", ++first);
        if (second == std::string::npos || first == second) {
            continue;
        }
        std::string moduleStream = fileName.substr(first, second - first);

        if (!isEnabled(moduleName, moduleStream)) {
            g_autofree gchar * file = g_build_filename(pImpl->persistDir.c_str(), fileNames[index].c_str(), NULL);
            if (remove(file)) {
                auto logger(Log::getLogger());
                logger->debug(tfm::format(_("Unable to remove a modular Fail Safe data in '%s'"), file));
            }
        }
    }
}

void ModulePackageContainer::applyObsoletes(){
    for (const auto &iter : pImpl->modules) {
        auto modulePkg = iter.second.get();
        if (!isEnabled(modulePkg)) {
            continue;
        }
        /* We cannot access the eol through the ModulemdModuleStream which is
         * wrapped in modulePkg because it was created with metedata from only
         * one repository but other repositories can have new eols which affect
         * this module stream. We have to use merged modular metadata from all
         * available repositories.
         */
        ModulemdObsoletes *modulePkgObsoletes = pImpl->moduleMetadata.getNewestActiveObsolete(modulePkg);
        if (modulePkgObsoletes) {
            auto moduleName = modulemd_obsoletes_get_obsoleted_by_module_name(modulePkgObsoletes);
            auto moduleStream = modulemd_obsoletes_get_obsoleted_by_module_stream(modulePkgObsoletes);

            if (moduleName && moduleStream) {
                if (!isDisabled(moduleName)) {
                    enable(moduleName, moduleStream, false);
                    if (std::string(moduleName) != modulePkg->getName()) {
                        reset(modulePkg, false);
                    }
                } else {
                    auto logger(Log::getLogger());
                    logger->debug(tfm::format(
                        _("Unable to apply modular obsoletes to '%s:%s' because target module '%s' is disabled"),
                        modulePkg->getName(), modulePkg->getStream(), moduleName));
                }
            } else {
                reset(modulePkg, false);
            }
        }
    }
}

}
