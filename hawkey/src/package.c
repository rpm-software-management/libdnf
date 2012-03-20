#include <assert.h>

//libsolv
#include "solv/evr.h"
#include "solv/pool.h"
#include "solv/repo.h"
#include "solv/util.h"

// hawkey
#include "iutil.h"
#include "package_internal.h"

static Solvable *
get_solvable(HyPackage pkg)
{
    return pool_id2solvable(pkg->pool, pkg->id);
}

HyPackage
package_create(Pool *pool, Id id)
{
    HyPackage pkg;

    pkg = solv_calloc(1, sizeof(*pkg));
    pkg->pool = pool;
    pkg->id = id;
    return pkg;
}

HyPackage
package_from_solvable(Solvable *s)
{
    HyPackage pkg;

    if (!s)
	return NULL;
    pkg = solv_calloc(1, sizeof(*pkg));
    pkg->pool = s->repo->pool;
    pkg->id = s - s->repo->pool->solvables;
    return pkg;
}

void
package_free(HyPackage pkg)
{
    solv_free(pkg);
}

int
package_cmp(HyPackage pkg1, HyPackage pkg2)
{
    Pool *pool = pkg1->pool;
    Solvable *s1 = pool_id2solvable(pool, pkg1->id);
    Solvable *s2 = pool_id2solvable(pool, pkg2->id);
    const char *name1 = pool_id2str(pool, s1->name);
    const char *name2 = pool_id2str(pool, s2->name);
    int ret = strcmp(name1, name2);

   if (ret)
	return ret;
    return package_evr_cmp(pkg1, pkg2);
}

int
package_evr_cmp(HyPackage pkg1, HyPackage pkg2)
{
    Solvable *s1 = get_solvable(pkg1);
    Solvable *s2 = get_solvable(pkg2);

    return pool_evrcmp(pkg1->pool, s1->evr, s2->evr, EVRCMP_COMPARE);
}

char *
package_get_location(HyPackage pkg)
{
    Solvable *s = get_solvable(pkg);
    repo_internalize_trigger(s->repo);
    return solv_strdup(solvable_get_location(s, NULL));
}

char *
package_get_nvra(HyPackage pkg)
{
    Solvable *s = get_solvable(pkg);
    return solv_strdup(pool_solvable2str(pkg->pool, s));
}

const char*
package_get_name(HyPackage pkg)
{
    Pool *pool = pkg->pool;
    return pool_id2str(pool, get_solvable(pkg)->name);
}

const char*
package_get_arch(HyPackage pkg)
{
    Pool *pool = pkg->pool;
    return pool_id2str(pool, get_solvable(pkg)->arch);
}

const char*
package_get_evr(HyPackage pkg)
{
    Pool *pool = pkg->pool;
    return pool_id2str(pool, get_solvable(pkg)->evr);
}

const char*
package_get_reponame(HyPackage pkg)
{
    Solvable *s = get_solvable(pkg);
    return s->repo->name;
}

int
package_get_medianr(HyPackage pkg)
{
    Solvable *s = get_solvable(pkg);
    repo_internalize_trigger(s->repo);
    return solvable_lookup_num(s, SOLVABLE_MEDIANR, 0);
}

int
package_get_rpmdbid(HyPackage pkg)
{
    Solvable *s = get_solvable(pkg);
    int idx = solvable_lookup_num(s, RPM_RPMDBID, 0);
    assert(idx > 0);
    return idx;
}

int
package_get_size(HyPackage pkg)
{
    Solvable *s = get_solvable(pkg);
    repo_internalize_trigger(s->repo);
    return solvable_lookup_num(s, SOLVABLE_DOWNLOADSIZE, 0) * 1024;
}
