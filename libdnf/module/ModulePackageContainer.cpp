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
#include "libdnf/utils/utils.hpp"
#include "libdnf/utils/File.hpp"
#include "libdnf/dnf-sack-private.hpp"
#include <functional>
#include <../sack/query.hpp>
#include "libdnf/conf/ConfigParser.hpp"
#include "libdnf/conf/OptionStringList.hpp"

static std::string
stringFormater(std::string imput)
{
    return imput.empty() ? "*" : imput;
}

enum class ModuleState {
    UNKNOWN,
    ENABLED,
    DISABLED,
    DEFAULT
};

static ModuleState
fromString(const std::string &str) {
    if (str == "" || str == "default")
        return ModuleState::DEFAULT;
    if (str == "1" || str == "true" || str == "enabled")
        return ModuleState::ENABLED;
    if (str == "0" || str == "false" || str == "disabled")
        return ModuleState::DISABLED;

    return ModuleState::UNKNOWN;
}

static std::string
toString(const ModuleState &state) {
    switch (state) {
        case ModuleState::ENABLED:
            return "enabled";
        case ModuleState::DISABLED:
            return "disabled";
        case ModuleState::DEFAULT:
            return "";
        default:
            return "unknown";
    }
}

class ModulePackageContainer::Impl {
public:
    Impl();
    ~Impl();
    std::unique_ptr<libdnf::IdQueue> moduleSolve(
        const std::vector<ModulePackagePtr> & modules);
    bool insert(const std::string &moduleName, const char *path);


private:
    friend struct ModulePackageContainer;
    class ModulePersistor;
    std::unique_ptr<ModulePersistor> persistor;
    std::map<Id, ModulePackagePtr> modules;
    DnfSack * moduleSack;
    Map * activatedModules{nullptr};
    std::string installRoot;
    std::map<std::string, std::string> moduleDefaults;
};

class ModulePackageContainer::Impl::ModulePersistor {
public:
    ~ModulePersistor() = default;

    const std::string & getStream(const std::string &name);
    const std::vector<std::string> & getProfiles(const std::string &name);
    const ModuleState & getState(const std::string &name);

    std::map<std::string, std::string> getEnabledStreams();
    std::map<std::string, std::string> getDisabledStreams();
    std::map<std::string, std::pair<std::string, std::string>> getSwitchedStreams();
    std::map<std::string, std::vector<std::string>> getInstalledProfiles();
    std::map<std::string, std::vector<std::string>> getRemovedProfiles();

    bool changeStream(const std::string &name, const std::string &stream);
    bool addProfile(const std::string &name, const std::string &profile);
    bool removeProfile(const std::string &name, const std::string &profile);
    bool changeState(const std::string &name, ModuleState state);

    bool insert(const std::string &moduleName, const char *path);
    void rollback();
    void save(const std::string &installRoot, const std::string &modulesPath);

private:
    friend class Impl;
    bool update(const std::string &name);
    void reset(const std::string &name);

    struct Config {
        std::string stream;
        std::vector<std::string> profiles;
        ModuleState state;
        bool locked;
    };
    std::map<std::string, std::pair<libdnf::ConfigParser, struct Config>> configs;
};

ModulePackageContainer::ModulePackageContainer(bool allArch, std::string installRoot,
    const char * arch) : pImpl(new Impl)
{
    pImpl->moduleSack = dnf_sack_new();
    if (allArch) {
        dnf_sack_set_all_arch(pImpl->moduleSack, TRUE);
    } else {
        dnf_sack_set_arch(pImpl->moduleSack, arch, NULL);
    }
    Pool * pool = dnf_sack_get_pool(pImpl->moduleSack);
    HyRepo hrepo = hy_repo_create("available");
    Repo *repo = repo_create(pool, "available");
    repo->appdata = hrepo;
    hrepo->libsolv_repo = repo;
    hrepo->needs_internalizing = 1;
    pImpl->installRoot = installRoot;
}

ModulePackageContainer::~ModulePackageContainer() = default;

ModulePackageContainer::Impl::Impl() : persistor(new ModulePersistor) {}

ModulePackageContainer::Impl::~Impl()
{
    if (activatedModules) {
        map_free(activatedModules);
        delete activatedModules;
    }
    g_object_unref(moduleSack);
}

