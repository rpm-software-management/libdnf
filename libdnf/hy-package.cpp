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


#include <stdlib.h>
#include <solv/evr.h>
#include <solv/pool.h>
#include <solv/repo.h>
#include <solv/queue.h>

#include "dnf-advisory-private.hpp"
#include "dnf-packagedelta-private.hpp"
#include "dnf-sack-private.hpp"
#include "hy-iutil-private.hpp"
#include "hy-package-private.hpp"
#include "hy-repo-private.hpp"
#include "repo/solvable/DependencyContainer.hpp"

#define BLOCK_SIZE 31

typedef struct
{
    gboolean         loaded;
    Id               id;
    DnfSack         *sack;
} DnfPackagePrivate;

G_DEFINE_TYPE_WITH_PRIVATE(DnfPackage, dnf_package, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (static_cast<DnfPackagePrivate *>(dnf_package_get_instance_private (o)))

/**
 * dnf_package_finalize:
 **/
static void
dnf_package_finalize(GObject *object)
{
    G_OBJECT_CLASS(dnf_package_parent_class)->finalize(object);
}

/**
 * dnf_package_init:
 **/
static void
dnf_package_init(DnfPackage *package)
{
}

/**
 * dnf_package_class_init:
 **/
static void
dnf_package_class_init(DnfPackageClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_package_finalize;
}


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
    auto pkg = DNF_PACKAGE(g_object_new(DNF_TYPE_PACKAGE, NULL));
    auto priv = GET_PRIVATE(pkg);
    priv->sack = sack;
    priv->id = id;
    return pkg;
}


/* internal */
static Solvable *
get_solvable(DnfPackage *pkg)
{
    DnfPackagePrivate *priv = GET_PRIVATE(pkg);
    return pool_id2solvable(dnf_package_get_pool(pkg), priv->id);
}

/**
 * dnf_package_get_pool: (skip)
 * @pkg: a #DnfPackage instance.
 *
 * Gets the pool used for storage.
 *
 * Returns: (transfer none): a %Pool
 *
 * Since: 0.7.0
 */
Pool *
dnf_package_get_pool(DnfPackage *pkg)
{
    DnfPackagePrivate *priv = GET_PRIVATE(pkg);
    return dnf_sack_get_pool(priv->sack);
}

static guint64
lookup_num(DnfPackage *pkg, unsigned type)
{
    Solvable *s = get_solvable(pkg);
    repo_internalize_trigger(s->repo);
    return solvable_lookup_num(s, type, 0);
}

static DnfReldepList *
reldeps_for(DnfPackage *pkg, Id type)
{
    Solvable *s = get_solvable(pkg);
    DnfReldepList *reldeplist;
    Queue q, q_final;
    Id marker = -1;
    Id solv_type = type;
    Id rel_id;

    if (type == SOLVABLE_REQUIRES)
        marker = 0;

    if (type == SOLVABLE_PREREQMARKER) {
        solv_type = SOLVABLE_REQUIRES;
        marker = 1;
    }
    queue_init(&q);
    queue_init(&q_final);
    solvable_lookup_deparray(s, solv_type, &q, marker);

    for (int i = 0; i < q.count; i++) {
        rel_id = q.elements[i];
        if (rel_id != SOLVABLE_PREREQMARKER)
            queue_push(&q_final, rel_id);
    }

    reldeplist = new DependencyContainer(dnf_package_get_sack(pkg), q_final);

    queue_free(&q);
    queue_free(&q_final);
    return reldeplist;
}

/**
 * dnf_package_clone:
 * @pkg: a #DnfPackage instance.
 *
 * Clones the package.
 *
 * Returns: a #DnfPackage
 *
 * Since: 0.7.0
 */
DnfPackage *
dnf_package_clone(DnfPackage *pkg)
{
    return dnf_package_new(dnf_package_get_sack(pkg), dnf_package_get_id(pkg));
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
    DnfPackagePrivate *priv = GET_PRIVATE(pkg);
    return priv->id;
}

