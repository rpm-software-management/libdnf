#ifndef LIBDNF_MODULEDEFAULTSCONTAINER_HPP
#define LIBDNF_MODULEDEFAULTSCONTAINER_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <modulemd/modulemd.h>

class ModuleDefaultsContainer
{
public:
    ModuleDefaultsContainer() = default;
    ~ModuleDefaultsContainer() = default;

    void fromString(const std::string &content);

    void fromFile(const std::string &path);

    std::string getDefaultStreamFor(std::string moduleName);
//    std::vector<std::string> getDefaultProfilesFor(std::string moduleName);

private:
    void saveDefaults(const GPtrArray *data);

    std::map<std::string, std::shared_ptr<ModulemdDefaults>> defaults;
    void reportFailures(const GPtrArray *failures) const;
};


#endif //LIBDNF_MODULEDEFAULTSCONTAINER_HPP