void
ModulePackageContainer::add(const std::string &fileContent)
{
    Pool * pool = dnf_sack_get_pool(pImpl->moduleSack);
    auto metadata = ModuleMetadata::metadataFromString(fileContent);
    Repo * r;
    Id id;

    FOR_REPOS(id, r) {
        if (strcmp(r->name, "available") == 0) {
            for (auto data : metadata) {
                ModulePackagePtr modulePackage(new ModulePackage(
                    pImpl->moduleSack, r, data));
                pImpl->modules.insert(std::make_pair(modulePackage->getId(), modulePackage));
                g_autofree gchar *path = g_build_filename(pImpl->installRoot.c_str(), "/etc/dnf/modules.d", NULL);
                pImpl->persistor->insert(modulePackage->getName(), path);
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

void ModulePackageContainer::createConflictsBetweenStreams()
{
    // TODO Use libdnf::Query for filtering
    for (const auto &iter : pImpl->modules) {
        const auto &modulePackage = iter.second;

        for (const auto &innerIter : pImpl->modules) {
            if (modulePackage->getName() == innerIter.second->getName()
                && modulePackage->getStream() != innerIter.second->getStream()) {
                modulePackage->addStreamConflict(innerIter.second);
            }
        }
    }
}

ModulePackagePtr ModulePackageContainer::getModulePackage(Id id)
{
    return pImpl->modules[id];
}

std::vector<ModulePackagePtr>
ModulePackageContainer::requiresModuleEnablement(const libdnf::PackageSet & packages)
{
    auto activatedModules = pImpl->activatedModules;
    if (!activatedModules) {
        return {};
    }
    std::vector<ModulePackagePtr> output;
    libdnf::Query baseQuery(packages.getSack());
    baseQuery.addFilter(HY_PKG, HY_EQ, &packages);
    baseQuery.apply();
    libdnf::Query testQuery(baseQuery);
    auto modules = pImpl->modules;
    auto moduleMapSize = dnf_sack_get_pool(pImpl->moduleSack)->nsolvables;
    for (Id id = 0; id < moduleMapSize; ++id) {
        if (!MAPTST(activatedModules, id)) {
            continue;
        }
        auto module = modules[id];
        if (isEnabled(module)) {
            continue;
        }
        auto includeNEVRAs = module->getArtifacts();
        std::vector<const char *> includeNEVRAsCString(includeNEVRAs.size() + 1);
        transform(includeNEVRAs.begin(), includeNEVRAs.end(), includeNEVRAsCString.begin(), std::mem_fn(&std::string::c_str));
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
bool ModulePackageContainer::isEnabled(const std::string &name, const std::string &stream)
{
    return pImpl->persistor->getState(name) == ModuleState::ENABLED &&
        pImpl->persistor->getStream(name) == stream;
}

bool ModulePackageContainer::isEnabled(const ModulePackagePtr &module)
{
    return isEnabled(module->getName(), module->getStream());
}

/**
 * @brief Mark ModulePackage as part of an enabled stream.
 */
void ModulePackageContainer::enable(const std::string &name, const std::string &stream)
{
    for (const auto &iter : pImpl->modules) {
        auto modulePackage = iter.second;
        if (modulePackage->getName() == name && modulePackage->getStream() == stream) {
            pImpl->persistor->changeStream(name, stream);
            pImpl->persistor->changeState(name, ModuleState::ENABLED);
            /* TODO: throw error in case of stream change? */
        }
    }
}

/**
 * @brief Mark module as not part of an enabled stream.
 */
void ModulePackageContainer::disable(const std::string &name, const std::string &stream)
{
    for (const auto &iter : pImpl->modules) {
        auto modulePackage = iter.second;
        if (modulePackage->getName() == name && modulePackage->getStream() == stream) {
            pImpl->persistor->changeState(name, ModuleState::DISABLED);
            pImpl->persistor->changeStream(name, "");
        }
    }
}

void ModulePackageContainer::install(const std::string &name, const std::string &stream, const std::string &profile)
{
    for (const auto &iter : pImpl->modules) {
        auto modulePackage = iter.second;
        if (modulePackage->getName() == name && modulePackage->getStream() == stream) {
            if (pImpl->persistor->getStream(name) == stream)
                pImpl->persistor->addProfile(name, profile);
        }
    }
}

void ModulePackageContainer::uninstall(const std::string &name, const std::string &stream, const std::string &profile)
{
    for (const auto &iter : pImpl->modules) {
        auto modulePackage = iter.second;
        if (modulePackage->getName() == name && modulePackage->getStream() == stream) {
            if (pImpl->persistor->getStream(name) == stream)
                pImpl->persistor->removeProfile(name, profile);
        }
    }
}

std::unique_ptr<libdnf::IdQueue>
ModulePackageContainer::Impl::moduleSolve(const std::vector<ModulePackagePtr> & modules)
{
    if (modules.empty()) {
        return {};
    }
    Pool * pool = dnf_sack_get_pool(moduleSack);
    dnf_sack_recompute_considered(moduleSack);
    dnf_sack_make_provides_ready(moduleSack);
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
    transaction_free(transaction);
    solver_free(solver);
    return installed;
}

std::vector<ModulePackagePtr>
ModulePackageContainer::query(libdnf::Nsvcap& moduleNevra)
{
    auto version = moduleNevra.getVersion();
    std::string strinVersion;
    if (version != -1) {
        strinVersion = std::to_string(version);
    }
    return query(moduleNevra.getName(), moduleNevra.getStream(), strinVersion,
                 moduleNevra.getContext(), moduleNevra.getArch());
}

std::vector<ModulePackagePtr>
ModulePackageContainer::query(std::string subject)
{
    // Alternativally a search using module provides could be performed
    std::vector<ModulePackagePtr> result;
    libdnf::Query query(pImpl->moduleSack);
    // platform modules are installed and not in modules std::Map.
    query.addFilter(HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
    std::ostringstream ss;
    ss << subject << "*";
    query.addFilter(HY_PKG_NAME, HY_GLOB, ss.str().c_str());
    auto pset = query.runSet();
    Id moduleId = -1;
    while ((moduleId = pset->next(moduleId)) != -1) {
        result.push_back(pImpl->modules.at(moduleId));
    }
    return result;
}

std::vector<ModulePackagePtr>
ModulePackageContainer::query(std::string name, std::string stream, std::string version,
    std::string context, std::string arch)
{
    // Alternativally a search using module provides could be performed
    std::vector<ModulePackagePtr> result;
    libdnf::Query query(pImpl->moduleSack);
    // platform modules are installed and not in modules std::Map.
    query.addFilter(HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
    std::ostringstream ss;
    ss << stringFormater(name) << ":" << stringFormater(stream);
    ss << ":" << stringFormater(version) << ":";
    ss << stringFormater(context);
    query.addFilter(HY_PKG_NAME, HY_GLOB, ss.str().c_str());
    if (!arch.empty()) {
        query.addFilter(HY_PKG_ARCH, HY_GLOB, arch.c_str());
    }
    auto pset = query.runSet();
    Id moduleId = -1;
    while ((moduleId = pset->next(moduleId)) != -1) {
        result.push_back(pImpl->modules.at(moduleId));
    }
    return result;
}

std::string
ModulePackageContainer::getReport()
{
    std::string report;
    auto enabled = getEnabledStreams();
    if (!enabled.empty()) {
        report += "Module Enabling:\n";
        for (auto & item: enabled) {
            report += "    ";
            report += item.first;
            report += ":";
            report += item.second;
            report += "\n";
        }
        report += "\n";
    }
    auto disabled = getDisabledStreams();
    if (!disabled.empty()) {
        report += "Module Disabling:\n";
        for (auto & item: disabled) {
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
        switchedReport += "Module Switched Streams:\n";
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
    auto installedProfiles = getInstalledProfiles();
    if (!installedProfiles.empty()) {
        report += "Module Installing Profiles:\n";
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
        report += "Module Removing Profiles:\n";
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
    return report;
}

void ModulePackageContainer::setModuleDefaults(const std::map<std::string, std::string> &defaultStreams)
{
    pImpl->moduleDefaults = defaultStreams;
}

void ModulePackageContainer::resolveActiveModulePackages()
{
    auto defaultStreams = pImpl->moduleDefaults;
    Pool * pool = dnf_sack_get_pool(pImpl->moduleSack);
    std::vector<ModulePackagePtr> packages;

    // Use only Enabled or Default modules for transaction
    for (const auto &iter : pImpl->modules) {
        auto module = iter.second;

        bool hasDefaultStream;
        try {
            hasDefaultStream = defaultStreams.at(module->getName()) == module->getStream();
        } catch (std::out_of_range &exception) {
            hasDefaultStream = false;
            // TODO logger.debug(exception.what())
        }
        if (isEnabled(module)) {
            packages.push_back(module);
        } else if (hasDefaultStream) {
            pImpl->persistor->changeState(module->getName(), ModuleState::DEFAULT);
            packages.push_back(module);
        }
    }
    auto ids = pImpl->moduleSolve(packages);
    if (!ids || ids->size() == 0) {
        return;
    }
    if (pImpl->activatedModules) {
        map_free(pImpl->activatedModules);
    } else {
        pImpl->activatedModules = new Map;
    }
    map_init(pImpl->activatedModules, pool->nsolvables);
    auto activatedModules = pImpl->activatedModules;
    for (int i = 0; i < ids->size(); ++i) {
        Id id = (*ids)[i];
        auto solvable = pool_id2solvable(pool, id);
        // TODO use Goal::listInstalls() to not requires filtering out Platform
        if (strcmp(solvable->repo->name, HY_SYSTEM_REPO_NAME) == 0)
            continue;
        MAPSET(activatedModules, id);
    }
}

bool ModulePackageContainer::isModuleActive(Id id)
{
    if (pImpl->activatedModules) {
        return MAPTST(pImpl->activatedModules, id);
    }
    return false;
}


std::vector<ModulePackagePtr> ModulePackageContainer::getModulePackages()
{
    std::vector<ModulePackagePtr> values;
    auto modules = pImpl->modules;
    std::transform(std::begin(modules), std::end(modules), std::back_inserter(values),
                   [](const std::map<Id, ModulePackagePtr>::value_type &pair){ return pair.second; });

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

std::map<std::string, std::string> ModulePackageContainer::getDisabledStreams()
{
    return pImpl->persistor->getDisabledStreams();
}

std::map<std::string, std::pair<std::string, std::string>> ModulePackageContainer::getSwitchedStreams()
{
    return pImpl->persistor->getSwitchedStreams();
}

std::map<std::string, std::vector<std::string>> ModulePackageContainer::getInstalledProfiles()
{
    return pImpl->persistor->getInstalledProfiles();
}

std::map<std::string, std::vector<std::string>> ModulePackageContainer::getRemovedProfiles()
{
    return pImpl->persistor->getRemovedProfiles();
}
const std::string & ModulePackageContainer::Impl::ModulePersistor::getStream(const std::string &name)
{
    return configs[name].second.stream;
}

bool ModulePackageContainer::Impl::ModulePersistor::changeStream(const std::string &name, const std::string &stream)
{
    if (getStream(name) == stream)
        return false;

    configs[name].second.stream = stream;
    return true;
}

const std::vector<std::string> &
ModulePackageContainer::Impl::ModulePersistor::getProfiles(const std::string &name)
{
    return configs[name].second.profiles;
}

bool ModulePackageContainer::Impl::ModulePersistor::addProfile(const std::string &name, const std::string &profile)
{
    auto &profiles = configs[name].second.profiles;
    const auto &it = std::find(std::begin(profiles), std::end(profiles), name);
    if (it != std::end(profiles))
        return false;

    profiles.push_back(profile);
    return true;
}

bool ModulePackageContainer::Impl::ModulePersistor::removeProfile(const std::string &name, const std::string &profile)
{
    auto &profiles = configs[name].second.profiles;

    for (auto it = profiles.begin(); it != profiles.end(); it++) {
        if (*it == profile) {
            profiles.erase(it);
            return true;
        }
    }

    return false;
}

const ModuleState &
ModulePackageContainer::Impl::ModulePersistor::getState(const std::string &name)
{
    return configs[name].second.state;
}

bool ModulePackageContainer::Impl::ModulePersistor::changeState(const std::string &name, ModuleState state)
{
    if (configs[name].second.state == state)
        return false;

    configs[name].second.state = state;
    return true;
}

bool ModulePackageContainer::Impl::ModulePersistor::insert(const std::string &moduleName, const char *path)
{
    /* There can only be one config file per module */
    if (configs.find(moduleName) != configs.end()) {
        return false;
    }

    configs.emplace(moduleName, std::make_pair(libdnf::ConfigParser{}, Config()));

    auto &parser = configs[moduleName].first;
    auto &newConfig = configs[moduleName].second;
    auto &data = parser.getData();

    data[moduleName]["name"] = moduleName;
    try {
        parser.read(std::string(path) + "/" + moduleName + ".module");
    } catch (const libdnf::ConfigParser::CantOpenFile &) {
        /* No module config file present. Fill values in */
        data[moduleName]["state"] = "";
        data[moduleName]["stream"] = newConfig.stream = "";
        data[moduleName]["profiles"] = "";
        newConfig.state = fromString("");
        return true;
    }

    libdnf::OptionStringList slist{std::vector<std::string>()};
    const auto &plist = parser.getValue(moduleName, "profiles");
    newConfig.profiles = std::move(slist.fromString(plist));

    std::string stateStr;
    const auto it = data[moduleName].find("enabled");
    if (it != data[moduleName].end()) { /* old format */
        stateStr = it->second;
    } else {
        stateStr = data[moduleName].find("state")->second;
    }

    newConfig.state = fromString(stateStr);
    data[moduleName]["state"] = toString(newConfig.state); /* save new format */
    data[moduleName].erase("enabled"); /* remove option from old format */

    newConfig.stream = data[moduleName]["stream"];

    return true;
}

bool ModulePackageContainer::Impl::ModulePersistor::update(const std::string &name)
{
    bool changed = false;
    auto &data = configs[name].first.getData();

    const auto &state = toString(getState(name));
    if (data[name]["state"] != state) {
        data[name]["state"] = state;
        changed = true;
    }

    if (data[name]["stream"] != getStream(name)) {
        data[name]["stream"] = getStream(name);
        changed = true;
    }

    libdnf::OptionStringList slist{std::vector<std::string>()};
    auto profiles = std::move(slist.toString(getProfiles(name)));
    profiles = profiles.substr(1, profiles.size()-2);
    if (data[name]["profiles"] != profiles) {
        data[name]["profiles"] = std::move(profiles);
        changed = true;
    }

    return changed;
}

void ModulePackageContainer::Impl::ModulePersistor::reset(const std::string &name)
{
    auto &data = configs[name].first.getData();

    configs[name].second.stream = data[name]["stream"];
    configs[name].second.state = fromString(data[name]["state"]);
    libdnf::OptionStringList slist{std::vector<std::string>()};
    configs[name].second.profiles = slist.fromString(data[name]["profiles"]);
}

void ModulePackageContainer::Impl::ModulePersistor::save(const std::string &installRoot, const std::string &modulesPath)
{
    for (auto &iter : configs) {
        const auto &name = iter.first;

        if (update(name)) {
            g_autofree gchar *fname = g_build_filename(installRoot.c_str(),
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

    for (const auto &it : configs) {
        libdnf::OptionStringList slist{std::vector<std::string>()};
        const auto &name = it.first;
        const auto &parser = it.second.first;
        const auto &newConf = it.second.second;

        const auto &vprof = slist.fromString(parser.getValue(name, "profiles"));
        std::vector<std::string> profDiff;
        std::set_difference(newConf.profiles.begin(), newConf.profiles.end(),
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

    for (const auto &it : configs) {
        libdnf::OptionStringList slist{std::vector<std::string>()};
        const auto &name = it.first;
        const auto &parser = it.second.first;
        const auto &newConf = it.second.second;

        const auto &vprof = slist.fromString(parser.getValue(name, "profiles"));
        std::vector<std::string> profDiff;
        std::set_difference(vprof.begin(), vprof.end(),
                            newConf.profiles.begin(), newConf.profiles.end(),
                            std::back_inserter(profDiff));
        if (profDiff.size() > 0) {
            profiles.emplace(name, std::move(profDiff));
        }
    }

    return profiles;
}
