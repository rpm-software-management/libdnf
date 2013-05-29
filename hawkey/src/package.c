#include <assert.h>
#include <stdlib.h>

//libsolv
#include <solv/evr.h>
#include <solv/pool.h>
#include <solv/repo.h>
#include <solv/util.h>

// hawkey
#include "iutil.h"
#include "package_internal.h"
#include "reldep_internal.h"

/* internal */
static Solvable *
get_solvable(HyPackage pkg)
{
    return pool_id2solvable(pkg->pool, pkg->id);
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
    return package_create(package_pool(pkg), package_id(pkg));
}

HyPackage
package_create(Pool *pool, Id id)
{
    HyPackage pkg;

    pkg = solv_calloc(1, sizeof(*pkg));
    pkg->nrefs = 1;
    pkg->pool = pool;
    pkg->id = id;
    return pkg;
}

HyPackage
package_from_solvable(Solvable *s)
{
    if (!s)
	return NULL;

    Id p = s - s->repo->pool->solvables;
    return package_create(s->repo->pool, p);
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
    solv_free(pkg);
}

HyPackage
hy_package_link(HyPackage pkg)
{
    pkg->nrefs++;
    return pkg;
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
    return (pkg->pool->installed == s->repo);
}

int
hy_package_cmp(HyPackage pkg1, HyPackage pkg2)
{
    Pool *pool = pkg1->pool;
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

    return pool_evrcmp(pkg1->pool, s1->evr, s2->evr, EVRCMP_COMPARE);
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
    return solv_strdup(pool_solvable2str(pkg->pool, s));
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
    Pool *pool = pkg->pool;
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
    Pool *pool = pkg->pool;
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
    Pool *pool = pkg->pool;
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

HyPackageDelta
hy_package_get_delta_from_evr(HyPackage pkg, const char *from_evr)
{
    Pool *pool = pkg->pool;
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
