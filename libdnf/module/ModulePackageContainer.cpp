#include "ModulePackageContainer.hpp"
#include "ModulePackageMaker.hpp"

#include "libdnf/utils/utils.hpp"
#include "libdnf/utils/File.hpp"

#include <algorithm>
#include <set>

ModulePackageContainer::ModulePackageContainer(DnfSack *sack)
        : sack(sack)
{}

void ModulePackageContainer::loadFromFile(HyRepo repo, const std::string &filePath)
{
    auto fromFileModules = ModulePackageMaker::fromFile(sack, repo, filePath);
    modules.insert(fromFileModules.begin(), fromFileModules.end());
}

void ModulePackageContainer::loadFromString(HyRepo repo, const std::string &content)
{
    auto fromStringModules = ModulePackageMaker::fromString(sack, repo, content);
    modules.insert(fromStringModules.begin(), fromStringModules.end());
}

std::vector<std::string> ModulePackageContainer::getModuleNames()
{
    std::vector<std::string> names;
    names.reserve(modules.size());
    for (const auto &module : modules) {
        names.push_back(module.first);
    }
    return names;
}

std::shared_ptr<ModulePackage> ModulePackageContainer::getModulePackage(const std::string &name,
                                                                        const std::string &stream)
{
    auto modulePackages = getModulePackages(name, stream);
    return getLatest(modulePackages);
}

std::shared_ptr<ModulePackage> ModulePackageContainer::getEnabledModulePackage(const std::string &name)
{
    auto modulePackages = getEnabledModulePackages(name);
    return getLatest(modulePackages);
}

std::vector<std::shared_ptr<ModulePackage>> ModulePackageContainer::getEnabledModulePackages(const std::string &name)
{
    auto modulePackages = getModulesByName(name);
    std::vector<std::shared_ptr<ModulePackage>> enabledPackages;

    for (const auto &modulePackage : modulePackages) {
        if (modulePackage->isEnabled()) {
            enabledPackages.emplace_back(modulePackage);
        }
    }

    if (enabledPackages.empty())
        throw EnabledStreamException(name);

    return enabledPackages;
}

std::vector<std::shared_ptr<ModulePackage>> ModulePackageContainer::getModulePackages(const std::string &name)
{
    return getModulesByName(name);
}

std::vector<std::shared_ptr<ModulePackage>> ModulePackageContainer::getModulePackages(const std::string &name, const std::string &stream)
{
    auto modulePackages = getModulesByName(name);
    return getModulesByStream(stream, modulePackages);
}

std::vector<std::shared_ptr<ModulePackage>> ModulePackageContainer::getModuleDependencies(const std::shared_ptr<ModulePackage> &modulePackage, std::vector<std::string> visited)
{
    if (std::find(visited.begin(), visited.end(), modulePackage->getFullIdentifier()) == visited.end())
        visited.push_back(modulePackage->getFullIdentifier());
    else
        return std::vector<std::shared_ptr<ModulePackage>>{};

    std::vector<std::shared_ptr<ModulePackage>> requires;
    for (const auto &moduleDependency: modulePackage->getModuleRequires()) {
        for (const auto &moduleRequires : moduleDependency->getRequires()) {
            for (const auto &mapIter : moduleRequires) {
                const auto &name = mapIter.first;
                for (const auto &stream : mapIter.second) {
                    bool isStreamExcluded = string::startsWith(stream, "-");
                    if (isStreamExcluded)
                        continue;

                    try {
                        for (const auto &foundModulePackage : getModulePackages(name, stream)) {
                            auto resolvedModuleDependencies = getModuleDependencies(foundModulePackage, visited);
                            requires.insert(requires.end(), resolvedModuleDependencies.begin(), resolvedModuleDependencies.end());
                        }
                    } catch (NoStreamException &exception) {
                        continue;
                    }
                }
            }
        }
    }

    return requires;
}

std::vector<std::shared_ptr<ModulePackage>> ModulePackageContainer::getModulesByName(const std::string &name) const
{
    try {
        return modules.at(name);
    } catch (std::out_of_range &e) {
        throw NoModuleException(name);
    }
}

std::vector<std::shared_ptr<ModulePackage>> ModulePackageContainer::getModulesByStream(const std::string &stream,
                                                                                       const std::vector<std::shared_ptr<ModulePackage>> &modulePackages)
{
    std::vector<std::shared_ptr<ModulePackage>> streamModules;

    for (const auto &modulePackage : modulePackages) {
        if (modulePackage->getStream() == stream) {
            streamModules.emplace_back(modulePackage);
        }
    }

    if (streamModules.empty())
        throw NoStreamException(stream);

    return streamModules;
}

std::shared_ptr<ModulePackage> ModulePackageContainer::getLatest(const std::vector<std::shared_ptr<ModulePackage>> &modulePackages) const
{
    std::shared_ptr<ModulePackage> latest = nullptr;
    for (const auto &package : modulePackages) {
        if (latest == nullptr || latest->getVersion() < package->getVersion()) {
            latest = package;
        }
    }
    return latest;
}
