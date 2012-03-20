#include "assert.h"

// libsolv
#include "solv/pool.h"
#include "solv/repo.h"
#include "solv/queue.h"
#include "solv/util.h"

// hawkey
#include "packagelist.h"
#include "package_internal.h"
#include "sack_internal.h"

struct _HyPackageList {
    HyPackage *elements;
    int count;
    int left;
};

struct _HyPackageListIter {
    HyPackageList plist;
    int i;
    HyPackage current_pkg;
};

#define BLOCK_SIZE 31

HyPackageList
packagelist_create(void)
{
    HyPackageList plist = solv_calloc(1, sizeof(*plist));
    return plist;
}

/**
 * List of installed obsoletes of the given package.
 */
HyPackageList
packagelist_of_obsoletes(HySack sack, HyPackage pkg)
{
    HyPackageList plist = packagelist_create();
    Pool *pool = sack_pool(sack);
    Id *pp, r, rr;
    Solvable *s, *so;
    int obsprovides = pool_get_flag(pool, POOL_FLAG_OBSOLETEUSESPROVIDES);

    s = pool_id2solvable(pool, package_id(pkg));
    sack_make_provides_ready(sack);
    for (pp = s->repo->idarraydata + s->obsoletes; *pp; ++pp) {
	FOR_PROVIDES(r, rr, *pp) {
	    assert(r != SYSTEMSOLVABLE);
	    so = pool_id2solvable(pool, r);
	    if (!obsprovides && !pool_match_nevr(pool, so, *pp))
		continue; /* only matching pkg names */

	    if (so->repo != pool->installed)
		continue;
	    packagelist_push(plist, package_create(pool, r));
	}
    }
    return plist;
}

void
packagelist_free(HyPackageList plist)
{
    int i;

    for (i = 0; i < plist->count; ++i) {
	solv_free(plist->elements[i]);
    }
    solv_free(plist->elements);
    solv_free(plist);
}

int
packagelist_count(HyPackageList plist)
{
    return plist->count;
}

HyPackage
packagelist_get(HyPackageList plist, int index)
{
    assert(index < plist->count);
    return plist->elements[index];
}

void packagelist_push(HyPackageList plist, HyPackage pkg)
{
    plist->elements = solv_extend(plist->elements, plist->count, 1,
				  sizeof(pkg), BLOCK_SIZE);
    plist->elements[plist->count++] = pkg;
}

HyPackageListIter
packagelist_iter_create(HyPackageList plist)
{
    HyPackageListIter iter = solv_calloc(1, sizeof(*iter));
    iter->plist = plist;
    iter->i = -1;
    return iter;
}

void
packagelist_iter_free(HyPackageListIter iter)
{
    solv_free(iter);
}

HyPackage
packagelist_iter_next(HyPackageListIter iter)
{
    if (++iter->i >= iter->plist->count)
	return NULL;
    return packagelist_get(iter->plist, iter->i);
}
