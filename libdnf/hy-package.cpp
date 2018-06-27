/*
 * Copyright (C) 2012-2014 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * SECTION:dnf-package
 * @short_description: Package
 * @include: libdnf.h
 * @stability: Unstable
 *
 * An object representing a package in the system.
 *
 * See also: #DnfContext
 */


#include <solv/evr.h>
#include <solv/pool.h>
#include <solv/repo.h>
#include <tuple>

#include "dnf-advisory-private.hpp"
#include "dnf-packagedelta-private.hpp"
#include "dnf-sack-private.hpp"
#include "hy-iutil-private.hpp"

#include "libdnf/repo/RpmPackage.hpp"

/**
 * dnf_package_new:
 *
 * Creates a new #DnfPackage.
 *
 * Returns:(transfer full): a #DnfPackage
 *
 * Since: 0.7.0
 **/
DnfPackage *
dnf_package_new(DnfSack *sack, Id id)
{
    return new libdnf::RpmPackage(sack, id);
}

/**
 * dnf_package_get_id: (skip):
 * @pkg: a #DnfPackage instance.
 *
 * Gets the internal ID used to identify the package in the pool.
 *
 * Returns: an integer, or 0 for unknown
 *
 * Since: 0.7.0
 */
Id
dnf_package_get_id(DnfPackage *pkg)
{
    return pkg->getId();
}

/**
 * dnf_package_installed:
 * @pkg: a #DnfPackage instance.
 *
 * Tests if the package is installed.
 *
 * Returns: %TRUE if installed
 *
 * Since: 0.7.0
 */
bool
dnf_package_installed(DnfPackage *pkg)
{
    return pkg->isInstalled();
}

/**
 * dnf_package_cmp:
 * @pkg1: a #DnfPackage instance.
 * @pkg2: another #DnfPackage instance.
 *
 * Compares two packages.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
int
dnf_package_cmp(DnfPackage *pkg1, DnfPackage *pkg2)
{
    return pkg1->compare(*pkg2);
}

/**
 * dnf_package_evr_cmp:
 * @pkg1: a #DnfPackage instance.
 * @pkg2: another #DnfPackage instance.
 *
 * Compares two packages, only using the EVR values.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
int
dnf_package_evr_cmp(DnfPackage *pkg1, DnfPackage *pkg2)
{
    return pkg1->evrCompare(*pkg2);
}

/**
 * dnf_package_get_location:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the location (XXX??) for the package. Note that the returned string has
 * an undefined lifetime and may become invalid at a later time. You should copy
 * the string if storing it into a long-lived data structure.
 *
 * Returns: (transfer none): string
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_location(DnfPackage *pkg)
{
    return pkg->getLocation();
}

/**
 * dnf_package_get_baseurl:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the baseurl for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_baseurl(DnfPackage *pkg)
{
    return pkg->getBaseurl();
}

/**
 * dnf_package_get_nevra:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the package NEVRA. Note that the returned string has an undefined
 * lifetime and may become invalid at a later time. You should copy the string
 * if storing it into a long-lived data structure.
 *
 * Returns: (transfer none): a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_nevra(DnfPackage *pkg)
{
    return pkg->getNevra();
}

/**
 * dnf_package_get_sourcerpm:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the source RPM for the package.
 *
 * Returns: (transfer none): a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_sourcerpm(DnfPackage *pkg)
{
    return pkg->getSourceRpm();
}

/**
 * dnf_package_get_version:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the version for the package.
 *
 * Returns: (transfer none): a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_version(DnfPackage *pkg)
{
    return pkg->getVersion();
}

/**
 * dnf_package_get_release:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the release for the package.
 *
 * Returns: (transfer none): a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_release(DnfPackage *pkg)
{
    return pkg->getRelease();
}

/**
 * dnf_package_get_name:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the name for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_name(DnfPackage *pkg)
{
    return pkg->getName();
}

/**
 * dnf_package_get_packager:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the packager for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_packager(DnfPackage *pkg)
{
    return pkg->getPackager();
}

/**
 * dnf_package_get_arch:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the architecture for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_arch(DnfPackage *pkg)
{
    return pkg->getArch();
}

/**
 * dnf_package_get_chksum:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the checksum for the package.
 *
 * Returns: raw checksum bytes
 *
 * Since: 0.7.0
 */
