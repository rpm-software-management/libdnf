#include "assert.h"

// libsolv
#include "solv/pool.h"
#include "solv/repo.h"
#include "solv/queue.h"
#include "solv/util.h"

// hawkey
#include "packagelist.h"

#define BLOCK_SIZE 31

PackageList
packagelist_create(void)
{
    PackageList plist = solv_calloc(1, sizeof(*plist));
    return plist;
}

/**
 * List of installed obsoletes of the given package.
 */
PackageList
packagelist_of_obsoletes(Sack sack, Package pkg)
{
    PackageList plist = packagelist_create();
    Pool *pool = pkg->pool;
    Id *pp, r, rr;
    Solvable *s, *so;
    int obsprovides = pool_get_flag(pool, POOL_FLAG_OBSOLETEUSESPROVIDES);

    s = pool_id2solvable(pool, pkg->id);
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
packagelist_free(PackageList plist)
{
    int i;

    for (i = 0; i < plist->count; ++i) {
	solv_free(plist->elements[i]);
    }
    solv_free(plist->elements);
    solv_free(plist);
}

int
packagelist_count(PackageList plist)
{
    return plist->count;
}

Package
packagelist_get(PackageList plist, int index)
{
    assert(index < plist->count);
    return plist->elements[index];
}

void packagelist_push(PackageList plist, Package pkg)
{
    plist->elements = solv_extend(plist->elements, plist->count, 1,
				  sizeof(pkg), BLOCK_SIZE);
    plist->elements[plist->count++] = pkg;
}

Package
packagelist_element(PackageList plist, int i)
{
    return plist->elements[i];
}

PackageListIter
packagelist_iter_create(PackageList plist)
{
    PackageListIter iter = solv_calloc(1, sizeof(*iter));
    iter->plist = plist;
    iter->i = -1;
    return iter;
}

void
packagelist_iter_free(PackageListIter iter)
{
    solv_free(iter);
}

Package
packagelist_iter_next(PackageListIter iter)
{
    if (++iter->i >= iter->plist->count)
	return NULL;
    return packagelist_element(iter->plist, iter->i);
}
