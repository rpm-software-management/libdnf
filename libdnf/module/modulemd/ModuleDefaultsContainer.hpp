#ifndef LIBDNF_MODULEDEFAULTSCONTAINER_HPP
#define LIBDNF_MODULEDEFAULTSCONTAINER_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <modulemd/modulemd-defaults.h>
#include <modulemd/modulemd-prioritizer.h>

class ModuleDefaultsContainer
{
public:
    struct Exception : public std::runtime_error
    {
        explicit Exception(const std::string &what) : std::runtime_error(what) {}
    };

    struct ConflictException : public Exception
    {
        explicit ConflictException(const std::string &what) : Exception(what) {}
    };

    struct ResolveException : public Exception
    {
        explicit ResolveException(const std::string &what) : Exception(what) {}
    };

    ModuleDefaultsContainer();
    ~ModuleDefaultsContainer() = default;

    void fromString(const std::string &content, int priority);
    void fromFile(const std::string &path, int priority);

    std::string getDefaultStreamFor(std::string moduleName);
    std::map<std::string, std::string> getDefaultStreams();

    void resolve();

private:
    void saveDefaults(GPtrArray *data, int priority);

    template<typename T>
    void checkAndThrowException(GError *error);

    std::shared_ptr<ModulemdPrioritizer> prioritizer;
    std::map<std::string, std::shared_ptr<ModulemdDefaults>> defaults;
    void reportFailures(const GPtrArray *failures) const;
};


#endif //LIBDNF_MODULEDEFAULTSCONTAINER_HPP
