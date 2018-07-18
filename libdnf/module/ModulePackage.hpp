#ifndef LIBDNF_MODULEPACKAGE_HPP
#define LIBDNF_MODULEPACKAGE_HPP

#include <memory>
#include <string>
#include <modulemd/modulemd-module.h>

#include "modulemd/ModuleMetadata.hpp"
#include "modulemd/profile/ModuleProfile.hpp"
#include "libdnf/repo/solvable/Package.hpp"
#include "../goal/IdQueue.hpp"

class ModulePackageMaker;

class ModulePackage // TODO inherit in future; : public Package
{
public:
    enum class ModuleState {
        UNKNOWN,
        ENABLED
    };

    ModulePackage(Pool *pool, Repo *repo, const std::shared_ptr<ModuleMetadata> &metadata);

    /**
    * @brief Create module provides based on modulemd metadata.
    */
    std::string getName() const;
    std::string getStream() const;
    std::string getNameStream() const;
    std::string getNameStreamVersion() const;
    std::string getVersion() const;
    std::string getContext() const;
    const char * getArchitecture() const;
    std::string getFullIdentifier() const;

    std::string getSummary() const;
    std::string getDescription() const;

    std::vector<std::string> getArtifacts() const;

    std::shared_ptr<Profile> getProfile(const std::string &name) const;
    std::vector<std::shared_ptr<ModuleProfile> > getProfiles() const;

    std::vector<std::shared_ptr<ModuleDependencies> > getModuleDependencies() const;

    bool isEnabled();

    void addStreamConflict(const std::shared_ptr<ModulePackage> &package);

    void enable();

    Id getId() const { return id; };
    Pool * getPool();

private:
    void createDependencies(Solvable *solvable) const;

    std::shared_ptr<ModuleMetadata> metadata;
    ModuleState state;

    // TODO: remove after inheriting from Package
    Pool *pool;
    Id id;
};

inline Pool * ModulePackage::getPool() { return pool;}

Id createPlatformSolvable(Pool *pool, const std::string &osReleasePath);
std::unique_ptr<libdnf::IdQueue> moduleSolve(const std::vector<std::shared_ptr<ModulePackage>> & modules);

#endif //LIBDNF_MODULEPACKAGE_HPP
