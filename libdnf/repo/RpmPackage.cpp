#include "RpmPackage.hpp"
#include "libdnf/nevra.hpp"
#include "libdnf/dnf-advisory-private.hpp"
#include "libdnf/dnf-packagedelta-private.hpp"
#include "libdnf/utils/utils.hpp"
#include "libdnf/hy-util.h"
#include "libdnf/sack/advisory.hpp"

#include <map>
#include <solv/evr.h>
#include <sstream>
#include <fcntl.h>

LrChecksumType libdnf::RpmPackage::transformToLrChecksumType(GChecksumType checksum)
{
    if (checksum == G_CHECKSUM_MD5)
        return LR_CHECKSUM_MD5;
    if (checksum == G_CHECKSUM_SHA1)
        return LR_CHECKSUM_SHA1;
    if (checksum == G_CHECKSUM_SHA256)
        return LR_CHECKSUM_SHA256;
    return LR_CHECKSUM_SHA512;
}

std::string libdnf::RpmPackage::buildPackageId(const std::string &name, const std::string &version,
                                       const std::string &arch, const std::string &data) const
{
    std::stringstream ss;
    ss << name
       << (version.empty() ? "" : ";") << version
       << (arch.empty() ? "" : ";") << arch
       << (data.empty() ? "" : ";") << data;

    return ss.str();
}

// TODO refactor after DnfRepo is refactored
bool libdnf::RpmPackage::download(const std::vector<libdnf::RpmPackage> &packages, const std::string &directory, DnfState *state)
{
    DnfState *stateLocal;
    std::map<std::shared_ptr<DnfRepo>, GPtrArray */* std::vector<libdnf::RpmPackage> */> repoToPackages;

    for (const auto &package : packages) {
        auto repo = package.getRepo();
        GPtrArray *repoPackages;

        if (repo == nullptr) {
            throw "Repo of " + std::string(package.getName()) + " is unset";
        }

        repoPackages = repoToPackages[repo];

        if (repoPackages == nullptr) {
            repoToPackages[repo] = g_ptr_array_new();
        }
        g_ptr_array_add(repoPackages, (gpointer) &package);
    }

    dnf_state_set_number_steps(state, (unsigned int) repoToPackages.size());

    for (auto &repoToPackage : repoToPackages) {
        GError *error = nullptr;

        stateLocal = dnf_state_get_child(state);
        if (!dnf_repo_download_packages(repoToPackage.first.get(), repoToPackage.second, directory.c_str(),
                                        stateLocal, &error))
            return false;

        if (!dnf_state_done(state, &error))
            return false;
    }
    return true;
}

unsigned long libdnf::RpmPackage::getDownloadSize(const std::vector<libdnf::RpmPackage> &packages)
{
    unsigned long downloadSize = 0;

    for (const auto &package : packages) {
        downloadSize += package.getDownloadSize();
    }

    return downloadSize;
}


libdnf::RpmPackage::RpmPackage(DnfSack *sack, Id id)
        : Package(sack, id)
{
    action = DNF_STATE_ACTION_UNKNOWN;
    repo = nullptr;
    headerHash = nullptr;
}

libdnf::RpmPackage::RpmPackage(DnfSack *sack, const std::shared_ptr<DnfRepo> &repo, const libdnf::Nevra &nevra)
        : Package(sack, dnf_repo_get_repo(repo.get()), nevra.getName().c_str(), nevra.getEvr().c_str(), nevra.getArch().c_str())
{
    action = DNF_STATE_ACTION_UNKNOWN;
    this->repo = repo;
    headerHash = nullptr;
}

libdnf::RpmPackage::~RpmPackage() = default;

libdnf::RpmPackage::RpmPackage(const libdnf::RpmPackage &rpmPackage)
    : Package(rpmPackage)
{}

bool libdnf::RpmPackage::operator==(const libdnf::RpmPackage &rpmPackage)
{
    bool identicalIds = getId() == rpmPackage.getId();
    if (getSack() == nullptr)
        return identicalIds;

    Solvable *solvable = pool_id2solvable(getPool(), getId());
    Solvable *otherSolvable = pool_id2solvable(getPool(), rpmPackage.getId());
    return getId() == rpmPackage.getId() && solvable_identical(solvable, otherSolvable) == 1;
}

int libdnf::RpmPackage::compare(const libdnf::RpmPackage &rpmPackage)
{
    int result = strcmp(getName(), rpmPackage.getName());
    if (result != 0)
        return result;

    result = evrCompare(rpmPackage);
    if (result != 0)
        return result;

    return strcmp(getArch(), getArch());
}

