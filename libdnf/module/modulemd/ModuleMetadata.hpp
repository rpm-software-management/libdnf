
#ifndef LIBDNF_MODULEMETADATA_HPP
#define LIBDNF_MODULEMETADATA_HPP

#include <string>
#include <map>
#include <vector>

#include "profile/ModuleProfile.hpp"
#include "ModuleDependencies.hpp"

#include <modulemd/modulemd-module.h>

class ModuleMetadata
{
public:
    static std::vector<std::shared_ptr<ModuleMetadata> > metadataFromFile(const std::string &filePath);
    static std::vector<std::shared_ptr<ModuleMetadata> > metadataFromString(const std::string &fileContent);

public:
    explicit ModuleMetadata(const std::shared_ptr<ModulemdModule> &modulemd);
    ~ModuleMetadata();

    std::string getName() const;
    std::string getStream() const;
    long long getVersion() const;
    std::string getContext() const;
    std::string getArchitecture() const;
    std::string getDescription() const;
    std::string getSummary() const;
    std::vector<std::shared_ptr<ModuleDependencies> > getDependencies() const;
    std::vector<std::string> getArtifacts() const;
    std::vector<std::shared_ptr<ModuleProfile>> getProfiles() const;
    std::shared_ptr<Profile> getProfile(const std::string &profileName) const;

private:
    static std::vector<std::shared_ptr<ModuleMetadata> > wrapModulemdModule(GPtrArray *data);

    std::shared_ptr<ModulemdModule> modulemd;
    static void reportFailures(const GPtrArray *failures);
};


#endif //LIBDNF_MODULEMETADATA_HPP
