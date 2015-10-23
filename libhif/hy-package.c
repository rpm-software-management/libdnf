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

#include <assert.h>
#include <stdlib.h>

//libsolv
#include <solv/evr.h>
#include <solv/pool.h>
#include <solv/repo.h>

// hawkey
#include "hif-advisory-private.h"
#include "hy-iutil.h"
#include "hif-sack-private.h"
#include "hif-packagedelta-private.h"
#include "hy-package-private.h"
#include "hy-reldep-private.h"
#include "hy-repo-private.h"

#define BLOCK_SIZE 31

/* internal */
static Solvable *
get_solvable(HyPackage pkg)
{
    return pool_id2solvable(package_pool(pkg), pkg->id);
}

Pool *
package_pool(HyPackage pkg)
{
    return hif_sack_get_pool(pkg->sack);
}

static guint64
lookup_num(HyPackage pkg, unsigned type)
{
    Solvable *s = get_solvable(pkg);
    repo_internalize_trigger(s->repo);
    return solvable_lookup_num(s, type, 0);
}

static HyReldepList
reldeps_for(HyPackage pkg, Id type)
{
    Pool *pool = package_pool(pkg);
    Solvable *s = get_solvable(pkg);
    HyReldepList reldeplist;
    Queue q;

    queue_init(&q);
    solvable_lookup_deparray(s, type, &q, -1);
    reldeplist = reldeplist_from_queue(pool, q);

    queue_free(&q);
    return reldeplist;
}

HyPackage
package_clone(HyPackage pkg)
{
    return package_create(package_sack(pkg), package_id(pkg));
}

HyPackage
package_create(HifSack *sack, Id id)
{
    HyPackage pkg;

    pkg = g_malloc0(sizeof(*pkg));
    pkg->nrefs = 1;
    pkg->sack = sack;
    pkg->id = id;
    return pkg;
}

HyPackage
package_from_solvable(HifSack *sack, Solvable *s)
{
    if (!s)
        return NULL;

    Id p = s - s->repo->pool->solvables;
    return package_create(sack, p);
}

/* public */
void
hy_package_free(HyPackage pkg)
{
    if (--pkg->nrefs > 0)
        return;
    if (pkg->destroy_func)
        pkg->destroy_func(pkg->userdata);
    g_free(pkg);
}

HyPackage
hy_package_link(HyPackage pkg)
{
    pkg->nrefs++;
    return pkg;
}

void *
hy_package_get_userdata(HyPackage pkg)
{
    return pkg->userdata;
}

void
hy_package_set_userdata(HyPackage pkg, void *userdata, HyUserdataDestroy destroy_func)
{
    if (pkg->destroy_func)
        pkg->destroy_func(pkg->userdata);
    pkg->userdata = userdata;
    pkg->destroy_func = destroy_func;
}

int
hy_package_identical(HyPackage pkg1, HyPackage pkg2)
{
    return pkg1->id == pkg2->id;
}

int
hy_package_installed(HyPackage pkg)
{
    Solvable *s = get_solvable(pkg);
    return (package_pool(pkg)->installed == s->repo);
}

int
hy_package_cmp(HyPackage pkg1, HyPackage pkg2)
{
    Pool *pool1 = package_pool(pkg1);
    Pool *pool2 = package_pool(pkg2);
    Solvable *s1 = pool_id2solvable(pool1, pkg1->id);
    Solvable *s2 = pool_id2solvable(pool2, pkg2->id);
    const char *str1 = pool_id2str(pool1, s1->name);
    const char *str2 = pool_id2str(pool2, s2->name);
    int ret = strcmp(str1, str2);
    if (ret)
        return ret;

    ret = hy_package_evr_cmp(pkg1, pkg2);
    if (ret)
        return ret;

    str1 = pool_id2str(pool1, s1->arch);
    str2 = pool_id2str(pool2, s2->arch);
    return strcmp(str1, str2);
}