int libdnf::RpmPackage::evrCompare(const std::string &evr)
{
    return pool_evrcmp_str(getPool(), getEvr(), evr.c_str(), EVRCMP_COMPARE);
}

const char *libdnf::RpmPackage::getLocation() const
{
    Solvable *solvable = pool_id2solvable(getPool(), getId());
    if (solvable->repo) {
        repo_internalize_trigger(solvable->repo);
    }

    return solvable_get_location(solvable, nullptr);
}

const char *libdnf::RpmPackage::getSourceRpm() const
{
    Solvable *solvable = pool_id2solvable(getPool(), getId());
    repo_internalize_trigger(solvable->repo);
    return solvable_lookup_sourcepkg(solvable);
}

const char *libdnf::RpmPackage::getVersion() const
{
    char *e, *v, *r;
    pool_split_evr(getPool(), Package::getSolvableEvr(), &e, &v, &r);
    return v;
}

const char *libdnf::RpmPackage::getRelease() const
{
    char *e, *v, *r;
    pool_split_evr(getPool(), Package::getSolvableEvr(), &e, &v, &r);
    return r;
}

std::string libdnf::RpmPackage::getFilename() const
{
    if (isInstalled() || repo == nullptr)
        return std::string{};

    if (dnf_repo_is_local(repo.get())) {
        return std::string(dnf_repo_get_location(repo.get())) + "/" + getLocation();
    } else {
        return std::string(dnf_repo_get_packages(repo.get())) + "/" + getLocation();
    }
}

std::tuple<const unsigned char *, int> libdnf::RpmPackage::getChecksum() const
{
    Solvable *solvable = pool_id2solvable(getPool(), getId());
    repo_internalize_trigger(solvable->repo);

    int checksumType;
    const unsigned char *cChecksum = solvable_lookup_bin_checksum(solvable, SOLVABLE_CHECKSUM, &checksumType);

    if (cChecksum) {
        checksumType = checksumt_l2h(checksumType);

        return std::tuple<const unsigned char *, int>{cChecksum, checksumType};
    }

    return std::tuple<const unsigned char *, int>{nullptr, -1};
}

std::tuple<const unsigned char *, int> libdnf::RpmPackage::getHeaderChecksum() const
{
    Solvable *solvable = pool_id2solvable(getPool(), getId());
    repo_internalize_trigger(solvable->repo);

    int headerChecksumType;
    const unsigned char *cChecksum = solvable_lookup_bin_checksum(solvable, SOLVABLE_HDRID, &headerChecksumType);

    if (cChecksum) {
        headerChecksumType = checksumt_l2h(headerChecksumType);
        return std::tuple<const unsigned char *, int>{cChecksum, headerChecksumType};
    }

    return std::tuple<const unsigned char *, int>{nullptr, -1};
}

const unsigned char *libdnf::RpmPackage::getPackageHeaderHash() const
{
    if (headerHash != nullptr)
        return headerHash;

    auto checksum = getHeaderChecksum();
    if (std::get<0>(checksum) == nullptr)
        return nullptr;

    return reinterpret_cast<const unsigned char *>(hy_chksum_str(std::get<0>(checksum), std::get<1>(checksum)));
}

std::string libdnf::RpmPackage::getPackageId() const
{
    std::string reponame = getReponame();
    if (string::endsWith(reponame, HY_SYSTEM_REPO_NAME)) {
        reponame = "installed";
    } else if (string::endsWith(reponame, HY_CMDLINE_REPO_NAME)) {
        reponame = "local";
    }
    return buildPackageId(Package::getSolvableName(), Package::getSolvableEvr(), Package::getArch(), reponame);
}

unsigned int libdnf::RpmPackage::getCost() const
{
    if (repo == nullptr) {
        return UINT_MAX;
    }

    return dnf_repo_get_cost(repo.get());
}

// TODO change return value to std::vector<std::string> after deleting hy_package::dnf_package_get_files()
std::vector<char *> libdnf::RpmPackage::getFiles() const
{
    Pool *pool = getPool();
    Dataiterator dataiterator{};
    std::vector<char *> files;
    Solvable *solvable = pool_id2solvable(getPool(), getId());

    repo_internalize_trigger(solvable->repo);

    dataiterator_init(&dataiterator, pool, solvable->repo, getId(), SOLVABLE_FILELIST, nullptr,
                      SEARCH_FILES | SEARCH_COMPLETE_FILELIST);
    while (dataiterator_step(&dataiterator)) {
        files.emplace_back(const_cast<char *>(dataiterator.kv.str));
    }
    dataiterator_free(&dataiterator);

    return files;
}

