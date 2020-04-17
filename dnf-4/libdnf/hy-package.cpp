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

#include "libdnf/utils/utils.hpp"

#include <algorithm>
#include <ctime>
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
 * dnf_package_get_changelogs:
 * @pkg: a #DnfPackage instance.
 *
 * Gets the list of changelogs for the package.
 *
 * Returns: (transfer container) (element-type Changelog): a list
 *
 * Since: 0.19.1
 */
std::vector<libdnf::Changelog>
dnf_package_get_changelogs(DnfPackage *pkg)
{
    DnfPackagePrivate *priv = GET_PRIVATE(pkg);
    Pool *pool = dnf_package_get_pool(pkg);
    Solvable *s = get_solvable(pkg);
    Dataiterator di;
    std::vector<libdnf::Changelog> changelogslist;

    dataiterator_init(&di, pool, s->repo, priv->id, SOLVABLE_CHANGELOG_AUTHOR, NULL, 0);
    dataiterator_prepend_keyname(&di, SOLVABLE_CHANGELOG);
    while (dataiterator_step(&di)) {
        dataiterator_setpos_parent(&di);
        std::string author(pool_lookup_str(pool, SOLVID_POS, SOLVABLE_CHANGELOG_AUTHOR));
        std::string text(pool_lookup_str(pool, SOLVID_POS, SOLVABLE_CHANGELOG_TEXT));
        changelogslist.emplace_back(
            static_cast<time_t>(pool_lookup_num(pool, SOLVID_POS, SOLVABLE_CHANGELOG_TIME, 0)),
            std::move(author), std::move(text));
    }
    dataiterator_free(&di);
    std::reverse(changelogslist.begin(), changelogslist.end());
    return changelogslist;
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
    GPtrArray *advisorylist = g_ptr_array_new_with_free_func((GDestroyNotify) dnf_advisory_free);
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
            if (libdnf::isAdvisoryApplicable(*advisory, sack)) {
                g_ptr_array_add(advisorylist, advisory);
            } else {
                dnf_advisory_free(advisory);
            }
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
