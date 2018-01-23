#ifndef LIBDNF_DEPENDENCY_HPP
#define LIBDNF_DEPENDENCY_HPP

#include <memory>
#include <string>
#include <vector>
#include <solv/knownid.h>

#include "libdnf/dnf-sack.h"

struct Dependency
{
public:
    Dependency(DnfSack *sack, int id);
    Dependency(DnfSack *sack, const char *name, const char *version, int solvComparisonOperator);
    Dependency(DnfSack *sack, const std::string &dependency);
    Dependency(const Dependency &dependency);
    ~Dependency();

    const char *getName() const;
    const char *getRelation() const;
    const char *getVersion() const;
    const char *toString() const;
    Id getId() const;

private:
    void parseAndCreateDependency(const std::string &dependency);
    void createRelationalDependency(const std::vector<std::string> &results);
    bool isRichDependency(const std::string &dependency) const;
    int determineComparisonType(const std::string &type);

    DnfSack *sack;
    Id id;
};

#endif //LIBDNF_DEPENDENCY_HPP
