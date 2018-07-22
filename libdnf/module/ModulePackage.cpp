#include "ModulePackage.hpp"
#include "libdnf/module/state/EnabledState.hpp"

#include <sstream>
#include <utility>
#include <iostream>

ModulePackage::ModulePackage(Pool *pool, Repo *repo, const std::shared_ptr<ModuleMetadata> &metadata)
        : metadata(metadata)
        , state(std::make_shared<ModuleState>())
        , pool(pool)
{
    id = repo_add_solvable(repo);
    Solvable *solvable = pool_id2solvable(pool, id);

    createProvisions(solvable);
    createDependencies(solvable);
}

void ModulePackage::createProvisions(Solvable *solvable) const
{
    std::ostringstream ss;
    ss << getName() << ":" << getStream() << ":" << getVersion() << ":" << getContext();
    solvable_set_str(solvable, SOLVABLE_NAME, ss.str().c_str());
    solvable_set_str(solvable, SOLVABLE_EVR, "0");
    solvable_set_str(solvable, SOLVABLE_ARCH, metadata->getArchitecture().c_str());

    ss.str(std::string());
    ss << "module(" << getName() << ")";
    auto depId = pool_str2id(pool, ss.str().c_str(), 1);
    solvable_add_deparray(solvable, SOLVABLE_PROVIDES, depId, -1);

    ss.str(std::string());
    ss << "module(" << getNameStream() << ")";
    depId = pool_str2id(pool, ss.str().c_str(), 1);
    solvable_add_deparray(solvable, SOLVABLE_PROVIDES, depId, -1);

    ss.str(std::string());
    ss << "module(" << getNameStreamVersion() << ")";
    depId = pool_str2id(pool, ss.str().c_str(), 1);
    solvable_add_deparray(solvable, SOLVABLE_PROVIDES, depId, -1);
}

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

std::string ModulePackage::getName() const
{
    return metadata->getName();
}

std::string ModulePackage::getNameStream() const
{
    std::ostringstream ss;
    ss << getName() << ":" << getStream();
    return ss.str();
}

std::string ModulePackage::getNameStreamVersion() const
{
    std::ostringstream ss;
    ss << getNameStream() << ":" << getVersion();
    return ss.str();
}

std::string ModulePackage::getStream() const
{
    return metadata->getStream();
}

std::string ModulePackage::getVersion() const
{
    return std::to_string(metadata->getVersion());
}

std::string ModulePackage::getContext() const
{
    return metadata->getContext();
}

std::string ModulePackage::getArchitecture() const
{
    return metadata->getArchitecture();
}

std::string ModulePackage::getFullIdentifier() const
{
    std::ostringstream ss;
    ss << getName() << ":" << getStream() << ":" << getVersion() << ":" << getContext() << ":" << getArchitecture();
    return ss.str();
}

std::string ModulePackage::getSummary() const
{
    return metadata->getSummary();
}

std::string ModulePackage::getDescription() const
{
    return metadata->getDescription();
}

std::vector<std::string> ModulePackage::getArtifacts() const
{
    return metadata->getArtifacts();
}

std::shared_ptr<Profile> ModulePackage::getProfile(const std::string &name) const
{
    return metadata->getProfile(name);
}

std::vector<std::shared_ptr<ModuleProfile> > ModulePackage::getProfiles() const
{
    return metadata->getProfiles();
}

std::vector<std::shared_ptr<ModuleDependencies> > ModulePackage::getModuleDependencies() const
{
    return metadata->getDependencies();
}

bool ModulePackage::isEnabled()
{
    return state->isEnabled();
}

void ModulePackage::setState(const std::shared_ptr<ModuleState> &state)
{
    this->state = state;
}

void ModulePackage::enable()
{
    state = std::make_shared<EnabledState>();
}

void ModulePackage::addStreamConflict(const std::shared_ptr<ModulePackage> &package)
{
    std::ostringstream ss;
    Solvable *solvable = pool_id2solvable(pool, id);

    ss << "module(" + package->getNameStream() + ")";
    auto depId = pool_str2id(pool, ss.str().c_str(), 1);
    solvable_add_deparray(solvable, SOLVABLE_CONFLICTS, depId, -1);

}
