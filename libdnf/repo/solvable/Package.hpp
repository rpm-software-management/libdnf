#ifndef LIBDNF_PACKAGE_HPP
#define LIBDNF_PACKAGE_HPP

#include <vector>
#include <solv/solvable.h>
#include <solv/repo.h>


#include "libdnf/hy-types.h"
#include "libdnf/hy-repo-private.hpp"

#include "Dependency.hpp"

struct Package
{
public:
    Package(DnfSack *sack, Id id);
    Package(const Package &package);
    virtual ~Package();

    std::shared_ptr<DependencyContainer> getConflicts() const;
    std::shared_ptr<DependencyContainer> getEnhances() const;
    std::shared_ptr<DependencyContainer> getObsoletes() const;
    std::shared_ptr<DependencyContainer> getProvides() const;
    std::shared_ptr<DependencyContainer> getRecommends() const;
    std::shared_ptr<DependencyContainer> getRequires() const;
    std::shared_ptr<DependencyContainer> getRequiresPre() const;
    std::shared_ptr<DependencyContainer> getSuggests() const;
    std::shared_ptr<DependencyContainer> getSupplements() const;
    Id getId() const;

    virtual const char *getName() const = 0;
    virtual const char *getVersion() const = 0;
    const char *getArch() const;

protected:
    Package(DnfSack *sack, HyRepo repo, const char *name, const char *version, const char *arch);
    Package(DnfSack *sack, HyRepo repo, const std::string &name, const std::string &version, const std::string &arch);

    void addConflicts(std::shared_ptr<Dependency> dependency);
    void addEnhances(std::shared_ptr<Dependency> dependency);
    void addObsoletes(std::shared_ptr<Dependency> dependency);
    void addProvides(std::shared_ptr<Dependency> dependency);
    void addRecommends(std::shared_ptr<Dependency> dependency);
    void addRequires(std::shared_ptr<Dependency> dependency);
    void addRequiresPre(std::shared_ptr<Dependency> dependency);
    void addSuggests(std::shared_ptr<Dependency> dependency);
    void addSupplements(std::shared_ptr<Dependency> dependency);

    const char *getSolvableName() const;
    const char *getSolvableEvr() const;
    const char *getSolvableVendor() const;
    void setSolvableVendor(const char *vendor);

private:
    void createSolvable(HyRepo repo);
    void fillSolvableData(const char *name, const char *version, const char *arch) const;
    std::shared_ptr<DependencyContainer> getDependencies(Id type, Id marker = -1) const;
    void addDependency(std::shared_ptr<Dependency> dependency, int type, Id marker = -1);
    Queue *getDependencyQueue(Id type, Id marker) const;

    DnfSack *sack;
    Id id;
};

#endif //LIBDNF_PACKAGE_HPP
