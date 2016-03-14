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
 * SECTION:hif-package
 * @short_description: Package
 * @include: libhif.h
 * @stability: Unstable
 *
 * An object representing a package in the system.
 *
 * See also: #HifContext
 */


#include <stdlib.h>
#include <solv/evr.h>
#include <solv/pool.h>
#include <solv/repo.h>
#include <solv/queue.h>

#include "hif-advisory-private.h"
#include "hif-packagedelta-private.h"
#include "hif-sack-private.h"
#include "hy-iutil.h"
#include "hy-package-private.h"
#include "hy-reldep-private.h"
#include "hy-repo-private.h"

#define BLOCK_SIZE 31

typedef struct
{
    gboolean         loaded;
    Id               id;
    HifSack         *sack;
} HifPackagePrivate;

G_DEFINE_TYPE_WITH_PRIVATE(HifPackage, hif_package, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (hif_package_get_instance_private (o))

/**
 * hif_package_finalize:
 **/
static void
hif_package_finalize(GObject *object)
{
    G_OBJECT_CLASS(hif_package_parent_class)->finalize(object);
}

/**
 * hif_package_init:
 **/
static void
hif_package_init(HifPackage *package)
{
}

/**
 * hif_package_class_init:
 **/
static void
hif_package_class_init(HifPackageClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = hif_package_finalize;
}


/**
 * hif_package_new:
 *
 * Creates a new #HifPackage.
 *
 * Returns:(transfer full): a #HifPackage
 *
 * Since: 0.7.0
 **/
HifPackage *
hif_package_new(HifSack *sack, Id id)
{
    HifPackage *pkg;
    HifPackagePrivate *priv;
    pkg = g_object_new(HIF_TYPE_PACKAGE, NULL);
    priv = GET_PRIVATE(pkg);
    priv->sack = sack;
    priv->id = id;
    return HIF_PACKAGE(pkg);
}


/* internal */
static Solvable *
get_solvable(HifPackage *pkg)
{
    HifPackagePrivate *priv = GET_PRIVATE(pkg);
    return pool_id2solvable(hif_package_get_pool(pkg), priv->id);
}

/**
 * hif_package_get_pool: (skip)
 * @pkg: a #HifPackage instance.
 *
 * Gets the pool used for storage.
 *
 * Returns: (transfer none): a %Pool
 *
 * Since: 0.7.0
 */
Pool *
hif_package_get_pool(HifPackage *pkg)
{
    HifPackagePrivate *priv = GET_PRIVATE(pkg);
    return hif_sack_get_pool(priv->sack);
}

static guint64
lookup_num(HifPackage *pkg, unsigned type)
{
    Solvable *s = get_solvable(pkg);
    repo_internalize_trigger(s->repo);
    return solvable_lookup_num(s, type, 0);
}

static HyReldepList
reldeps_for(HifPackage *pkg, Id type)
{
    Pool *pool = hif_package_get_pool(pkg);
    Solvable *s = get_solvable(pkg);
    HyReldepList reldeplist;
    Queue q;
    Id marker = -1;
    Id solv_type = type;

    if (type == SOLVABLE_PREREQMARKER) {
        solv_type = SOLVABLE_REQUIRES;
        marker = 1;
    }
    queue_init(&q);
    solvable_lookup_deparray(s, solv_type, &q, marker);

    reldeplist = reldeplist_from_queue(pool, q);

    queue_free(&q);
    return reldeplist;
}

/**
 * hif_package_clone:
 * @pkg: a #HifPackage instance.
 *
 * Clones the package.
 *
 * Returns: a #HifPackage
 *
 * Since: 0.7.0
 */
HifPackage *
hif_package_clone(HifPackage *pkg)
{
    return hif_package_new(hif_package_get_sack(pkg), hif_package_get_id(pkg));
}

/**
 * hif_package_get_id: (skip):
 * @pkg: a #HifPackage instance.
 *
 * Gets the internal ID used to identify the package in the pool.
 *
 * Returns: an integer, or 0 for unknown
 *
 * Since: 0.7.0
 */
Id
hif_package_get_id(HifPackage *pkg)
{
    HifPackagePrivate *priv = GET_PRIVATE(pkg);
    return priv->id;
}

/**
 * hif_package_get_sack:
 * @pkg: a #HifPackage instance.
 *
 * Gets the package sack for the package.
 *
 * Returns: a #HifSack, or %NULL
 *
 * Since: 0.7.0
 */
HifSack *
hif_package_get_sack(HifPackage *pkg)
{
    HifPackagePrivate *priv = GET_PRIVATE(pkg);
    return priv->sack;
}

/**
 * hif_package_from_solvable: (skip):
 * @pkg: a #HifPackage instance.
 *
 * Creates a package from a solvable.
 *
 * Returns: a #HyPackage
 *
 * Since: 0.7.0
 */
HifPackage *
hif_package_from_solvable(HifSack *sack, Solvable *s)
{
    if (!s)
        return NULL;

    Id p = s - s->repo->pool->solvables;
    return hif_package_new(sack, p);
}

/**
 * hif_package_get_identical:
 * @pkg1: a #HifPackage instance.
 * @pkg2: another #HifPackage instance.
 *
 * Tests two packages for equality.
 *
 * Returns: %TRUE if the packages are the same
 *
 * Since: 0.7.0
 */
gboolean
hif_package_get_identical(HifPackage *pkg1, HifPackage *pkg2)
{
    HifPackagePrivate *priv1 = GET_PRIVATE(pkg1);
    HifPackagePrivate *priv2 = GET_PRIVATE(pkg2);
    return priv1->id == priv2->id;
}

/**
 * hif_package_installed:
 * @pkg: a #HifPackage instance.
 *
 * Tests if the package is installed.
 *
 * Returns: %TRUE if installed
 *
 * Since: 0.7.0
 */
gboolean
hif_package_installed(HifPackage *pkg)
{
    Solvable *s = get_solvable(pkg);
    return (hif_package_get_pool(pkg)->installed == s->repo);
}

/**
 * hif_package_cmp:
 * @pkg1: a #HifPackage instance.
 * @pkg2: another #HifPackage instance.
 *
 * Compares two packages.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
int
hif_package_cmp(HifPackage *pkg1, HifPackage *pkg2)
{
    Pool *pool1 = hif_package_get_pool(pkg1);
    Pool *pool2 = hif_package_get_pool(pkg2);
    Solvable *s1 = pool_id2solvable(pool1, hif_package_get_id(pkg1));
    Solvable *s2 = pool_id2solvable(pool2, hif_package_get_id(pkg2));
    const char *str1 = pool_id2str(pool1, s1->name);
    const char *str2 = pool_id2str(pool2, s2->name);
    int ret = strcmp(str1, str2);
    if (ret)
        return ret;

    ret = hif_package_evr_cmp(pkg1, pkg2);
    if (ret)
        return ret;

    str1 = pool_id2str(pool1, s1->arch);
    str2 = pool_id2str(pool2, s2->arch);
    return strcmp(str1, str2);
}

/**
 * hif_package_evr_cmp:
 * @pkg1: a #HifPackage instance.
 * @pkg2: another #HifPackage instance.
 *
 * Compares two packages, only using the EVR values.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
int
hif_package_evr_cmp(HifPackage *pkg1, HifPackage *pkg2)
{
    Pool *pool1 = hif_package_get_pool(pkg1);
    Pool *pool2 = hif_package_get_pool(pkg2);
    Solvable *s1 = get_solvable(pkg1);
    Solvable *s2 = get_solvable(pkg2);
    const char *str1 = pool_id2str(pool1, s1->evr);
    const char *str2 = pool_id2str(pool2, s2->evr);

    return pool_evrcmp_str(hif_package_get_pool(pkg1), str1, str2, EVRCMP_COMPARE);
}

/**
 * hif_package_get_location:
 * @pkg: a #HifPackage instance.
 *
 * Gets the location (XXX??) for the package.
 *
 * Returns: (transfer full): string
 *
 * Since: 0.7.0
 */
char *
hif_package_get_location(HifPackage *pkg)
{
    Solvable *s = get_solvable(pkg);
    repo_internalize_trigger(s->repo);
    return g_strdup(solvable_get_location(s, NULL));
}

/**
 * hif_package_get_baseurl:
 * @pkg: a #HifPackage instance.
 *
 * Gets the baseurl for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
hif_package_get_baseurl(HifPackage *pkg)
{
    Solvable *s = get_solvable(pkg);
    return solvable_lookup_str(s, SOLVABLE_MEDIABASE);
}

/**
 * hif_package_get_nevra:
 * @pkg: a #HifPackage instance.
 *
 * Gets the package NEVRA.
 *
 * Returns: (transfer full): a string, or %NULL
 *
 * Since: 0.7.0
 */
char *
hif_package_get_nevra(HifPackage *pkg)
{
    Solvable *s = get_solvable(pkg);
    return g_strdup(pool_solvable2str(hif_package_get_pool(pkg), s));
}

/**
 * hif_package_get_sourcerpm:
 * @pkg: a #HifPackage instance.
 *
 * Gets the source RPM for the package.
 *
 * Returns: (transfer full): a string, or %NULL
 *
 * Since: 0.7.0
 */
char *
hif_package_get_sourcerpm(HifPackage *pkg)
{
    Solvable *s = get_solvable(pkg);
    return g_strdup(solvable_lookup_sourcepkg(s));
}

/**
 * hif_package_get_version:
 * @pkg: a #HifPackage instance.
 *
 * Gets the version for the package.
 *
 * Returns: (transfer full): a string, or %NULL
 *
 * Since: 0.7.0
 */
char *
hif_package_get_version(HifPackage *pkg)
{
    char *e, *v, *r;
    pool_split_evr(hif_package_get_pool(pkg), hif_package_get_evr(pkg), &e, &v, &r);
    return g_strdup(v);
}

/**
 * hif_package_get_release:
 * @pkg: a #HifPackage instance.
 *
 * Gets the release for the package.
 *
 * Returns: (transfer full): a string, or %NULL
 *
 * Since: 0.7.0
 */
char *
hif_package_get_release(HifPackage *pkg)
{
    char *e, *v, *r;
    pool_split_evr(hif_package_get_pool(pkg), hif_package_get_evr(pkg), &e, &v, &r);
    return g_strdup(r);
}

/**
 * hif_package_get_name:
 * @pkg: a #HifPackage instance.
 *
 * Gets the name for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
hif_package_get_name(HifPackage *pkg)
{
    Pool *pool = hif_package_get_pool(pkg);
    return pool_id2str(pool, get_solvable(pkg)->name);
}

/**
 * hif_package_get_packager:
 * @pkg: a #HifPackage instance.
 *
 * Gets the XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
hif_package_get_packager(HifPackage *pkg)
{
    return solvable_lookup_str(get_solvable(pkg), SOLVABLE_PACKAGER);
}

/**
 * hif_package_get_arch:
 * @pkg: a #HifPackage instance.
 *
 * Gets the architecture for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
hif_package_get_arch(HifPackage *pkg)
{
    Pool *pool = hif_package_get_pool(pkg);
    return pool_id2str(pool, get_solvable(pkg)->arch);
}

/**
 * hif_package_get_chksum:
 * @pkg: a #HifPackage instance.
 *
 * Gets the checksum for the package.
 *
 * Returns: raw checksum bytes
 *
 * Since: 0.7.0
 */
const unsigned char *
hif_package_get_chksum(HifPackage *pkg, int *type)
{
    Solvable *s = get_solvable(pkg);
    const unsigned char* ret;

    ret = solvable_lookup_bin_checksum(s, SOLVABLE_CHECKSUM, type);
    if (ret)
        *type = checksumt_l2h(*type);
    return ret;
}

/**
 * hif_package_get_hdr_chksum:
 * @pkg: a #HifPackage instance.
 *
 * Gets the SHA1 checksum of the packages header.
 * This is only set for packages in the rpmdb.
 *
 * Returns: raw checksum bytes
 *
 * Since: 0.7.0
 */
const unsigned char *
hif_package_get_hdr_chksum(HifPackage *pkg, int *type)
{
    Solvable *s = get_solvable(pkg);
    const unsigned char *ret;

    ret = solvable_lookup_bin_checksum(s, SOLVABLE_HDRID, type);
    if (ret)
        *type = checksumt_l2h(*type);
    return ret;
}

/**
 * hif_package_get_description:
 * @pkg: a #HifPackage instance.
 *
 * Gets the description for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
hif_package_get_description(HifPackage *pkg)
{
    return solvable_lookup_str(get_solvable(pkg), SOLVABLE_DESCRIPTION);
}

/**
 * hif_package_get_evr:
 * @pkg: a #HifPackage instance.
 *
 * Gets the EVR for the package.
 *
 * Returns: a string
 *
 * Since: 0.7.0
 */
const char *
hif_package_get_evr(HifPackage *pkg)
{
    Pool *pool = hif_package_get_pool(pkg);
    return pool_id2str(pool, get_solvable(pkg)->evr);
}

/**
 * hif_package_get_group:
 * @pkg: a #HifPackage instance.
 *
 * Gets the group for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
hif_package_get_group(HifPackage *pkg)
{
  return solvable_lookup_str(get_solvable(pkg), SOLVABLE_GROUP);
}

/**
 * hif_package_get_license:
 * @pkg: a #HifPackage instance.
 *
 * Gets the license for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
hif_package_get_license(HifPackage *pkg)
{
    return solvable_lookup_str(get_solvable(pkg), SOLVABLE_LICENSE);
}

/**
 * hif_package_get_reponame:
 * @pkg: a #HifPackage instance.
 *
 * Gets the reponame for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
hif_package_get_reponame(HifPackage *pkg)
{
    Solvable *s = get_solvable(pkg);
    return s->repo->name;
}

/**
 * hif_package_get_summary:
 * @pkg: a #HifPackage instance.
 *
 * Gets the summary for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
hif_package_get_summary(HifPackage *pkg)
{
    Solvable *s = get_solvable(pkg);
    return solvable_lookup_str(s, SOLVABLE_SUMMARY);
}

/**
 * hif_package_get_url:
 * @pkg: a #HifPackage instance.
 *
 * Gets the url for the package.
 *
 * Returns: a string, or %NULL
 *
 * Since: 0.7.0
 */
const char *
hif_package_get_url(HifPackage *pkg)
{
    return solvable_lookup_str(get_solvable(pkg), SOLVABLE_URL);
}

/**
 * hif_package_get_downloadsize:
 * @pkg: a #HifPackage instance.
 *
 * Gets the download size for the package.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
guint64
hif_package_get_downloadsize(HifPackage *pkg)
{
    return lookup_num(pkg, SOLVABLE_DOWNLOADSIZE);
}

/**
 * hif_package_get_epoch:
 * @pkg: a #HifPackage instance.
 *
 * Gets the epoch for the package.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
guint64
hif_package_get_epoch(HifPackage *pkg)
{
    return pool_get_epoch(hif_package_get_pool(pkg), hif_package_get_evr(pkg));
}

/**
 * hif_package_get_hdr_end:
 * @pkg: a #HifPackage instance.
 *
 * Gets the header end index for the package.
 *
 * Returns: an index, or 0 for not known
 *
 * Since: 0.7.0
 */
guint64
hif_package_get_hdr_end(HifPackage *pkg)
{
    return lookup_num(pkg, SOLVABLE_HEADEREND);
}

/**
 * hif_package_get_installsize:
 * @pkg: a #HifPackage instance.
 *
 * Gets the installed size for the package.
 *
 * Returns: size in bytes
 *
 * Since: 0.7.0
 */
guint64
hif_package_get_installsize(HifPackage *pkg)
{
    return lookup_num(pkg, SOLVABLE_INSTALLSIZE);
}

/**
 * hif_package_get_buildtime:
 * @pkg: a #HifPackage instance.
 *
 * Gets the build time for the package.
 *
 * Returns: UNIX time
 *
 * Since: 0.7.0
 */
guint64
hif_package_get_buildtime(HifPackage *pkg)
{
    return lookup_num(pkg, SOLVABLE_BUILDTIME);
}

/**
 * hif_package_get_installtime:
 * @pkg: a #HifPackage instance.
 *
 * Gets the install time for the package.
 *
 * Returns: UNIX time
 *
 * Since: 0.7.0
 */
guint64
hif_package_get_installtime(HifPackage *pkg)
{
    return lookup_num(pkg, SOLVABLE_INSTALLTIME);
}

/**
 * hif_package_get_medianr:
 * @pkg: a #HifPackage instance.
 *
 * Gets the media number for the package.
 *
 * Returns: integer value
 *
 * Since: 0.7.0
 */
guint64
hif_package_get_medianr(HifPackage *pkg)
{
    return lookup_num(pkg, SOLVABLE_MEDIANR);
}

/**
 * hif_package_get_rpmdbid:
 * @pkg: a #HifPackage instance.
 *
 * Gets the RPMDB ID for the package.
 *
 * Returns: an ID, or 0 for not known
 *
 * Since: 0.7.0
 */
guint64
hif_package_get_rpmdbid(HifPackage *pkg)
{
    guint64 ret = lookup_num(pkg, RPM_RPMDBID);
    g_assert(ret > 0);
    return ret;
}

/**
 * hif_package_get_size:
 * @pkg: a #HifPackage instance.
 *
 * Gets the size for the package.
 *
 * Returns: size in bytes
 *
 * Since: 0.7.0
 */
guint64
hif_package_get_size(HifPackage *pkg)
{
    unsigned type = hif_package_installed(pkg) ? SOLVABLE_INSTALLSIZE :
                                                 SOLVABLE_DOWNLOADSIZE;
    return lookup_num(pkg, type);
}

/**
 * hif_package_get_conflicts:
 * @pkg: a #HifPackage instance.
 *
 * Gets the conflicts for the package.
 *
 * Returns: A #HyReldepList
 *
 * Since: 0.7.0
 */
HyReldepList
hif_package_get_conflicts(HifPackage *pkg)
{
    return reldeps_for(pkg, SOLVABLE_CONFLICTS);
}

/**
 * hif_package_get_enhances:
 * @pkg: a #HifPackage instance.
 *
 * Gets the enhances for the package.
 *
 * Returns: A #HyReldepList
 *
 * Since: 0.7.0
 */
HyReldepList
hif_package_get_enhances(HifPackage *pkg)
{
    return reldeps_for(pkg, SOLVABLE_ENHANCES);
}

/**
 * hif_package_get_obsoletes:
 * @pkg: a #HifPackage instance.
 *
 * Gets the obsoletes for the package.
 *
 * Returns: A #HyReldepList
 *
 * Since: 0.7.0
 */
HyReldepList
hif_package_get_obsoletes(HifPackage *pkg)
{
    return reldeps_for(pkg, SOLVABLE_OBSOLETES);
}

/**
 * hif_package_get_provides:
 * @pkg: a #HifPackage instance.
 *
 * Gets the provides for the package.
 *
 * Returns: A #HyReldepList
 *
 * Since: 0.7.0
 */
HyReldepList
hif_package_get_provides(HifPackage *pkg)
{
    return reldeps_for(pkg, SOLVABLE_PROVIDES);
}

/**
 * hif_package_get_recommends:
 * @pkg: a #HifPackage instance.
 *
 * Gets the recommends for the package.
 *
 * Returns: A #HyReldepList
 *
 * Since: 0.7.0
 */
HyReldepList
hif_package_get_recommends(HifPackage *pkg)
{
    return reldeps_for(pkg, SOLVABLE_RECOMMENDS);
}

/**
 * hif_package_get_requires:
 * @pkg: a #HifPackage instance.
 *
 * Gets the requires for the package.
 *
 * Returns: A #HyReldepList
 *
 * Since: 0.7.0
 */
HyReldepList
hif_package_get_requires(HifPackage *pkg)
{
    return reldeps_for(pkg, SOLVABLE_REQUIRES);
}

/**
 * hif_package_get_requires_pre:
 * @pkg: a #HifPackage instance.
 *
 * Gets the Requires(pre) for the package.
 *
 * Returns: A #HyReldepList
 *
 * Since: 0.7.0
 */
HyReldepList
hif_package_get_requires_pre(HifPackage *pkg)
{
    return reldeps_for(pkg, SOLVABLE_PREREQMARKER);
}

/**
 * hif_package_get_suggests:
 * @pkg: a #HifPackage instance.
 *
 * Gets the suggests for the package.
 *
 * Returns: A #HyReldepList
 *
 * Since: 0.7.0
 */
HyReldepList
hif_package_get_suggests(HifPackage *pkg)
{
    return reldeps_for(pkg, SOLVABLE_SUGGESTS);
}

/**
 * hif_package_get_supplements:
 * @pkg: a #HifPackage instance.
 *
 * Gets the supplements for the package.
 *
 * Returns: A #HyReldepList
 *
 * Since: 0.7.0
 */
HyReldepList
hif_package_get_supplements(HifPackage *pkg)
{
    return reldeps_for(pkg, SOLVABLE_SUPPLEMENTS);
}

/**
 * hif_package_get_files:
 * @pkg: a #HifPackage instance.
 *
 * Gets the files contained in the package.
 *
 * Returns: (transfer full): the file list
 *
 * Since: 0.7.0
 */
gchar **
hif_package_get_files(HifPackage *pkg)
{
    HifPackagePrivate *priv = GET_PRIVATE(pkg);
    Pool *pool = hif_package_get_pool(pkg);
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
 * hif_package_get_advisories:
 * @pkg: a #HifPackage instance.
 *
 * Gets the list of advisories for the package.
 *
 * Returns: (transfer container) (element-type HifAdvisory): a list
 *
 * Since: 0.7.0
 */
GPtrArray *
hif_package_get_advisories(HifPackage *pkg, int cmp_type)
{
    Dataiterator di;
    Id evr;
    int cmp;
    HifAdvisory *advisory;
    Pool *pool = hif_package_get_pool(pkg);
    GPtrArray *advisorylist = g_ptr_array_new_with_free_func((GDestroyNotify) g_object_unref);
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
            advisory = hif_advisory_new(pool, di.solvid);
            g_ptr_array_add(advisorylist, advisory);
            dataiterator_skip_solvable(&di);
        }
    }
    dataiterator_free(&di);
    return advisorylist;
}

/**
 * hif_package_get_delta_from_evr:
 * @pkg: a #HifPackage instance.
 * @from_evr: the EVR string of what's installed
 *
 * Gets the package delta for the package given an existing EVR.
 *
 * Returns: a #HifPackageDelta, or %NULL
 *
 * Since: 0.7.0
 */
HifPackageDelta *
hif_package_get_delta_from_evr(HifPackage *pkg, const char *from_evr)
{
    Pool *pool = hif_package_get_pool(pkg);
    Solvable *s = get_solvable(pkg);
    HifPackageDelta *delta = NULL;
    Dataiterator di;
    const char *name = hif_package_get_name(pkg);

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

        // we have the right delta info, set up HifPackageDelta *and break out:
        delta = hif_packagedelta_new(pool);

        break;
    }
    dataiterator_free(&di);

    return delta;
}
