#include "ModulePackage.hpp"

#include <iostream>
#include <sstream>
#include <utility>

ModulePackage::ModulePackage(Pool *pool, Repo *repo, const std::shared_ptr<ModuleMetadata> &metadata)
        : metadata(metadata)
        , state(ModuleState::UNKNOWN)
        , pool(pool)
{
    id = repo_add_solvable(repo);
    Solvable *solvable = pool_id2solvable(pool, id);

    createProvisions(solvable);
    createDependencies(solvable);
}

/**
 * @brief Create module provides based on modulemd metadata.
 */
void ModulePackage::createProvisions(Solvable *solvable) const
{
    std::ostringstream ss;

    // create solvable with:
    //   Name: $name:$stream:$version:$context
    //   Version: 0
    //   Arch: $arch
    ss << getName() << ":" << getStream() << ":" << getVersion() << ":" << getContext();
    solvable_set_str(solvable, SOLVABLE_NAME, ss.str().c_str());
    solvable_set_str(solvable, SOLVABLE_EVR, "0");
    solvable_set_str(solvable, SOLVABLE_ARCH, metadata->getArchitecture().c_str());

    // create Provide: module($name)
    ss.str(std::string());
    ss << "module(" << getName() << ")";
    auto depId = pool_str2id(pool, ss.str().c_str(), 1);
    solvable_add_deparray(solvable, SOLVABLE_PROVIDES, depId, -1);

    // create Provide: module($name:$stream)
    ss.str(std::string());
    ss << "module(" << getNameStream() << ")";
    depId = pool_str2id(pool, ss.str().c_str(), 1);
    solvable_add_deparray(solvable, SOLVABLE_PROVIDES, depId, -1);

    // create Provide: module($name:$stream:$version)
    ss.str(std::string());
    ss << "module(" << getNameStreamVersion() << ")";
    depId = pool_str2id(pool, ss.str().c_str(), 1);
    solvable_add_deparray(solvable, SOLVABLE_PROVIDES, depId, -1);
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
std::string ModulePackage::getArchitecture() const
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