const unsigned char *
dnf_package_get_chksum(DnfPackage *pkg, int *type)
{
    auto checksum = pkg->getChecksum();
    *type = std::get<1>(checksum);
    return std::get<0>(checksum);
}

/**
 * dnf_package_get_hdr_chksum:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the SHA1 checksum of the packages header.
 * This is only set for packages in the rpmdb.
 *
 * Returns: raw checksum bytes
 *
 * Since: 0.7.0
 */
const unsigned char *
dnf_package_get_hdr_chksum(DnfPackage *pkg, int *type)
{
    auto checksum = pkg->getHeaderChecksum();
    *type = std::get<1>(checksum);
    return std::get<0>(checksum);
}

/**
 * dnf_package_get_description:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the description for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_description(DnfPackage *pkg)
{
    return pkg->getDescription();
}

/**
 * dnf_package_get_evr:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the EVR for the package.
 *
 * Returns: a string
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_evr(DnfPackage *pkg)
{
    return pkg->getEvr();
}

/**
 * dnf_package_get_group:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the group for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_group(DnfPackage *pkg)
{
  return pkg->getGroup();
}

/**
 * dnf_package_get_license:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the license for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_license(DnfPackage *pkg)
{
    return pkg->getLicense();
}

/**
 * dnf_package_get_reponame:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the reponame for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_reponame(DnfPackage *pkg)
{
    return pkg->getReponame();
}

/**
 * dnf_package_get_summary:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the summary for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_summary(DnfPackage *pkg)
{
    return pkg->getSummary();
}

/**
 * dnf_package_get_url:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the url for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_url(DnfPackage *pkg)
{
    return pkg->getUrl();
}

/**
 * dnf_package_get_downloadsize:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the download size for the package.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
unsigned long
dnf_package_get_downloadsize(DnfPackage *pkg)
{
    return pkg->getDownloadSize();
}

/**
 * dnf_package_get_epoch:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the epoch for the package.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
unsigned long
dnf_package_get_epoch(DnfPackage *pkg)
{
    return pkg->getEpoch();
}

/**
 * dnf_package_get_hdr_end:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the header end index for the package.
 *
 * Returns: an index, or 0 for not known
 *
 * Since: 0.7.0
 */
unsigned long
dnf_package_get_hdr_end(DnfPackage *pkg)
{
    return pkg->getHeaderEndIndex();
}

/**
 * dnf_package_get_installsize:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the installed size for the package.
 *
 * Returns: size in bytes
 *
 * Since: 0.7.0
 */
unsigned long
dnf_package_get_installsize(DnfPackage *pkg)
{
    return pkg->getInstallSize();
}

/**
 * dnf_package_get_buildtime:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the build time for the package.
 *
 * Returns: UNIX time
 *
 * Since: 0.7.0
 */
unsigned long
dnf_package_get_buildtime(DnfPackage *pkg)
{
    return pkg->getBuildTime();
}

/**
 * dnf_package_get_installtime:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the install time for the package.
 *
 * Returns: UNIX time
 *
 * Since: 0.7.0
 */
unsigned long
dnf_package_get_installtime(DnfPackage *pkg)
{
    return pkg->getInstallTime();
}

/**
 * dnf_package_get_medianr:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the media number for the package.
 *
 * Returns: integer value
 *
 * Since: 0.7.0
 */
unsigned long
dnf_package_get_medianr(DnfPackage *pkg)
{
    return pkg->getMediaNumber();
}

/**
 * dnf_package_get_rpmdbid:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the RPMDB ID for the package.
 *
 * Returns: an ID, or 0 for not known
 *
 * Since: 0.7.0
 */
unsigned long
dnf_package_get_rpmdbid(DnfPackage *pkg)
{
    return pkg->getRpmdbId();
}

/**
 * dnf_package_get_size:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the size for the package.
 *
 * Returns: size in bytes
 *
 * Since: 0.7.0
 */
