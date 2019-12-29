#pragma once


// forward declarations
namespace libdnf::rpm {
class Package;
}  // namespace libdnf::rpm


#include <string>


namespace libdnf::rpm {


/// @replaces dnf:dnf/package.py:class:Package
class Package {
public:
    /// @replaces dnf:dnf/package.py:attribute:Package.name
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_name(DnfPackage * pkg)
    std::string get_name() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.e
    /// @replaces dnf:dnf/package.py:attribute:Package.epoch
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_epoch(DnfPackage * pkg)
    uint32_t get_epoch() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.v
    /// @replaces dnf:dnf/package.py:attribute:Package.version
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_version(DnfPackage * pkg)
    std::string get_version() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.r
    /// @replaces dnf:dnf/package.py:attribute:Package.release
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_release(DnfPackage * pkg)
    std::string get_release() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.a
    /// @replaces dnf:dnf/package.py:attribute:Package.arch
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_arch(DnfPackage * pkg)
    std::string get_arch() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.evr
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_evr(DnfPackage * pkg)
    std::string get_evr() const;

    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_nevra(DnfPackage * pkg)
    std::string get_nevra() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.group
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_group(DnfPackage * pkg)
    std::string get_group() const;

    /// If package is installed, return get_install_size(). Return get_download_size() otherwise.
    /// @replaces dnf:dnf/package.py:attribute:Package.size
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_size(DnfPackage * pkg)
    std::size_t get_size() const;

    /// Return size of an RPM package: <size package="..."/>
    /// @replaces dnf:dnf/package.py:attribute:Package.downloadsize
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_downloadsize(DnfPackage * pkg)
    std::size_t get_download_size() const;

    /// Return size of an RPM package installed on a system: <size installed="..."/>
    /// @replaces dnf:dnf/package.py:attribute:Package.installsize
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_installsize(DnfPackage * pkg)
    std::size_t get_install_size() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.license
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_license(DnfPackage * pkg)
    std::string get_license() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.sourcerpm
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_sourcerpm(DnfPackage * pkg)
    std::string get_sourcerpm() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.buildtime
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_buildtime(DnfPackage * pkg)
    void get_build_time() const;

    std::string get_build_host() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.packager
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_packager(DnfPackage * pkg)
    std::string get_packager() const;

    std::string get_vendor() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.url
    std::string get_url() const;

    // TODO: getBugUrl() not possible due to lack of support in libsolv and metadata?

    /// @replaces dnf:dnf/package.py:attribute:Package.summary
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_summary(DnfPackage * pkg)
    std::string get_summary() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.description
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_description(DnfPackage * pkg)
    std::string get_description() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.files
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_files(DnfPackage * pkg)
    /// TODO: files, directories, info about ghost etc. - existing implementation returns incomplete data
    void get_files() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.changelogs
    /// @replaces libdnf:libdnf/hy-package-private.hpp:function:dnf_package_get_changelogs(DnfPackage * pkg)
    void get_changelogs() const;

    // DEPENDENCIES

    /// @replaces dnf:dnf/package.py:attribute:Package.provides
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_provides(DnfPackage * pkg)
    void get_provides() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.requires
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_requires(DnfPackage * pkg)
    void get_requires() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.requires_pre
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_requires_pre(DnfPackage * pkg)
    void get_requires_pre() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.conflicts
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_conflicts(DnfPackage * pkg)
    void get_conflicts() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.obsoletes
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_obsoletes(DnfPackage * pkg)
    void get_obsoletes() const;

    // WEAK DEPENDENCIES

    /// @replaces dnf:dnf/package.py:attribute:Package.recommends
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_recommends(DnfPackage * pkg)
    void get_recommends() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.suggests
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_suggests(DnfPackage * pkg)
    void get_suggests() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.enhances
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_enhances(DnfPackage * pkg)
    void get_enhances() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.supplements
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_supplements(DnfPackage * pkg)
    void get_supplements() const;


    // REPODATA

    /// @replaces dnf:dnf/package.py:attribute:Package.repo
    /// @replaces dnf:dnf/package.py:attribute:Package.repoid
    /// @replaces dnf:dnf/package.py:attribute:Package.reponame
    /// @replaces libdnf:libdnf/dnf-package.h:function:dnf_package_get_repo(DnfPackage * pkg)
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_reponame(DnfPackage * pkg)
    void get_repo() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.baseurl
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_baseurl(DnfPackage * pkg)
    std::string get_baseurl() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.location
    /// @replaces dnf:dnf/package.py:attribute:Package.relativepath
    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_location(DnfPackage * pkg)
    std::string get_location() const;


    // SYSTEM

    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_installed(DnfPackage * pkg)
    /// @replaces dnf:dnf/package.py:attribute:Package.installed
    bool is_installed() const;

    /// @replaces dnf:dnf/package.py:method:Package.localPkg(self)
    /// @replaces libdnf:libdnf/dnf-package.h:function:dnf_package_is_local(DnfPackage * pkg)
    bool is_local() const;

    /// For an installed package, return repoid of repo from the package was installed.
    /// For an available package, return an empty string.
    ///
    /// @replaces dnf:dnf/package.py:attribute:Package.ui_from_repo
    /// @replaces libdnf:libdnf/dnf-package.h:function:dnf_package_get_origin(DnfPackage * pkg)
    void get_from_repo_id() const;

    /// @replaces dnf:dnf/package.py:attribute:Package.reason
    void get_reason() const;

    /// @replaces libdnf:libdnf/hy-package.h:function:dnf_package_get_installtime(DnfPackage * pkg)
    void get_install_time() const;

protected:
    /// @replaces libdnf:libdnf/dnf-package.h:function:dnf_package_get_package_id(DnfPackage * pkg)
    std::string get_id() const;
};


}  // namespace libdnf::rpm