std::vector<std::shared_ptr<DnfAdvisory> > libdnf::RpmPackage::getAdvisories(int compareType) const
{
    Dataiterator dataiterator{};
    Pool *pool = getPool();
    std::vector<std::shared_ptr<DnfAdvisory> > advisories;
    Solvable *solvable = pool_id2solvable(getPool(), getId());

    dataiterator_init(&dataiterator, pool, nullptr, 0, UPDATE_COLLECTION_NAME,
                      pool_id2str(pool, solvable->name), SEARCH_STRING);
    dataiterator_prepend_keyname(&dataiterator, UPDATE_COLLECTION);

    while (dataiterator_step(&dataiterator)) {
        dataiterator_setpos_parent(&dataiterator);
        if (pool_lookup_id(pool, SOLVID_POS, UPDATE_COLLECTION_ARCH) != solvable->arch)
            continue;
        Id evr = pool_lookup_id(pool, SOLVID_POS, UPDATE_COLLECTION_EVR);
        if (!evr)
            continue;

        int poolEvrComparison = pool_evrcmp(pool, evr, solvable->evr, EVRCMP_COMPARE);
        if ((poolEvrComparison > 0 && (compareType & HY_GT)) ||
            (poolEvrComparison < 0 && (compareType & HY_LT)) ||
            (poolEvrComparison == 0 && (compareType & HY_EQ))) {

            // TODO change to std::make_shared<Advisory>(sack, dataiterator.solvid) after refactoring of DnfAdvisory
            advisories.emplace_back(std::shared_ptr<libdnf::Advisory>(dnf_advisory_new(getSack(), dataiterator.solvid)));
            dataiterator_skip_solvable(&dataiterator);
        }
    }
    dataiterator_free(&dataiterator);

    return advisories;
}

std::shared_ptr<DnfPackageDelta> libdnf::RpmPackage::getDelta(const std::string &fromEvr) const
{
    Pool *pool = getPool();
    std::shared_ptr<DnfPackageDelta> delta;
    Dataiterator dataiterator{};
    Solvable *solvable = pool_id2solvable(getPool(), getId());

    dataiterator_init(&dataiterator, pool, solvable->repo, SOLVID_META, DELTA_PACKAGE_NAME,
                      Package::getSolvableName(), SEARCH_STRING);
    dataiterator_prepend_keyname(&dataiterator, REPOSITORY_DELTAINFO);
    while (dataiterator_step(&dataiterator)) {
        dataiterator_setpos_parent(&dataiterator);
        if (pool_lookup_id(pool, SOLVID_POS, DELTA_PACKAGE_EVR) != solvable->evr ||
            pool_lookup_id(pool, SOLVID_POS, DELTA_PACKAGE_ARCH) != solvable->arch)
            continue;

        const std::string baseEvr = pool_id2str(pool, pool_lookup_id(pool, SOLVID_POS, DELTA_BASE_EVR));
        if (baseEvr != fromEvr)
            continue;

        // TODO change to std::make_shared<PackageDelta>(pool) after refactoring of DnfPackageDelta
        delta = std::shared_ptr<DnfPackageDelta>(dnf_packagedelta_new(pool),
                                                 [](DnfPackageDelta *delta){ g_object_unref(delta); });
        break;
    }
    dataiterator_free(&dataiterator);

    return delta;
}

bool libdnf::RpmPackage::checkFilename()
{
    const std::string path = getFilename();

    if (!filesystem::exists(path) && dnf_repo_is_local(repo.get())) {
            throw "File missing in local repository " + path;
    }

    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        throw "Failed to open " + path;
    }

    auto checksum = getChecksum();
    auto expectedChecksum = hy_chksum_str(std::get<0>(checksum), std::get<1>(checksum));

    gboolean matches;
    GError *error = nullptr;
    if (!lr_checksum_fd_cmp(transformToLrChecksumType((GChecksumType) std::get<1>(checksum)),
                            fd,
                            expectedChecksum,
                            true, /* use xattr value */
                            &matches,
                            &error))
        close(fd);


    if (!matches && dnf_repo_is_local(repo.get())) {
        throw "Checksum mismatch in local repository " + path;
    }

    return true;
}

const char *libdnf::RpmPackage::getSourceName()
{
    std::string &&sourceRpm = getSourceRpm();
    if (!sourceRpm.empty()) {
        sourceRpm = string::trimSuffix(sourceRpm, ".src.rpm");
        sourceRpm = string::rsplit(sourceRpm, "-", 2)[0];
        return sourceRpm.c_str();
    }

    return nullptr;
}

const char *libdnf::RpmPackage::lookupString(unsigned type) const
{
    Solvable *solvable = pool_id2solvable(getPool(), getId());
    repo_internalize_trigger(solvable->repo);
    return solvable_lookup_str(solvable, type);
}

