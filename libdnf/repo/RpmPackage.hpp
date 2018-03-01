#ifndef LIBDNF_RPMPACKAGE_HPP
#define LIBDNF_RPMPACKAGE_HPP


#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "libdnf/dnf-advisory.h"
#include "libdnf/dnf-utils.h"
#include "libdnf/hy-iutil-private.hpp"

#include "libdnf/repo/solvable/Package.hpp"
#include "libdnf/repo/solvable/DependencyContainer.hpp"

typedef enum {
    DNF_PACKAGE_INFO_UNKNOWN                        = 0,    /// Unknown state
    DNF_PACKAGE_INFO_UPDATE                         = 11,   /// Package update
    DNF_PACKAGE_INFO_INSTALL                        = 12,   /// Package install
    DNF_PACKAGE_INFO_REMOVE                         = 13,   /// Package remove
    DNF_PACKAGE_INFO_CLEANUP                        = 14,   /// Package cleanup
    DNF_PACKAGE_INFO_OBSOLETE                       = 15,   /// Package obsolete
    DNF_PACKAGE_INFO_REINSTALL                      = 19,   /// Package re-install
    DNF_PACKAGE_INFO_DOWNGRADE                      = 20,   /// Package downgrade
    /*< private >*/
    DNF_PACKAGE_INFO_LAST
} DnfPackageInfo;

namespace libdnf {
class RpmPackage : public Package
{
public:
    static bool download(const std::vector<RpmPackage> &packages, const std::string &directory, DnfState *state);
    static unsigned long getDownloadSize(const std::vector<RpmPackage> &packages);

public:
    RpmPackage(DnfSack *sack, Id id);
    RpmPackage(DnfSack *sack, const std::shared_ptr<DnfRepo> &repo, const libdnf::Nevra &nevra);
    RpmPackage(const RpmPackage &rpmPackage);
    ~RpmPackage() override;

    bool operator==(const RpmPackage &rpmPackage);
    bool operator==(const std::string &evr);
    bool operator>(const std::string &evr);
    bool operator<(const std::string &evr);
    int compare(const RpmPackage &rpmPackage);
    int evrCompare(const RpmPackage &rpmPackage);
    int evrCompare(const std::string &evr);

    bool isInstalled() const;
    const char *getLocation() const;
    const char *getBaseurl() const;
    const char *getNevra() const;
    const char *getSourceRpm() const;
    const char *getName() const override;
    const char *getVersion() const override;
    const char *getRelease() const;
    const char *getPackager() const;
    const char *getDescription() const;
    const char *getEvr() const;
    const char *getGroup() const;
    const char *getLicense() const;
    const char *getReponame() const;
    const char *getSummary() const;
    const char *getUrl() const;
    const unsigned char *getPackageHeaderHash() const;
    std::string getPackageId() const;
    std::string getFilename() const;
    std::tuple<const unsigned char *, int> getChecksum() const;
    std::tuple<const unsigned char *, int> getHeaderChecksum() const;
    unsigned int getCost() const;
    unsigned long getDownloadSize() const;
    unsigned long getEpoch() const;
    unsigned long getHeaderEndIndex() const;
    unsigned long getInstallSize() const;
    unsigned long getBuildTime() const;
    unsigned long getInstallTime() const;
    unsigned long getMediaNumber() const;
    unsigned long getRpmdbId() const;
    unsigned long getSize() const;
    std::shared_ptr<DnfPackageDelta> getDelta(const std::string &fromEvr) const;
    const std::shared_ptr<DnfRepo> &getRepo() const;
    std::vector<char *> getFiles() const;
    std::vector<std::shared_ptr<DnfAdvisory> > getAdvisories(int compareType) const;
    DnfStateAction getAction() const;

    void setPackageHeaderHash(unsigned char *hash);
    void setRepo(const std::shared_ptr<DnfRepo> &repo);
    void setAction(DnfStateAction action);

    bool checkFilename();

    /** dnf api */
    const char *getDebugName();
    const char *getSourceDebugName();
    const char *getSourceName();
    bool isEvrEqual(const RpmPackage &package);
    bool isEvrGreater(const RpmPackage &package);
    bool isEvrLesser(const RpmPackage &package);

private:
    const char *lookupString(unsigned type) const;
    unsigned long lookupNumber(unsigned type) const;

    LrChecksumType transformToLrChecksumType(GChecksumType checksum);
    std::string buildPackageId(const std::string &name, const std::string &version, const std::string &arch,
                               const std::string &data) const;

    std::shared_ptr<DnfRepo> repo;
    DnfStateAction action;
    unsigned char *headerHash;
};

};

#endif //LIBDNF_RPMPACKAGE_HPP
