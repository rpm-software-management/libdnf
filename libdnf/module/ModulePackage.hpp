#ifndef LIBDNF_MODULEPACKAGE_HPP
#define LIBDNF_MODULEPACKAGE_HPP

#include <memory>
#include <string>
#include <modulemd/modulemd-module.h>

#include "ModuleView.hpp"
#include "state/ModuleState.hpp"
#include "modulemd/ModuleMetadata.hpp"
#include "modulemd/profile/ModuleProfile.hpp"
#include "libdnf/repo/solvable/Package.hpp"

class ModulePackage: public Package
{
public:
    explicit ModulePackage(DnfSack *sack, HyRepo repo, std::shared_ptr<ModuleMetadata> metadata);
    ModulePackage(DnfSack *sack, HyRepo repo, std::shared_ptr<ModuleMetadata> metadata, std::shared_ptr<ModuleView> view);

    const char *getName() const override;
    std::string getStream() const;
    std::string getNameStream() const;
    const char *getVersion() const override;
    std::string getContext() const;
    std::string getArchitecture() const;
    std::string getFullIdentifier() const;

    std::string getSummary() const;
    std::string getDescription() const;

    std::vector<std::string> getArtifacts() const;

    std::shared_ptr<Profile> getProfile(const std::string &name) const;
    std::vector<std::shared_ptr<ModuleProfile> > getProfiles() const;

    std::vector<std::shared_ptr<ModuleDependencies> > getModuleRequires() const;

    bool isEnabled();

    void setState(std::shared_ptr<ModuleState> state);
    void setView(std::shared_ptr<ModuleView> view);

    void enable();
//    void disable();
//
//    void install(std::shared_ptr<Profile> profile);
//    void upgrade(std::shared_ptr<Profile> profile);
//    void upgrade();
//    void remove(std::shared_ptr<Profile> profile);
//    void remove();
//
//    void printInfo();

private:
    std::shared_ptr<ModuleMetadata> metadata;
    std::shared_ptr<ModuleState> state;
    std::shared_ptr<ModuleView> view;
};


#endif //LIBDNF_MODULEPACKAGE_HPP
