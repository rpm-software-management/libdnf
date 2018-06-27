/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
*
* Copyright (C) 2013-2015 Richard Hughes <richard@hughsie.com>
*
* Licensed under the GNU Lesser General Public License Version 2.1
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or(at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
*/
/**
* SECTION:dnf-goal
* @short_description: Helper methods for dealing with hawkey packages.
* @include: libdnf.h
* @stability: Unstable
*
* These methods make it easier to get and set extra data on a package.
*/


#include <cstdlib>

#include <librepo/librepo.h>
#include <memory>

#include "repo/RpmPackage.hpp"
#include "dnf-package.h"
#include "dnf-types.h"
#include "dnf-utils.h"
#include "hy-util.h"
#include "repo/solvable/Dependency.hpp"
#include "repo/solvable/DependencyContainer.hpp"
#include "libdnf/utils/utils.hpp"


/**
* dnf_package_get_filename:
* @pkg: a #DnfPackage *instance.
*
* Gets the package filename.
*
* Returns: absolute filename, or %NULL
*
* Since: 0.1.0
**/
const char *
dnf_package_get_filename(DnfPackage *pkg)
{
  return pkg->getFilename().c_str();
}

/**
* dnf_package_get_origin:
* @pkg: a #DnfPackage *instance.
*
* Gets the package origin.
*
* Returns: the package origin, or %NULL
*
* Since: 0.1.0
**/
const char *
dnf_package_get_origin(DnfPackage *pkg)
{
    return "";
}

/**
* dnf_package_get_pkgid:
* @pkg: a #DnfPackage *instance.
*
* Gets the pkgid, which is the SHA hash of the package header.
*
* Returns: pkgid string, or NULL
*
* Since: 0.1.0
**/
const char *
dnf_package_get_pkgid(DnfPackage *pkg)
{
  return reinterpret_cast<const char *>(pkg->getPackageHeaderHash());
}

/**
* dnf_package_set_pkgid:
* @pkg: a #DnfPackage *instance.
* @pkgid: pkgid, e.g. "e6e3b2b10c1ef1033769147dbd1bf851c7de7699"
*
* Sets the package pkgid, which is the SHA hash of the package header.
*
* Since: 0.1.8
**/
void
dnf_package_set_pkgid(DnfPackage *pkg, const char *pkgid)
{
  pkg->setPackageHeaderHash((unsigned char *) pkgid);
}

/**
* dnf_package_get_package_id:
* @pkg: a #DnfPackage *instance.
*
* Gets the package-id as used by PackageKit.
*
* Returns: the package_id string, or %NULL, e.g. "hal;2:0.3.4;i386;installed:fedora"
*
* Since: 0.1.0
**/
const char *
dnf_package_get_package_id(DnfPackage *pkg)
{
  return pkg->getPackageId().c_str();
}

/**
* dnf_package_get_cost:
* @pkg: a #DnfPackage *instance.
*
* Returns the cost of the repo that provided the package.
*
* Returns: the cost, where higher is more expensive, default 1000
*
* Since: 0.1.0
**/
unsigned int
dnf_package_get_cost(DnfPackage *pkg)
{
  return pkg->getCost();
}

/**
* dnf_package_set_filename:
* @pkg: a #DnfPackage *instance.
* @filename: absolute filename.
*
* Sets the file on disk that matches the package repo.
*
* Since: 0.1.0
**/
void
dnf_package_set_filename(DnfPackage *pkg, const char *filename)
{}

/**
* dnf_package_set_origin:
* @pkg: a #DnfPackage *instance.
* @origin: origin, e.g. "fedora"
*
* Sets the package origin repo.
*
* Since: 0.1.0
**/
void
dnf_package_set_origin(DnfPackage *pkg, const char *origin)
{}

/**
* dnf_package_set_repo:
* @pkg: a #DnfPackage *instance.
* @repo: a #DnfRepo.
*
* Sets the repo the package was created from.
*
* Since: 0.1.0
**/
void
dnf_package_set_repo(DnfPackage *pkg, DnfRepo *repo)
{
  pkg->setRepo(std::shared_ptr<DnfRepo>(repo));
}

/**
* dnf_package_get_repo:
* @pkg: a #DnfPackage *instance.
*
* Gets the repo the package was created from.
*
* Returns: a #DnfRepo or %NULL
*
* Since: 0.1.0
**/
DnfRepo *
dnf_package_get_repo(DnfPackage *pkg)
{
  return pkg->getRepo().get();
}

/**
* dnf_package_get_info:
* @pkg: a #DnfPackage *instance.
*
* Gets the info enum assigned to the package.
*
* Returns: #DnfPackageInfo value
*
* Since: 0.1.0
**/
int
dnf_package_get_info(DnfPackage *pkg)
{
    return 0;
}

/**
* dnf_package_get_action:
* @pkg: a #DnfPackage *instance.
*
* Gets the action assigned to the package, i.e. what is going to be performed.
*
* Returns: a #DnfStateAction
*
* Since: 0.1.0
*/
DnfStateAction
dnf_package_get_action(DnfPackage *pkg)
{
  return pkg->getAction();
}

/**
* dnf_package_set_info:
* @pkg: a #DnfPackage *instance.
* @info: the info flags.
*
* Sets the info flags for the package.
*
* Since: 0.1.0
**/
void
dnf_package_set_info(DnfPackage *pkg, int info)
{}

/**
* dnf_package_set_action:
* @pkg: a #DnfPackage *instance.
* @action: the #DnfStateAction for the package.
*
* Sets the action for the package, i.e. what is going to be performed.
*
* Since: 0.1.0
*/
void
dnf_package_set_action(DnfPackage *pkg, DnfStateAction action)
{
  pkg->setAction(action);
}

/**
 * dnf_package_is_gui:
 * @pkg: a #DnfPackage *instance.
 *
 * Returns: %TRUE if the package is a GUI package
 *
 * Since: 0.1.0
 **/
bool
dnf_package_is_gui(DnfPackage *pkg)
{
    std::shared_ptr<libdnf::DependencyContainer> requires = pkg->getRequires();
    for (int i = 0; i < requires->count(); i++) {
        auto dependency = requires->get(i);
        std::string dependencyStr = dependency->getName();
        if (dependencyStr == "libgtk" ||
            dependencyStr == "libQt5Gui.so" ||
            dependencyStr == "libQtGui.so" ||
            dependencyStr == "libqt-mt.so") {
            return true;
        }
    }

    return false;
}

/**
 * dnf_package_is_devel:
 * @pkg: a #DnfPackage *instance.
 *
 * Returns: %TRUE if the package is a development package
 *
 * Since: 0.1.0
 **/
bool
dnf_package_is_devel(DnfPackage *pkg)
{
    std::string name = pkg->getName();
    return string::endsWith(name, "-debuginfo") ||
           string::endsWith(name,"-devel") ||
           string::endsWith(name,"-static") ||
           string::endsWith(name,"-libs");
}

/**
 * dnf_package_is_downloaded:
 * @pkg: a #DnfPackage *instance.
 *
 * Returns: %TRUE if the package is already downloaded
 *
 * Since: 0.1.0
 **/
bool
dnf_package_is_downloaded(DnfPackage *pkg)
{
    if (pkg->isInstalled())
        return false;

    auto filename = pkg->getFilename();
    if (filename.empty()) {
        throw "Failed to get cache filename for " + std::string(pkg->getName());
    }

    return filesystem::exists(filename);
}

/**
 * dnf_package_is_installonly:
 * @pkg: a #DnfPackage *instance.
 *
 * Returns: %TRUE if the package can be installed more than once
 *
 * Since: 0.1.0
 */
bool
dnf_package_is_installonly(DnfPackage *pkg)
{
    const gchar **installonlyPkgs = dnf_context_get_installonly_pkgs(nullptr);

    for (int i = 0; installonlyPkgs[i] != nullptr; i++) {
        if (pkg->getName() == std::string(installonlyPkgs[i]))
            return true;
    }

    return false;
}

/**
 * dnf_package_check_filename:
 * @pkg: a #DnfPackage *instance.
 * @valid: Set to %TRUE if the package is valid.
 * @error: a #GError or %NULL..
 *
 * Checks the package is already downloaded and valid.
 *
 * Returns: %TRUE if the package was checked successfully
 *
 * Since: 0.1.0
 **/
bool
dnf_package_check_filename(DnfPackage *pkg, bool *valid, GError **error)
{
    *valid = pkg->checkFilename();
    return *valid;
}

/**
 * dnf_package_download:
 * @pkg: a #DnfPackage *instance.
 * @directory: destination directory, or %NULL for the cachedir.
 * @state: the #DnfState.
 * @error: a #GError or %NULL..
 *
 * Downloads the package.
 *
 * Returns: the complete filename of the downloaded file
 *
 * Since: 0.1.0
 **/
char *
dnf_package_download(DnfPackage *pkg,
                     const char *directory,
                     DnfState *state,
                     GError **error)
{
    auto repo = pkg->getRepo();
    if (repo == nullptr) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "package repo is not set");
        return nullptr;
    }

    char *path = dnf_repo_download_package(repo.get(), pkg, directory, state, error);
    return path;
}