int
hy_package_evr_cmp(HyPackage pkg1, HyPackage pkg2)
{
    Pool *pool1 = package_pool(pkg1);
    Pool *pool2 = package_pool(pkg2);
    Solvable *s1 = get_solvable(pkg1);
    Solvable *s2 = get_solvable(pkg2);
    const char *str1 = pool_id2str(pool1, s1->evr);
    const char *str2 = pool_id2str(pool2, s2->evr);

    return pool_evrcmp_str(package_pool(pkg1), str1, str2, EVRCMP_COMPARE);
}

char *
hy_package_get_location(HyPackage pkg)
{
    Solvable *s = get_solvable(pkg);
    repo_internalize_trigger(s->repo);
    return g_strdup(solvable_get_location(s, NULL));
}

const char *
hy_package_get_baseurl(HyPackage pkg)
{
    Solvable *s = get_solvable(pkg);
    return solvable_lookup_str(s, SOLVABLE_MEDIABASE);
}

char *
hy_package_get_nevra(HyPackage pkg)
{
    Solvable *s = get_solvable(pkg);
    return g_strdup(pool_solvable2str(package_pool(pkg), s));
}

char *
hy_package_get_sourcerpm(HyPackage pkg)
{
    Solvable *s = get_solvable(pkg);
    return g_strdup(solvable_lookup_sourcepkg(s));
}

char *
hy_package_get_version(HyPackage pkg)
{
    char *e, *v, *r;

    pool_split_evr(package_pool(pkg), hy_package_get_evr(pkg), &e, &v, &r);
    return g_strdup(v);
}

char *
hy_package_get_release(HyPackage pkg)
{
    char *e, *v, *r;

    pool_split_evr(package_pool(pkg), hy_package_get_evr(pkg), &e, &v, &r);
    return g_strdup(r);
}

const char*
hy_package_get_name(HyPackage pkg)
{
    Pool *pool = package_pool(pkg);
    return pool_id2str(pool, get_solvable(pkg)->name);
}

const char *
hy_package_get_packager(HyPackage pkg)
{
    return solvable_lookup_str(get_solvable(pkg), SOLVABLE_PACKAGER);
}

const char*
hy_package_get_arch(HyPackage pkg)
{
    Pool *pool = package_pool(pkg);
    return pool_id2str(pool, get_solvable(pkg)->arch);
}

const unsigned char *
hy_package_get_chksum(HyPackage pkg, int *type)
{
    Solvable *s = get_solvable(pkg);
    const unsigned char* ret;

    ret = solvable_lookup_bin_checksum(s, SOLVABLE_CHECKSUM, type);
    if (ret)
        *type = checksumt_l2h(*type);
    return ret;
}

/**
 * SHA1 checksum of the package's header.
 *
 * Only sane for packages in rpmdb.
 */
const unsigned char *
hy_package_get_hdr_chksum(HyPackage pkg, int *type)
{
    Solvable *s = get_solvable(pkg);
    const unsigned char *ret;

    ret = solvable_lookup_bin_checksum(s, SOLVABLE_HDRID, type);
    if (ret)
        *type = checksumt_l2h(*type);
    return ret;
}

const char *
hy_package_get_description(HyPackage pkg)
{
    return solvable_lookup_str(get_solvable(pkg), SOLVABLE_DESCRIPTION);
}

const char*
hy_package_get_evr(HyPackage pkg)
{
    Pool *pool = package_pool(pkg);
    return pool_id2str(pool, get_solvable(pkg)->evr);
}

const char *
hy_package_get_license(HyPackage pkg)
{
    return solvable_lookup_str(get_solvable(pkg), SOLVABLE_LICENSE);
}

const char*
hy_package_get_reponame(HyPackage pkg)
{
    Solvable *s = get_solvable(pkg);
    return s->repo->name;
}

const char*
hy_package_get_summary(HyPackage pkg)
{
    Solvable *s = get_solvable(pkg);
    return solvable_lookup_str(s, SOLVABLE_SUMMARY);
}

const char *
hy_package_get_url(HyPackage pkg)
{
    return solvable_lookup_str(get_solvable(pkg), SOLVABLE_URL);
}