unsigned long
dnf_package_get_size(DnfPackage *pkg)
{
    return pkg->getSize();
}

/**
 * dnf_package_get_conflicts:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the conflicts for the package.
 *
 * Returns: A #DnfReldepList
 *
 * Since: 0.7.0
 */
DnfReldepList *
dnf_package_get_conflicts(DnfPackage *pkg)
{
    return pkg->getConflicts().get();
}

/**
 * dnf_package_get_enhances:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the enhances for the package.
 *
 * Returns: A #DnfReldepList
 *
 * Since: 0.7.0
 */
DnfReldepList *
dnf_package_get_enhances(DnfPackage *pkg)
{
    return pkg->getEnhances().get();
}

/**
 * dnf_package_get_obsoletes:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the obsoletes for the package.
 *
 * Returns: A #DnfReldepList
 *
 * Since: 0.7.0
 */
DnfReldepList *
dnf_package_get_obsoletes(DnfPackage *pkg)
{
    return pkg->getObsoletes().get();
}

/**
 * dnf_package_get_provides:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the provides for the package.
 *
 * Returns: A #DnfReldepList
 *
 * Since: 0.7.0
 */
DnfReldepList *
dnf_package_get_provides(DnfPackage *pkg)
{
    return pkg->getProvides().get();
}

/**
 * dnf_package_get_recommends:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the recommends for the package.
 *
 * Returns: A #DnfReldepList
 *
 * Since: 0.7.0
 */
DnfReldepList *
dnf_package_get_recommends(DnfPackage *pkg)
{
    return pkg->getRecommends().get();
}

/**
 * dnf_package_get_requires:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the requires for the package.
 *
 * Returns: A #DnfReldepList
 *
 * Since: 0.7.0
 */
DnfReldepList *
dnf_package_get_requires(DnfPackage *pkg)
{
    return pkg->getRequires().get();
}

/**
 * dnf_package_get_requires_pre:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the Requires(pre) for the package.
 *
 * Returns: A #DnfReldepList
 *
 * Since: 0.7.0
 */
DnfReldepList *
dnf_package_get_requires_pre (DnfPackage *pkg)
{
    return pkg->getRequiresPre().get();
}

/**
 * dnf_package_get_suggests:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the suggests for the package.
 *
 * Returns: A #DnfReldepList
 *
 * Since: 0.7.0
 */
DnfReldepList *
dnf_package_get_suggests(DnfPackage *pkg)
{
    return pkg->getSuggests().get();
}

/**
 * dnf_package_get_supplements:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the supplements for the package.
 *
 * Returns: A #DnfReldepList
 *
 * Since: 0.7.0
 */
DnfReldepList *
dnf_package_get_supplements(DnfPackage *pkg)
{
    return pkg->getSupplements().get();
}

/**
 * dnf_package_get_files:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the files contained in the package.
 *
 * Returns: (transfer full): the file list
 *
 * Since: 0.7.0
 */
char **
dnf_package_get_files(DnfPackage *pkg)
{
    return pkg->getFiles().data();
}

/**
 * dnf_package_get_advisories:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the list of advisories for the package.
 *
 * Returns: (transfer container) (element-type DnfAdvisory): a list
 *
 * Since: 0.7.0
 */
GPtrArray *
dnf_package_get_advisories(DnfPackage *pkg, int cmp_type)
{
    auto advisories = pkg->getAdvisories(cmp_type);
    GPtrArray *array = g_ptr_array_new();

    for (const auto &advisory : advisories)
        g_ptr_array_add(array, advisory.get());

    return array;
}

/**
 * dnf_package_get_delta_from_evr:
 * @pkg: a #DnfPackage instance.
 * @from_evr: the EVR string of what's installed
 *
 * Gets the package delta for the package given an existing EVR.
 *
 * Returns: a #DnfPackageDelta, or %NULL
 *
 * Since: 0.7.0
 */
DnfPackageDelta *
dnf_package_get_delta_from_evr(DnfPackage *pkg, const char *from_evr)
{
    return pkg->getDelta(from_evr).get();
}
