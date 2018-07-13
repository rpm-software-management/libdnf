#include "ModulePackageMaker.hpp"
#include "modulemd/ModuleMetadata.hpp"

#include <memory>

std::map<Id, std::shared_ptr<ModulePackage>> ModulePackageMaker::fromString(Pool *pool, HyRepo repo, const std::string &fileContent)
{
    auto metadata = ModuleMetadata::metadataFromString(fileContent);
    return createModulePackages(pool, repo, metadata);
}

std::map<Id, std::shared_ptr<ModulePackage>> ModulePackageMaker::createModulePackages(Pool *pool, HyRepo repo, const std::vector<std::shared_ptr<ModuleMetadata>> &metadata)
{
    std::map<Id, std::shared_ptr<ModulePackage>> modules;

    Repo *solvRepo = repo->libsolv_repo;
    Repo *clonedRepo = repo_create(pool, solvRepo->name);

    for (auto data : metadata) {
        auto modulePackage = std::make_shared<ModulePackage>(pool, clonedRepo, data);
        modules.insert(std::make_pair(modulePackage->getId(), modulePackage));
    }

    return modules;
}
