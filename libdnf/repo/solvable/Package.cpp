#include "Package.hpp"

#include <utility>
#include "DependencyContainer.hpp"
#include <../hy-iutil-private.hpp>
#include "../../goal/IdQueue.hpp"


inline static std::string stringSafeConstructor(const char * value)
{
    return value ? value : std::string();
}

namespace libdnf {

Package::Package(DnfSack *sack, Id id)
        : sack(sack)
        , id(id)
{}

Package::Package(DnfSack *sack,
                 HyRepo repo,
                 const char *name,
                 const char *version,
                 const char *arch,
                 bool createSolvable)
        : sack(sack)
{
    if (createSolvable) {
        this->createSolvable(repo);
        fillSolvableData(name, version, arch);
    }
}

Package::Package(DnfSack *sack,
                 HyRepo repo,
                 const std::string &name,
                 const std::string &version,
                 const std::string &arch,
                 bool createSolvable)
        : sack(sack)
{
    if (createSolvable) {
        this->createSolvable(repo);
        fillSolvableData(name.c_str(), version.c_str(), arch.c_str());
    }
}

Package::Package(const Package &package)
        : sack(package.sack)
        , id(package.id)
{}

Package::~Package() = default;

const char *Package::getName() const
{
    Pool *pool = dnf_sack_get_pool(sack);
    Solvable *solvable = pool_id2solvable(pool, id);
    return pool_id2str(pool, solvable->name);
}

const char *Package::getEvr() const
{
    Pool *pool = dnf_sack_get_pool(sack);
    Solvable *solvable = pool_id2solvable(pool, id);
    return pool_id2str(pool, solvable->evr);
}

const char *Package::getArch() const
{
    Pool *pool = dnf_sack_get_pool(sack);
    Solvable *solvable = pool_id2solvable(pool, id);
    return pool_id2str(pool, solvable->arch);
}

const char *Package::getSolvableVendor() const
{
    Pool *pool = dnf_sack_get_pool(sack);
    Solvable *solvable = pool_id2solvable(pool, id);
    return pool_id2str(pool, solvable->vendor);
}

void Package::setSolvableVendor(const char *vendor)
{
    Solvable *solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    solvable_set_str(solvable, SOLVABLE_VENDOR, vendor);
}

Id Package::getId() const
{
    return id;
}

std::string Package::getBaseUrl()
{
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    
    return stringSafeConstructor(solvable_lookup_str(solvable, SOLVABLE_MEDIABASE));
}

std::vector<std::string> Package::getFiles()
{
    Pool *pool = dnf_sack_get_pool(sack);
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    Dataiterator di;
    std::vector<std::string> out;
    repo_internalize_trigger(solvable->repo);
    dataiterator_init(&di, pool, solvable->repo, id, SOLVABLE_FILELIST, NULL,
                      SEARCH_FILES | SEARCH_COMPLETE_FILELIST);
    while (dataiterator_step(&di)) {
        out.emplace_back(di.kv.str);
    }
    dataiterator_free(&di);
    return out;
}

std::string Package::getSourceRpm()
{
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    repo_internalize_trigger(solvable->repo);
    return stringSafeConstructor(solvable_lookup_sourcepkg(solvable));
}

std::string Package::getVersion()
{
    char *e, *v, *r;
    pool_split_evr(dnf_sack_get_pool(sack), getEvr(), &e, &v, &r);
    return stringSafeConstructor(v);
}

std::string Package::getRelease()
{
    char *e, *v, *r;
    pool_split_evr(dnf_sack_get_pool(sack), getEvr(), &e, &v, &r);
    return stringSafeConstructor(r);
}

bool Package::isInstalled() const
{
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    return (dnf_sack_get_pool(sack)->installed == solvable->repo);
}


std::unique_ptr<libdnf::DependencyContainer> Package::getConflicts() const
{
    return getDependencies(SOLVABLE_CONFLICTS);
};

std::unique_ptr<libdnf::DependencyContainer> Package::getEnhances() const
{
    return getDependencies(SOLVABLE_ENHANCES);
}

std::unique_ptr<libdnf::DependencyContainer> Package::getObsoletes() const
{
    return getDependencies(SOLVABLE_OBSOLETES);
}

std::unique_ptr<libdnf::DependencyContainer> Package::getProvides() const
{
    return getDependencies(SOLVABLE_PROVIDES);
}

std::unique_ptr<libdnf::DependencyContainer> Package::getRecommends() const
{
    return getDependencies(SOLVABLE_RECOMMENDS);
}

std::unique_ptr<libdnf::DependencyContainer> Package::getRequires() const
{
    return getDependencies(SOLVABLE_REQUIRES, 0);
}

std::unique_ptr<libdnf::DependencyContainer> Package::getRequiresPre() const
{
    return getDependencies(SOLVABLE_REQUIRES, 1);
}

std::unique_ptr<libdnf::DependencyContainer> Package::getSuggests() const
{
    return getDependencies(SOLVABLE_SUGGESTS);
}

std::unique_ptr<libdnf::DependencyContainer> Package::getSupplements() const
{
    return getDependencies(SOLVABLE_SUPPLEMENTS);
}

void Package::addConflicts(std::shared_ptr<libdnf::Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_CONFLICTS);
}

void Package::addEnhances(std::shared_ptr<libdnf::Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_ENHANCES);
}

void Package::addObsoletes(std::shared_ptr<libdnf::Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_OBSOLETES);
}

void Package::addProvides(std::shared_ptr<libdnf::Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_PROVIDES);
}

void Package::addRecommends(std::shared_ptr<libdnf::Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_RECOMMENDS);
}

void Package::addRequires(std::shared_ptr<libdnf::Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_REQUIRES, 0);
}

void Package::addRequiresPre(std::shared_ptr<libdnf::Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_REQUIRES, 1);
}

void Package::addSuggests(std::shared_ptr<libdnf::Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_SUGGESTS);
}

void Package::addSupplements(std::shared_ptr<libdnf::Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_SUPPLEMENTS);
}

void Package::createSolvable(HyRepo repo)
{
    id = repo_add_solvable(repo->libsolv_repo);
}

void Package::fillSolvableData(const char *name, const char *version,
                               const char *arch) const
{
    Solvable *solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);

    solvable_set_str(solvable, SOLVABLE_NAME, name);
    solvable_set_str(solvable, SOLVABLE_EVR, version);
    solvable_set_str(solvable, SOLVABLE_ARCH, arch);
}

std::unique_ptr<libdnf::DependencyContainer> Package::getDependencies(Id type, Id marker) const
{
    IdQueue dependencyQueue;

    solvable_lookup_deparray(pool_id2solvable(dnf_sack_get_pool(sack), id), type,
                             dependencyQueue.getQueue(), marker);
    std::unique_ptr<libdnf::DependencyContainer> container(new libdnf::DependencyContainer(sack));

    for (int i = 0; i < dependencyQueue.size(); i++) {
        Id id = dependencyQueue[i];
        if (id != SOLVABLE_PREREQMARKER) {
            container->add(id);
        }
    }
    return container;
}

void Package::addDependency(std::shared_ptr<libdnf::Dependency> dependency, int type, Id marker)
{
    Solvable *solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    solvable_add_deparray(solvable, type, dependency->getId(), marker);
}

}