/**
 * dnf_package_array_download:
 * @packages: an array of packages.
 * @directory: destination directory, or %NULL for the cachedir.
 * @state: the #DnfState.
 * @error: a #GError or %NULL..
 *
 * Downloads an array of packages.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.1.0
 */
bool
dnf_package_array_download(GPtrArray *packages,
                           const char *directory,
                           DnfState *state,
                           GError **error)
{
    std::vector<libdnf::RpmPackage> packagesVector;
    for (unsigned int i = 0; i < packages->len; i++) {
        auto pkg = (DnfPackage *) g_ptr_array_index(packages, i);
        packagesVector.push_back(*pkg);
    }

    libdnf::RpmPackage::download(packagesVector, directory, state);
    return true;
}

/**
 * dnf_package_array_get_download_size:
 * @packages: an array of packages.
 *
 * Gets the download size for an array of packages.
 *
 * Returns: the download size
 *
 * Since: 0.2.3
 */
unsigned long
dnf_package_array_get_download_size(GPtrArray *packages)
{
    std::vector<libdnf::RpmPackage> packagesVector;
    for (unsigned int i = 0; i < packages->len; i++) {
        auto pkg = (DnfPackage *) g_ptr_array_index(packages, i);
        packagesVector.push_back(*pkg);
    }

    return libdnf::RpmPackage::getDownloadSize(packagesVector);
}
