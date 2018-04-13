#include "ModulePackageMaker.hpp"
#include "modulemd/ModuleMetadata.hpp"

#include <memory>

std::map<std::string, std::vector<std::shared_ptr<ModulePackage>>> ModulePackageMaker::fromFile(DnfSack *sack, HyRepo repo, const std::string &filePath)
{
    auto metadata = ModuleMetadata::metadataFromFile(filePath);
    return createModulePackages(sack, repo, metadata);
}

std::map<std::string, std::vector<std::shared_ptr<ModulePackage>>> ModulePackageMaker::fromString(DnfSack *sack, HyRepo repo, const std::string &fileContent)
{
    auto metadata = ModuleMetadata::metadataFromString(fileContent);
    return createModulePackages(sack, repo, metadata);
}

std::map<std::string, std::vector<std::shared_ptr<ModulePackage>>> ModulePackageMaker::createModulePackages(DnfSack *sack, HyRepo repo, const std::vector<std::shared_ptr<ModuleMetadata>> &metadata)
{
    std::map<std::string, std::vector<std::shared_ptr<ModulePackage>>> modules;
    for (auto data : metadata) {
        std::string name = data->getName();
        auto modulePackage = std::make_shared<ModulePackage>(sack, repo, data);
        modules[name].emplace_back(modulePackage);
    }

    return modules;
}
