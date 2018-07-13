#ifndef LIBDNF_MODULEPACKAGEMAKER_HPP
#define LIBDNF_MODULEPACKAGEMAKER_HPP

#include <map>
#include <string>

#include "ModulePackage.hpp"

class ModulePackageMaker
{
public:
    static std::map<Id, std::shared_ptr<ModulePackage>> fromString(Pool *pool, HyRepo repo, const std::string &fileContent);

private:
    static std::map<Id, std::shared_ptr<ModulePackage>> createModulePackages(Pool *pool, HyRepo repo, const std::vector<std::shared_ptr<ModuleMetadata>> &metadata);
};


#endif //LIBDNF_MODULEPACKAGEMAKER_HPP
