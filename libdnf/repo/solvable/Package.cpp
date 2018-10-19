#include "Package.hpp"

extern "C" {
#include <solv/solvable.h>
#include <solv/repo.h>
}

#include <utility>
#include "DependencyContainer.hpp"
#include "../../hy-iutil-private.hpp"
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

unsigned long Package::getEpoch()
{
    return pool_get_epoch(dnf_sack_get_pool(sack), getEvr());
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

std::string Package::getDebugName()
{
    return stringSafeConstructor(getName()) + "-debuginfo";
}

std::string Package::getSourceName()
{
    auto sourceRpm = getSourceRpm();
    if (sourceRpm.empty()) {
        return sourceRpm;
    }
    auto stop1 = sourceRpm.find_last_of('-');
    if (stop1 == std::string::npos) {
        return sourceRpm;
    }
    if (stop1 == 0) {
        return {};
    }
    auto stop2 = sourceRpm.find_last_of('-', stop1 - 1);
    if (stop2 == std::string::npos) {
        return sourceRpm.substr(stop1);
    }
    return sourceRpm.substr(stop2);
}

std::string Package::getSourceDebugName()
{
    auto output = getSourceName();
    if (output.empty()) {
        return output;
    }
    return output + "-debuginfo";
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

const char * Package::getRepoName()
{
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    return solvable->repo->name;
}

std::string Package::getDescription()
{
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    return stringSafeConstructor(solvable_lookup_str(solvable, SOLVABLE_DESCRIPTION));
}

std::string Package::getLicense()
{
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    return stringSafeConstructor(solvable_lookup_str(solvable, SOLVABLE_LICENSE));
}

std::string Package::getPackager()
{
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    return stringSafeConstructor(solvable_lookup_str(solvable, SOLVABLE_PACKAGER));
}

std::string Package::getSummary()
{
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    return stringSafeConstructor(solvable_lookup_str(solvable, SOLVABLE_SUMMARY));
}

std::string Package::getUrl()
{
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    return stringSafeConstructor(solvable_lookup_str(solvable, SOLVABLE_URL));
}

std::string Package::getGroup()
{
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    return stringSafeConstructor(solvable_lookup_str( solvable, SOLVABLE_GROUP));
}

std::string Package::getLocation()
{
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    repo_internalize_trigger(solvable->repo);
    return stringSafeConstructor(solvable_get_location(solvable, NULL));
}

const unsigned char * Package::getCheckSum(int *type)
{
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    const unsigned char* ret;

    repo_internalize_trigger(solvable->repo);
    ret = solvable_lookup_bin_checksum(solvable, SOLVABLE_CHECKSUM, type);
    if (ret)
        *type = checksumt_l2h(*type);
    return ret;
}

const unsigned char * Package::getHdrCheckkSum(int *type)
{
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    const unsigned char *ret;

    repo_internalize_trigger(solvable->repo);
    ret = solvable_lookup_bin_checksum(solvable, SOLVABLE_HDRID, type);
    if (ret)
        *type = checksumt_l2h(*type);
    return ret;
}

bool Package::isInstalled() const
{
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    return (dnf_sack_get_pool(sack)->installed == solvable->repo);
}

unsigned long long Package::getBuildTime()
{
    return lookup_num(SOLVABLE_BUILDTIME);
}

unsigned long long Package::downloadSize()
{
    return lookup_num(SOLVABLE_DOWNLOADSIZE);
}

unsigned long long Package::getInstallTime()
{
    return lookup_num(SOLVABLE_INSTALLTIME);
}

unsigned long long Package::getInstallSize()
{
    return lookup_num(SOLVABLE_INSTALLSIZE);
}

unsigned long long Package::getMediaNumber()
{
    return lookup_num(SOLVABLE_MEDIANR);
}

unsigned long long Package::getRpmdbId()
{
    return lookup_num(RPM_RPMDBID);
}

unsigned long long Package::getSize()
{
    Id type = isInstalled() ? SOLVABLE_INSTALLSIZE : SOLVABLE_DOWNLOADSIZE;
    return lookup_num(type);
}

unsigned long long Package::getHeaderEnd()
{
    return lookup_num(SOLVABLE_HEADEREND);
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

unsigned long long Package::lookup_num(Id type)
{
    Solvable * solvable = pool_id2solvable(dnf_sack_get_pool(sack), id);
    repo_internalize_trigger(solvable->repo);
    return solvable_lookup_num(solvable, type, 0);
}

}
