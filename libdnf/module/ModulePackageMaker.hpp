#ifndef LIBDNF_MODULEPACKAGEMAKER_HPP
#define LIBDNF_MODULEPACKAGEMAKER_HPP

#include <map>
#include <string>

#include "ModulePackage.hpp"

class ModulePackageMaker
{
public:
    static std::map<std::string, std::vector<std::shared_ptr<ModulePackage>>> fromFile(DnfSack *sack, HyRepo repo, const std::string &filePath);
    static std::map<std::string, std::vector<std::shared_ptr<ModulePackage>>> fromString(DnfSack *sack, HyRepo repo, const std::string &fileContent);

private:
    static std::map<std::string, std::vector<std::shared_ptr<ModulePackage>>> createModulePackages(DnfSack *sack, HyRepo repo, const std::vector<std::shared_ptr<ModuleMetadata>> &metadata);
};


#endif //LIBDNF_MODULEPACKAGEMAKER_HPP
