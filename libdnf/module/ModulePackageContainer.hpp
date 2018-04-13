#ifndef LIBDNF_MODULEPACKAGECONTAINER_HPP
#define LIBDNF_MODULEPACKAGECONTAINER_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <set>

#include "ModulePackage.hpp"

class ModulePackageContainer
{
public:
    struct Exception : public std::runtime_error
    {
        explicit Exception(const std::string &what) : runtime_error(what) {}
    };

    struct NoModuleException : public Exception
    {
        explicit NoModuleException(const std::string &moduleName) : Exception("No such module: " + moduleName) {}
    };

    struct NoStreamException : public Exception
    {
        explicit NoStreamException(const std::string &moduleStream) : Exception("No such stream: " + moduleStream) {}
    };

    struct EnabledStreamException : public Exception
    {
        explicit EnabledStreamException(const std::string &moduleName) : Exception("No enabled stream for module: " + moduleName) {}
    };

    explicit ModulePackageContainer(DnfSack *sack);
    ~ModulePackageContainer() = default;

    void loadFromFile(HyRepo repo, const std::string &filePath);
    void loadFromString(HyRepo repo, const std::string &content);
    std::vector<std::string> getModuleNames();
    std::shared_ptr<ModulePackage> getModulePackage(const std::string &name, const std::string &stream);
    std::shared_ptr<ModulePackage> getEnabledModulePackage(const std::string &name);
//    std::shared_ptr<ModulePackage> getModulePackage(const std::string &name, const std::string &stream, long long version);
    std::vector<std::shared_ptr<ModulePackage>> getModulePackages(const std::string &name);
    std::vector<std::shared_ptr<ModulePackage>> getModulePackages(const std::string &name, const std::string &stream);
    std::vector<std::shared_ptr<ModulePackage>> getEnabledModulePackages(const std::string &name);
    std::vector<std::shared_ptr<ModulePackage>> getModuleDependencies(const std::shared_ptr<ModulePackage> &modulePackage, std::vector<std::string> visited = std::vector<std::string>());


private:
    std::vector<std::shared_ptr<ModulePackage>> getModulesByName(const std::string &name) const;
    std::vector<std::shared_ptr<ModulePackage>> getModulesByStream(const std::string &stream, const std::vector<std::shared_ptr<ModulePackage>> &modulePackages);
//    std::shared_ptr<ModulePackage> getModulesByVersion(long long version, const std::vector<std::shared_ptr<ModulePackage>> &modulePackages);
    std::shared_ptr<ModulePackage> getLatest(const std::vector<std::shared_ptr<ModulePackage>> &modulePackages) const;

    DnfSack *sack;
    std::map<std::string, std::vector<std::shared_ptr<ModulePackage>>> modules;
};


#endif //LIBDNF_MODULEPACKAGECONTAINER_HPP
