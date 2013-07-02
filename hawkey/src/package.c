/*
 * Copyright (C) 2012-2013 Red Hat, Inc.
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
#include <solv/util.h>

// hawkey
#include "iutil.h"
#include "sack_internal.h"
#include "package_internal.h"
#include "reldep_internal.h"

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
    return sack_pool(pkg->sack);
}

static unsigned long long
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
    solvable_lookup_idarray(s, type, &q);
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
package_create(HySack sack, Id id)
{
    HyPackage pkg;

    pkg = solv_calloc(1, sizeof(*pkg));
    pkg->nrefs = 1;
    pkg->sack = sack;
    pkg->id = id;
    return pkg;
}

HyPackage
package_from_solvable(HySack sack, Solvable *s)
{
    if (!s)
	return NULL;

    Id p = s - s->repo->pool->solvables;
    return package_create(sack, p);
}

HyPackageDelta
delta_create(void) {
    return solv_calloc(1, sizeof(HyPackageDelta*));
}

/* public */
void
hy_package_free(HyPackage pkg)
{
    if (--pkg->nrefs > 0)
	return;
    if (pkg->destroy_func)
	pkg->destroy_func(pkg->userdata);
    solv_free(pkg);
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
    Pool *pool = package_pool(pkg1);
    Solvable *s1 = pool_id2solvable(pool, pkg1->id);
    Solvable *s2 = pool_id2solvable(pool, pkg2->id);
    const char *str1 = pool_id2str(pool, s1->name);
    const char *str2 = pool_id2str(pool, s2->name);
    int ret = strcmp(str1, str2);
    if (ret)
	return ret;

    ret = hy_package_evr_cmp(pkg1, pkg2);
    if (ret)
	return ret;

    str1 = pool_id2str(pool, s1->arch);
    str2 = pool_id2str(pool, s2->arch);
    return strcmp(str1, str2);
}

int
hy_package_evr_cmp(HyPackage pkg1, HyPackage pkg2)
{
    Solvable *s1 = get_solvable(pkg1);
    Solvable *s2 = get_solvable(pkg2);

    return pool_evrcmp(package_pool(pkg1), s1->evr, s2->evr, EVRCMP_COMPARE);
}

char *
hy_package_get_location(HyPackage pkg)
{
    Solvable *s = get_solvable(pkg);
    repo_internalize_trigger(s->repo);
    return solv_strdup(solvable_get_location(s, NULL));
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
    return solv_strdup(pool_solvable2str(package_pool(pkg), s));
}

char *
hy_package_get_sourcerpm(HyPackage pkg)
{
    Solvable *s = get_solvable(pkg);
    return solv_strdup(solvable_lookup_sourcepkg(s));
}

char *
hy_package_get_version(HyPackage pkg)
{
    char *e, *v, *r;

    pool_split_evr(package_pool(pkg), hy_package_get_evr(pkg), &e, &v, &r);
    return solv_strdup(v);
}

char *
hy_package_get_release(HyPackage pkg)
{
    char *e, *v, *r;

    pool_split_evr(package_pool(pkg), hy_package_get_evr(pkg), &e, &v, &r);
    return solv_strdup(r);
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

unsigned long long
hy_package_get_epoch(HyPackage pkg)
{
    return pool_get_epoch(package_pool(pkg), hy_package_get_evr(pkg));
}

unsigned long long
hy_package_get_buildtime(HyPackage pkg)
{
    return lookup_num(pkg, SOLVABLE_BUILDTIME);
}

unsigned long long
hy_package_get_installtime(HyPackage pkg)
{
    return lookup_num(pkg, SOLVABLE_INSTALLTIME);
}

unsigned long long
hy_package_get_medianr(HyPackage pkg)
{
    return lookup_num(pkg, SOLVABLE_MEDIANR);
}

unsigned long long
hy_package_get_rpmdbid(HyPackage pkg)
{
    unsigned long long ret = lookup_num(pkg, RPM_RPMDBID);
    assert(ret > 0);
    return ret;
}

unsigned long long
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
hy_package_get_requires(HyPackage pkg)
{
    return reldeps_for(pkg, SOLVABLE_REQUIRES);
}

HyStringArray
hy_package_get_files(HyPackage pkg)
{
    Pool *pool = package_pool(pkg);
    Solvable *s = get_solvable(pkg);
    Dataiterator di;
    int len = 0;
    HyStringArray strs = solv_extend(0, 0, 1, sizeof(char*), BLOCK_SIZE);

    dataiterator_init(&di, pool, s->repo, pkg->id, SOLVABLE_FILELIST, NULL,
		      SEARCH_FILES | SEARCH_COMPLETE_FILELIST);
    while (dataiterator_step(&di)) {
	strs[len++] = solv_strdup(di.kv.str);
	strs = solv_extend(strs, len, 1, sizeof(char*), BLOCK_SIZE);
    }
    dataiterator_free(&di);
    strs[len++] = NULL;
    return strs;
}

static Id
find_update(Pool *pool, HyPackage pkg)
{
    Dataiterator di;
    Id p, pp, evr;
    Id retval = 0;
    Solvable *is = NULL;
    Solvable *s = get_solvable(pkg);

    /* this is slow, don't do this ever time! */
    sack_make_provides_ready(pkg->sack);

    /* first find installed package for the range */
    FOR_PROVIDES(p, pp, s->name) {
        Solvable *iscand = pool->solvables + p;
        if (iscand->name != s->name || iscand->arch != s->arch)
            continue;
        if (!is || pool_evrcmp(pool, is->evr, iscand->evr, EVRCMP_COMPARE) < 0)
            is = iscand;
    }

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
        /* not newer than already installed */
        if (is && pool_evrcmp(pool, is->evr, evr, EVRCMP_COMPARE) >= 0)
            continue;
        if (pool_evrcmp(pool, s->evr, evr, EVRCMP_COMPARE) > 0)
            continue;
        retval = di.solvid;
        break;
    }
    dataiterator_free(&di);
    return retval;
}

