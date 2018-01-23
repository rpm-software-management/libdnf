#include "Package.hpp"

#include <utility>
#include "DependencyContainer.hpp"

Package::Package(DnfSack *sack, Id id)
        : sack(sack)
        , id(id)
{}

Package::Package(DnfSack *sack,
                 HyRepo repo,
                 const char *name,
                 const char *version,
                 const char *arch)
        : sack(sack)
{
    createSolvable(repo);
    fillSolvableData(name, version, arch);
}

Package::Package(DnfSack *sack,
                 HyRepo repo,
                 const std::string &name,
                 const std::string &version,
                 const std::string &arch)
        : sack(sack)
{
    createSolvable(repo);
    fillSolvableData(name.c_str(), version.c_str(), arch.c_str());
}

Package::Package(const Package &package)
        : sack(package.sack)
        , id(package.id)
{}

Package::~Package() = default;

const char *Package::getSolvableName() const
{
    Pool *pool = dnf_sack_get_pool(sack);
    Solvable *solvable = pool_id2solvable(pool, id);
    return pool_id2str(pool, solvable->name);
}

const char *Package::getSolvableEvr() const
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

std::shared_ptr<DependencyContainer> Package::getConflicts() const
{
    return getDependencies(SOLVABLE_CONFLICTS);
};

std::shared_ptr<DependencyContainer> Package::getEnhances() const
{
    return getDependencies(SOLVABLE_ENHANCES);
}

std::shared_ptr<DependencyContainer> Package::getObsoletes() const
{
    return getDependencies(SOLVABLE_OBSOLETES);
}

std::shared_ptr<DependencyContainer> Package::getProvides() const
{
    return getDependencies(SOLVABLE_PROVIDES);
}

std::shared_ptr<DependencyContainer> Package::getRecommends() const
{
    return getDependencies(SOLVABLE_RECOMMENDS);
}

std::shared_ptr<DependencyContainer> Package::getRequires() const
{
    return getDependencies(SOLVABLE_REQUIRES, 0);
}

std::shared_ptr<DependencyContainer> Package::getRequiresPre() const
{
    return getDependencies(SOLVABLE_REQUIRES, 1);
}

std::shared_ptr<DependencyContainer> Package::getSuggests() const
{
    return getDependencies(SOLVABLE_SUGGESTS);
}

std::shared_ptr<DependencyContainer> Package::getSupplements() const
{
    return getDependencies(SOLVABLE_SUPPLEMENTS);
}

void Package::addConflicts(std::shared_ptr<Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_CONFLICTS);
}

void Package::addEnhances(std::shared_ptr<Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_ENHANCES);
}

void Package::addObsoletes(std::shared_ptr<Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_OBSOLETES);
}

void Package::addProvides(std::shared_ptr<Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_PROVIDES);
}

void Package::addRecommends(std::shared_ptr<Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_RECOMMENDS);
}

void Package::addRequires(std::shared_ptr<Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_REQUIRES, 0);
}

void Package::addRequiresPre(std::shared_ptr<Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_REQUIRES, 1);
}

void Package::addSuggests(std::shared_ptr<Dependency> dependency)
{
    addDependency(std::move(dependency), SOLVABLE_SUGGESTS);
}

void Package::addSupplements(std::shared_ptr<Dependency> dependency)
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

std::shared_ptr<DependencyContainer> Package::getDependencies(Id type, Id marker) const
{
    auto queue = getDependencyQueue(type, marker);
    auto container = std::make_shared<DependencyContainer>(sack, *queue);

    queue_free(queue);
    delete queue;

    return container;
}

Queue *Package::getDependencyQueue(Id type, Id marker) const
{
    Queue dependencyQueue{};
    auto queue = static_cast<Queue *>(malloc(sizeof(Queue)));

    queue_init(queue);
    queue_init(&dependencyQueue);

    solvable_lookup_deparray(pool_id2solvable(dnf_sack_get_pool(sack), id), type, &dependencyQueue, marker);

    for (int i = 0; i < dependencyQueue.count; i++) {
        Id id = dependencyQueue.elements[i];
        if (id != SOLVABLE_PREREQMARKER)
            queue_push(queue, id);
    }

    queue_free(&dependencyQueue);

    return queue;
}

void Package::addDependency(std::shared_ptr<Dependency> dependency, int type, Id marker)
{
    Solvable *solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    solvable_add_deparray(solvable, type, dependency->getId(), marker);
}