/**
 * dnf_package_get_sack:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the package sack for the package.
 *
 * Returns: a #DnfSack, or %NULL
 *
 * Since: 0.7.0
 */
DnfSack *
dnf_package_get_sack(DnfPackage *pkg)
{
    DnfPackagePrivate *priv = GET_PRIVATE(pkg);
    return priv->sack;
}

/**
 * dnf_package_from_solvable: (skip):
 * @pkg: a #DnfPackage instance.
 *
 * Creates a package from a solvable.
 *
 * Returns: a #HyPackage
 *
 * Since: 0.7.0
 */
DnfPackage *
dnf_package_from_solvable(DnfSack *sack, Solvable *s)
{
    if (!s)
        return NULL;

    Id p = s - s->repo->pool->solvables;
    return dnf_package_new(sack, p);
}

/**
 * dnf_package_get_identical:
 * @pkg1: a #DnfPackage instance.
 * @pkg2: another #DnfPackage instance.
 *
 * Tests two packages for equality.
 *
 * Returns: %TRUE if the packages are the same
 *
 * Since: 0.7.0
 */
gboolean
dnf_package_get_identical(DnfPackage *pkg1, DnfPackage *pkg2)
{
    DnfPackagePrivate *priv1 = GET_PRIVATE(pkg1);
    DnfPackagePrivate *priv2 = GET_PRIVATE(pkg2);
    return priv1->id == priv2->id;
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
gboolean
dnf_package_installed(DnfPackage *pkg)
{
    Solvable *s = get_solvable(pkg);
    return (dnf_package_get_pool(pkg)->installed == s->repo);
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
    Pool *pool1 = dnf_package_get_pool(pkg1);
    Pool *pool2 = dnf_package_get_pool(pkg2);
    Solvable *s1 = pool_id2solvable(pool1, dnf_package_get_id(pkg1));
    Solvable *s2 = pool_id2solvable(pool2, dnf_package_get_id(pkg2));
    const char *str1 = pool_id2str(pool1, s1->name);
    const char *str2 = pool_id2str(pool2, s2->name);
    int ret = strcmp(str1, str2);
    if (ret)
        return ret;

    ret = dnf_package_evr_cmp(pkg1, pkg2);
    if (ret)
        return ret;

    str1 = pool_id2str(pool1, s1->arch);
    str2 = pool_id2str(pool2, s2->arch);
    return strcmp(str1, str2);
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
    Pool *pool1 = dnf_package_get_pool(pkg1);
    Pool *pool2 = dnf_package_get_pool(pkg2);
    Solvable *s1 = get_solvable(pkg1);
    Solvable *s2 = get_solvable(pkg2);
    const char *str1 = pool_id2str(pool1, s1->evr);
    const char *str2 = pool_id2str(pool2, s2->evr);

    return pool_evrcmp_str(dnf_package_get_pool(pkg1), str1, str2, EVRCMP_COMPARE);
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
    Solvable *s = get_solvable(pkg);
    if (s->repo) {
        repo_internalize_trigger(s->repo);
    }
    return solvable_get_location(s, NULL);
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
    Solvable *s = get_solvable(pkg);
    return solvable_lookup_str(s, SOLVABLE_MEDIABASE);
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
    Solvable *s = get_solvable(pkg);
    return pool_solvable2str(dnf_package_get_pool(pkg), s);
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
    Solvable *s = get_solvable(pkg);
    repo_internalize_trigger(s->repo);
    return solvable_lookup_sourcepkg(s);
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
    char *e, *v, *r;
    pool_split_evr(dnf_package_get_pool(pkg), dnf_package_get_evr(pkg), &e, &v, &r);
    return v;
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
    char *e, *v, *r;
    pool_split_evr(dnf_package_get_pool(pkg), dnf_package_get_evr(pkg), &e, &v, &r);
    return r;
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
    Pool *pool = dnf_package_get_pool(pkg);
    return pool_id2str(pool, get_solvable(pkg)->name);
}

/**
 * dnf_package_get_packager:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
dnf_package_get_packager(DnfPackage *pkg)
{
    return solvable_lookup_str(get_solvable(pkg), SOLVABLE_PACKAGER);
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
    Pool *pool = dnf_package_get_pool(pkg);
    return pool_id2str(pool, get_solvable(pkg)->arch);
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
    Solvable *s = get_solvable(pkg);
    const unsigned char* ret;

    repo_internalize_trigger(s->repo);
    ret = solvable_lookup_bin_checksum(s, SOLVABLE_CHECKSUM, type);
    if (ret)
        *type = checksumt_l2h(*type);
    return ret;
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
    Solvable *s = get_solvable(pkg);
    const unsigned char *ret;

    repo_internalize_trigger(s->repo);
    ret = solvable_lookup_bin_checksum(s, SOLVABLE_HDRID, type);
    if (ret)
        *type = checksumt_l2h(*type);
    return ret;
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
    return solvable_lookup_str(get_solvable(pkg), SOLVABLE_DESCRIPTION);
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
    Pool *pool = dnf_package_get_pool(pkg);
    return pool_id2str(pool, get_solvable(pkg)->evr);
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
  return solvable_lookup_str(get_solvable(pkg), SOLVABLE_GROUP);
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
    return solvable_lookup_str(get_solvable(pkg), SOLVABLE_LICENSE);
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
    Solvable *s = get_solvable(pkg);
    return s->repo->name;
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
    Solvable *s = get_solvable(pkg);
    return solvable_lookup_str(s, SOLVABLE_SUMMARY);
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
    return solvable_lookup_str(get_solvable(pkg), SOLVABLE_URL);
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
guint64
dnf_package_get_downloadsize(DnfPackage *pkg)
{
    return lookup_num(pkg, SOLVABLE_DOWNLOADSIZE);
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
guint64
dnf_package_get_epoch(DnfPackage *pkg)
{
    return pool_get_epoch(dnf_package_get_pool(pkg), dnf_package_get_evr(pkg));
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
guint64
dnf_package_get_hdr_end(DnfPackage *pkg)
{
    return lookup_num(pkg, SOLVABLE_HEADEREND);
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
guint64
dnf_package_get_installsize(DnfPackage *pkg)
{
    return lookup_num(pkg, SOLVABLE_INSTALLSIZE);
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
guint64
dnf_package_get_buildtime(DnfPackage *pkg)
{
    return lookup_num(pkg, SOLVABLE_BUILDTIME);
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
guint64
dnf_package_get_installtime(DnfPackage *pkg)
{
    return lookup_num(pkg, SOLVABLE_INSTALLTIME);
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
guint64
dnf_package_get_medianr(DnfPackage *pkg)
{
    return lookup_num(pkg, SOLVABLE_MEDIANR);
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
guint64
dnf_package_get_rpmdbid(DnfPackage *pkg)
{
    guint64 ret = lookup_num(pkg, RPM_RPMDBID);
    return ret;
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
guint64
dnf_package_get_size(DnfPackage *pkg)
{
    unsigned type = dnf_package_installed(pkg) ? SOLVABLE_INSTALLSIZE :
                                                 SOLVABLE_DOWNLOADSIZE;
    return lookup_num(pkg, type);
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
    return reldeps_for(pkg, SOLVABLE_CONFLICTS);
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
    return reldeps_for(pkg, SOLVABLE_ENHANCES);
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
    return reldeps_for(pkg, SOLVABLE_OBSOLETES);
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
    return reldeps_for(pkg, SOLVABLE_PROVIDES);
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
    return reldeps_for(pkg, SOLVABLE_RECOMMENDS);
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
    return reldeps_for(pkg, SOLVABLE_REQUIRES);
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
    return reldeps_for(pkg, SOLVABLE_PREREQMARKER);
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
    return reldeps_for(pkg, SOLVABLE_SUGGESTS);
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
    return reldeps_for(pkg, SOLVABLE_SUPPLEMENTS);
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
gchar **
dnf_package_get_files(DnfPackage *pkg)
{
    DnfPackagePrivate *priv = GET_PRIVATE(pkg);
    Pool *pool = dnf_package_get_pool(pkg);
    Solvable *s = get_solvable(pkg);
    Dataiterator di;
    GPtrArray *ret = g_ptr_array_new();

    repo_internalize_trigger(s->repo);
    dataiterator_init(&di, pool, s->repo, priv->id, SOLVABLE_FILELIST, NULL,
                      SEARCH_FILES | SEARCH_COMPLETE_FILELIST);
    while (dataiterator_step(&di)) {
        g_ptr_array_add(ret, g_strdup(di.kv.str));
    }
    dataiterator_free(&di);
    g_ptr_array_add(ret, NULL);
    return (gchar**)g_ptr_array_free (ret, FALSE);
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
    Dataiterator di;
    Id evr;
    int cmp;
    DnfAdvisory *advisory;
    Pool *pool = dnf_package_get_pool(pkg);
    DnfSack *sack = dnf_package_get_sack(pkg);
    GPtrArray *advisorylist = g_ptr_array_new();
    Solvable *s = get_solvable(pkg);

    dataiterator_init(&di, pool, 0, 0, UPDATE_COLLECTION_NAME,
                      pool_id2str(pool, s->name), SEARCH_STRING);
    dataiterator_prepend_keyname(&di, UPDATE_COLLECTION);
    while (dataiterator_step(&di)) {
        dataiterator_setpos_parent(&di);
        if (pool_lookup_id(pool, SOLVID_POS, UPDATE_COLLECTION_ARCH) != s->arch)
            continue;
        evr = pool_lookup_id(pool, SOLVID_POS, UPDATE_COLLECTION_EVR);
        if (!evr)
            continue;

        cmp = pool_evrcmp(pool, evr, s->evr, EVRCMP_COMPARE);
        if ((cmp > 0 && (cmp_type & HY_GT)) ||
            (cmp < 0 && (cmp_type & HY_LT)) ||
            (cmp == 0 && (cmp_type & HY_EQ))) {
            advisory = dnf_advisory_new(sack, di.solvid);
            g_ptr_array_add(advisorylist, advisory);
            dataiterator_skip_solvable(&di);
        }
    }
    dataiterator_free(&di);
    return advisorylist;
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
    Pool *pool = dnf_package_get_pool(pkg);
    Solvable *s = get_solvable(pkg);
    DnfPackageDelta *delta = NULL;
    Dataiterator di;
    const char *name = dnf_package_get_name(pkg);

    dataiterator_init(&di, pool, s->repo, SOLVID_META, DELTA_PACKAGE_NAME, name,
                      SEARCH_STRING);
    dataiterator_prepend_keyname(&di, REPOSITORY_DELTAINFO);
    while (dataiterator_step(&di)) {
        dataiterator_setpos_parent(&di);
        if (pool_lookup_id(pool, SOLVID_POS, DELTA_PACKAGE_EVR) != s->evr ||
            pool_lookup_id(pool, SOLVID_POS, DELTA_PACKAGE_ARCH) != s->arch)
            continue;
        const char * base_evr = pool_id2str(pool, pool_lookup_id(pool, SOLVID_POS,
                                                                 DELTA_BASE_EVR));
        if (strcmp(base_evr, from_evr))
            continue;

        // we have the right delta info, set up DnfPackageDelta *and break out:
        delta = dnf_packagedelta_new(pool);

        break;
    }
    dataiterator_free(&di);

    return delta;
}
