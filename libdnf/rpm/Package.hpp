#pragma once


#include <string>


namespace libdnf::rpm {


/// @replaces dnf:dnf/package.py:class:Package
class Package {
public:
    /// @replaces dnf:dnf/package.py:attribute:Package.name
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_name(DnfPackage * pkg)
    std::string getName() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.e
    /// @replaces dnf:dnf/package.py:attribute:Package.epoch
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_epoch(DnfPackage * pkg)
    uint32_t getEpoch() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.v
    /// @replaces dnf:dnf/package.py:attribute:Package.version
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_version(DnfPackage * pkg)
    std::string getVersion() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.r
    /// @replaces dnf:dnf/package.py:attribute:Package.release
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_release(DnfPackage * pkg)
    std::string getRelease() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.a
    /// @replaces dnf:dnf/package.py:attribute:Package.arch
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_arch(DnfPackage * pkg)
    std::string getArch() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.evr
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_evr(DnfPackage * pkg)
    std::string getEvr() const;

    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_nevra(DnfPackage * pkg)
    std::string getNevra() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.group
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_group(DnfPackage * pkg)
    std::string getGroup() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.size
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_size(DnfPackage * pkg)
    std::size_t getSize() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.license
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_license(DnfPackage * pkg)
    std::string getLicense() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.sourcerpm
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_sourcerpm(DnfPackage * pkg)
    std::string getSourcerpm() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.buildtime
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_buildtime(DnfPackage * pkg)
    void getBuildTime() const;

    std::string getBuildHost() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.packager
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_packager(DnfPackage * pkg)
    std::string getPackager() const;

    std::string getVendor() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.url
    std::string getUrl() const;

    // TODO: getBugUrl() not possible due to lack of support in libsolv and metadata?

    /// @replaces dnf:dnf/package.py:attribute:Package.summary
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_summary(DnfPackage * pkg)
    std::string getSummary() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.description
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_description(DnfPackage * pkg)
    std::string getDescription() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.files
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_files(DnfPackage * pkg)
    /// TODO: files, directories, info about ghost etc. - existing implementation returns incomplete data
    void getFiles() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.changelogs
    /// @replaces libdnf:libdnf/hy-package-private.hpp:function:dnf_package_get_changelogs(DnfPackage * pkg)
    void getChangelogs() const;

    // DEPENDENCIES

    /// @replaces dnf:dnf/package.py:attribute:Package.provides
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_provides(DnfPackage * pkg)
    void getProvides() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.requires
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_requires(DnfPackage * pkg)
    void getRequires() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.requires_pre
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_requires_pre(DnfPackage * pkg)
    void getRequiresPre() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.conflicts
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_conflicts(DnfPackage * pkg)
    void getConflicts() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.obsoletes
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_obsoletes(DnfPackage * pkg)
    void getObsoletes() const;

    // WEAK DEPENDENCIES

    /// @replaces dnf:dnf/package.py:attribute:Package.recommends
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_recommends(DnfPackage * pkg)
    void getRecommends() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.suggests
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_suggests(DnfPackage * pkg)
    void getSuggests() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.enhances
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_enhances(DnfPackage * pkg)
    void getEnhances() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.supplements
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_supplements(DnfPackage * pkg)
    void getSupplements() const;


    // REPODATA

    /// @replaces dnf:dnf/package.py:attribute:Package.repo
    /// @replaces dnf:dnf/package.py:attribute:Package.repoid
    /// @replaces dnf:dnf/package.py:attribute:Package.reponame
    /// @replaces libdnf:libdnf/dnf-package.h:function:dnf_package_get_repo(DnfPackage * pkg)
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_reponame(DnfPackage * pkg)
    void getRepo() const;

    // SYSTEM

    /// For an installed package, return repoid of repo from the package was installed.
    /// For an available package, return an empty string.
    ///
    /// @replaces dnf:dnf/package.py:attribute:Package.ui_from_repo
    /// @replaces libdnf:libdnf/dnf-package.h:function:dnf_package_get_origin(DnfPackage * pkg)
    void getFromRepoId() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.reason
    void getReason() const;

    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_installtime(DnfPackage * pkg)
    void getInstallTime() const;

private:
};


}  // namespace libdnf::rpm