const char *
hy_package_get_update_severity(HyPackage pkg)
{
    const char *tmp;
    Id p;
    Pool *pool = package_pool(pkg);

    p = find_update(pool, pkg);
    if (!p)
        return NULL;

    /* libsolv calls the <update status="stable"> a patch SOLVABLE_PATCHCATEGORY
     * and the <severity>foo</severity> an UPDATE_SEVERITY; we only use the
     * former in Fedora now, although we did support the latter at some stage */
    tmp = pool_lookup_str(pool, p, UPDATE_SEVERITY);
    if (!tmp)
        tmp = pool_lookup_str(pool, p, SOLVABLE_PATCHCATEGORY);
    return tmp;
}

const char *
hy_package_get_update_name(HyPackage pkg)
{
    const char *tmp;
    Id p;
    Pool *pool = package_pool(pkg);

    p = find_update(pool, pkg);
    if (!p)
        return NULL;

    /* libsolve 'helpfully' prepends "patch:" to the update name, remove it */
    tmp = pool_lookup_str(pool, p, SOLVABLE_NAME);
    if (!tmp)
        return NULL;
    if (strncmp(tmp, "patch:", 6) == 0)
        tmp += 6;

    return tmp;
}

const char *
hy_package_get_update_description(HyPackage pkg)
{
    Id p;
    Pool *pool = package_pool(pkg);

    p = find_update(pool, pkg);
    if (!p)
        return NULL;

    return pool_lookup_str(pool, p, SOLVABLE_DESCRIPTION);
}

unsigned long long
hy_package_get_update_issued(HyPackage pkg)
{
    Id p;
    Pool *pool = package_pool(pkg);

    p = find_update(pool, pkg);
    if (!p)
        return 0;

    /* libsolv does not distiguish between issued and updated */
    return pool_lookup_num(pool, p, SOLVABLE_BUILDTIME, 0);
}

HyStringArray
hy_package_get_update_urls(HyPackage pkg, const char *type)
{
    const char *type_tmp;
    const char *url_tmp;
    Dataiterator di;
    HyStringArray strs;
    Id p;
    int len = 0;
    Pool *pool = package_pool(pkg);

    p = find_update(pool, pkg);
    if (!p)
        return NULL;

    strs = solv_extend(0, 0, 1, sizeof(char*), BLOCK_SIZE);
    dataiterator_init(&di, pool, 0, p, UPDATE_REFERENCE, 0, 0);
    while (dataiterator_step(&di)) {
        dataiterator_setpos(&di);
        type_tmp = pool_lookup_str(pool, SOLVID_POS, UPDATE_REFERENCE_TYPE);
        url_tmp = pool_lookup_str(pool, SOLVID_POS, UPDATE_REFERENCE_HREF);
        if (!url_tmp || !type_tmp || strcmp (type_tmp, type) != 0)
                continue;
        strs[len++] = solv_strdup(di.kv.str);
        strs = solv_extend(strs, len, 1, sizeof(char*), BLOCK_SIZE);
    }
    strs[len++] = NULL;
    dataiterator_free(&di);
    return strs;
}

HyStringArray
hy_package_get_update_urls_bugzilla(HyPackage pkg)
{
    return hy_package_get_update_urls(pkg, "bugzilla");
}

HyStringArray
hy_package_get_update_urls_cve(HyPackage pkg)
{
    return hy_package_get_update_urls(pkg, "cve");
}

HyStringArray
hy_package_get_update_urls_vendor(HyPackage pkg)
{
    return hy_package_get_update_urls(pkg, "vendor");
}

HyPackageDelta
hy_package_get_delta_from_evr(HyPackage pkg, const char *from_evr)
{
    Pool *pool = package_pool(pkg);
    Solvable *s = get_solvable(pkg);
    HyPackageDelta delta = NULL;
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

	// we have the right delta info, set up HyPackageDelta and break out:
	delta = delta_create();
	const char *loc = pool_tmpjoin(pool,
				       pool_lookup_str(pool, SOLVID_POS, DELTA_LOCATION_DIR),
				       "/",
				       pool_lookup_str(pool, SOLVID_POS, DELTA_LOCATION_NAME));
	loc = pool_tmpappend(pool, loc, "-", pool_lookup_str(pool, SOLVID_POS, DELTA_LOCATION_EVR));
	loc = pool_tmpappend(pool, loc, ".", pool_lookup_str(pool, SOLVID_POS, DELTA_LOCATION_SUFFIX));
	delta->location = solv_strdup(loc);

	break;
    }
    dataiterator_free(&di);

    return delta;
}

const char *
hy_packagedelta_get_location(HyPackageDelta delta)
{
    return delta->location;
}

void
hy_packagedelta_free(HyPackageDelta delta)
{
    solv_free(delta->location);
    solv_free(delta);
}
