#include "ModulePackage.hpp"

#include <sstream>
#include <utility>
#include <libdnf/module/state/EnabledState.hpp>

ModulePackage::ModulePackage(DnfSack *sack, HyRepo repo, std::shared_ptr<ModuleMetadata> metadata)
        : Package(sack, repo, metadata->getName(), std::to_string(metadata->getVersion()), metadata->getArchitecture())
        , metadata(std::move(metadata))
        , state(std::make_shared<ModuleState>())
        , view(std::make_shared<ModuleView>())
{}

ModulePackage::ModulePackage(DnfSack *sack, HyRepo repo, std::shared_ptr<ModuleMetadata> metadata, std::shared_ptr<ModuleView> view)
        : Package(sack, repo, metadata->getName(), std::to_string(metadata->getVersion()), metadata->getArchitecture())
        , metadata(metadata)
        , state(std::make_shared<ModuleState>())
        , view(std::move(view))
{}

const char *ModulePackage::getName() const
{
    return metadata->getName().c_str();
}

std::string ModulePackage::getNameStream() const
{
    std::ostringstream ss;
    ss << getName() << ":" << getStream();
    return ss.str();
}

std::string ModulePackage::getStream() const
{
    return metadata->getStream();
}

const char *ModulePackage::getVersion() const
{
    return std::to_string(metadata->getVersion()).c_str();
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

std::vector<std::shared_ptr<ModuleDependencies> > ModulePackage::getModuleRequires() const
{
    return metadata->getDependencies();
}

bool ModulePackage::isEnabled()
{
    return state->isEnabled();
}

void ModulePackage::setState(std::shared_ptr<ModuleState> state)
{
    this->state = std::move(state);
}

void ModulePackage::setView(std::shared_ptr<ModuleView> view)
{
    this->view = std::move(view);
}

void ModulePackage::enable()
{
    state = std::make_shared<EnabledState>();
}