unsigned long libdnf::RpmPackage::lookupNumber(unsigned type) const
{
    Solvable *solvable = pool_id2solvable(getPool(), getId());
    repo_internalize_trigger(solvable->repo);
    return solvable_lookup_num(solvable, type, 0);
}

bool libdnf::RpmPackage::operator==(const std::string &evr) { return Package::getSolvableEvr() == evr; };
bool libdnf::RpmPackage::operator>(const std::string &evr) { return evrCompare(evr) > 0; };
bool libdnf::RpmPackage::operator<(const std::string &evr) { return evrCompare(evr) < 0; };
int libdnf::RpmPackage::evrCompare(const libdnf::RpmPackage &rpmPackage) { return evrCompare(rpmPackage.getEvr()); };

bool libdnf::RpmPackage::isInstalled() const { return getPool()->installed == pool_id2solvable(getPool(), getId())->repo; };
const char *libdnf::RpmPackage::getBaseurl() const { return lookupString(SOLVABLE_MEDIABASE); };
const char *libdnf::RpmPackage::getNevra() const { return pool_solvable2str(getPool(), pool_id2solvable(getPool(), getId())); };
const char *libdnf::RpmPackage::getPackager() const { return lookupString(SOLVABLE_PACKAGER); };
const char *libdnf::RpmPackage::getDescription() const { return lookupString(SOLVABLE_DESCRIPTION); };
const char *libdnf::RpmPackage::getEvr() const { return Package::getSolvableEvr(); };
const char *libdnf::RpmPackage::getGroup() const { return lookupString(SOLVABLE_GROUP); };
const char *libdnf::RpmPackage::getLicense() const { return lookupString(SOLVABLE_LICENSE); };
const char *libdnf::RpmPackage::getReponame() const { return pool_id2solvable(getPool(), getId())->repo->name; };
const char *libdnf::RpmPackage::getSummary() const { return lookupString(SOLVABLE_SUMMARY); };
const char *libdnf::RpmPackage::getUrl() const { return lookupString(SOLVABLE_URL); };
unsigned long libdnf::RpmPackage::getDownloadSize() const { return lookupNumber(SOLVABLE_DOWNLOADSIZE); };
const char *libdnf::RpmPackage::getName() const { return getSolvableName(); };
unsigned long libdnf::RpmPackage::getEpoch() const { return pool_get_epoch(getPool(), Package::getSolvableEvr()); };
unsigned long libdnf::RpmPackage::getHeaderEndIndex() const { return lookupNumber(SOLVABLE_HEADEREND); };
unsigned long libdnf::RpmPackage::getInstallSize() const { return lookupNumber(SOLVABLE_INSTALLSIZE); };
unsigned long libdnf::RpmPackage::getBuildTime() const { return lookupNumber(SOLVABLE_BUILDTIME); };
unsigned long libdnf::RpmPackage::getInstallTime() const { return lookupNumber(SOLVABLE_INSTALLTIME); };
unsigned long libdnf::RpmPackage::getMediaNumber() const { return lookupNumber(SOLVABLE_MEDIANR); };
unsigned long libdnf::RpmPackage::getRpmdbId() const { return lookupNumber(RPM_RPMDBID); };
unsigned long libdnf::RpmPackage::getSize() const { return lookupNumber(isInstalled() ? SOLVABLE_INSTALLSIZE : SOLVABLE_DOWNLOADSIZE); };
const std::shared_ptr<DnfRepo> &libdnf::RpmPackage::getRepo() const { return repo; };
DnfStateAction libdnf::RpmPackage::getAction() const { return action; };

void libdnf::RpmPackage::setPackageHeaderHash(unsigned char *hash) { headerHash = hash; };
void libdnf::RpmPackage::setRepo(const std::shared_ptr<DnfRepo> &repo) { this->repo = repo; };
void libdnf::RpmPackage::setAction(DnfStateAction action) { this->action = action; };

const char *libdnf::RpmPackage::getDebugName() { return (std::string{getName()} + "-debuginfo").c_str(); };
const char *libdnf::RpmPackage::getSourceDebugName() { return (std::string{getSourceName()} + "-debuginfo").c_str(); };

bool libdnf::RpmPackage::isEvrEqual(const libdnf::RpmPackage &package) { return *this == package.getEvr(); };
bool libdnf::RpmPackage::isEvrGreater(const libdnf::RpmPackage &package) { return *this < package.getEvr(); };
bool libdnf::RpmPackage::isEvrLesser(const libdnf::RpmPackage &package) { return *this > package.getEvr(); };