guint64
hy_package_get_downloadsize(HyPackage pkg)
{
    return lookup_num(pkg, SOLVABLE_DOWNLOADSIZE);
}

guint64
hy_package_get_epoch(HyPackage pkg)
{
    return pool_get_epoch(package_pool(pkg), hy_package_get_evr(pkg));
}

guint64
hy_package_get_hdr_end(HyPackage pkg)
{
    return lookup_num(pkg, SOLVABLE_HEADEREND);
}

guint64
hy_package_get_installsize(HyPackage pkg)
{
    return lookup_num(pkg, SOLVABLE_INSTALLSIZE);
}

guint64
hy_package_get_buildtime(HyPackage pkg)
{
    return lookup_num(pkg, SOLVABLE_BUILDTIME);
}

guint64
hy_package_get_installtime(HyPackage pkg)
{
    return lookup_num(pkg, SOLVABLE_INSTALLTIME);
}

guint64
hy_package_get_medianr(HyPackage pkg)
{
    return lookup_num(pkg, SOLVABLE_MEDIANR);
}

guint64
hy_package_get_rpmdbid(HyPackage pkg)
{
    guint64 ret = lookup_num(pkg, RPM_RPMDBID);
    assert(ret > 0);
    return ret;
}

guint64
hy_package_get_size(HyPackage pkg)
{
    unsigned type = hy_package_installed(pkg) ? SOLVABLE_INSTALLSIZE :
                                                SOLVABLE_DOWNLOADSIZE;
    return lookup_num(pkg, type);
}

HyReldepList
hy_package_get_conflicts(HyPackage pkg)
{
    return reldeps_for(pkg, SOLVABLE_CONFLICTS);
}

HyReldepList
hy_package_get_enhances(HyPackage pkg)
{
    return reldeps_for(pkg, SOLVABLE_ENHANCES);
}

HyReldepList
hy_package_get_obsoletes(HyPackage pkg)
{
    return reldeps_for(pkg, SOLVABLE_OBSOLETES);
}

HyReldepList
hy_package_get_provides(HyPackage pkg)
{
    return reldeps_for(pkg, SOLVABLE_PROVIDES);
}

HyReldepList
hy_package_get_recommends(HyPackage pkg)
{
    return reldeps_for(pkg, SOLVABLE_RECOMMENDS);
}

HyReldepList
hy_package_get_requires(HyPackage pkg)
{
    return reldeps_for(pkg, SOLVABLE_REQUIRES);
}

HyReldepList
hy_package_get_suggests(HyPackage pkg)
{
    return reldeps_for(pkg, SOLVABLE_SUGGESTS);
}

HyReldepList
hy_package_get_supplements(HyPackage pkg)
{
    return reldeps_for(pkg, SOLVABLE_SUPPLEMENTS);
}

gchar **
hy_package_get_files(HyPackage pkg)
{
    Pool *pool = package_pool(pkg);
    Solvable *s = get_solvable(pkg);
    Dataiterator di;
    GPtrArray *ret = g_ptr_array_new();

    repo_internalize_trigger(s->repo);
    dataiterator_init(&di, pool, s->repo, pkg->id, SOLVABLE_FILELIST, NULL,
                      SEARCH_FILES | SEARCH_COMPLETE_FILELIST);
    while (dataiterator_step(&di)) {
        g_ptr_array_add(ret, g_strdup(di.kv.str));
    }
    dataiterator_free(&di);
    g_ptr_array_add(ret, NULL);
    return (gchar**)g_ptr_array_free (ret, FALSE);
}

GPtrArray *
hy_package_get_advisories(HyPackage pkg, int cmp_type)
{
    Dataiterator di;
    Id evr;
    int cmp;
    HifAdvisory *advisory;
    Pool *pool = package_pool(pkg);
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

HifPackageDelta *
hy_package_get_delta_from_evr(HyPackage pkg, const char *from_evr)
{
    Pool *pool = package_pool(pkg);
    Solvable *s = get_solvable(pkg);
    HifPackageDelta *delta = NULL;
    Dataiterator di;
    const char *name = hy_package_get_name(pkg);

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
