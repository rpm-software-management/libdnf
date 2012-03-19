#include <assert.h>

// libsolv
#include "solv/bitmap.h"
#include "solv/evr.h"
#include "solv/repo.h"
#include "solv/solver.h"
#include "solv/util.h"

// hawkey
#include "iutil.h"
#include "query.h"

#define BLOCK_SIZE 15

static int
keyname2id(int keyname)
{
    switch(keyname) {
    case KN_PKG_NAME:
	return SOLVABLE_NAME;
    case KN_PKG_ARCH:
	return SOLVABLE_ARCH;
    case KN_PKG_SUMMARY:
	return SOLVABLE_SUMMARY;;
    default:
	assert(0);
    }
}

static int
type2flags(int type)
{
    switch (type) {
    case FT_EQ:
	return SEARCH_STRING;
    case FT_SUBSTR:
	return SEARCH_SUBSTRING;
    default:
	assert(0); // not implemented
    }
}

static int
type2relflags(int type)
{
    int flags = 0;
    if (type & FT_EQ)
	flags |= REL_EQ;
    if (type & FT_LT)
	flags |= REL_LT;
    if (type & FT_GT)
	flags |= REL_GT;
    assert(flags);
    return flags;
}

static void
filter_dataiterator(Query q, struct _Filter *f, Map *m)
{
    Pool *pool = q->sack->pool;
    Dataiterator di;
    int flags = type2flags(f->filter_type);
    Id keyname = keyname2id(f->keyname);

    dataiterator_init(&di, pool, 0, 0,
		      keyname,
		      f->match,
		      flags);
    while (dataiterator_step(&di))
	MAPSET(m, di.solvid);
    dataiterator_free(&di);
}

static void
filter_providers(Query q, struct _Filter *f, Map *m)
{
    Pool *pool = q->sack->pool;
    Id id_n, id_evr, r, p, pp;
    int flags;

    sack_make_provides_ready(q->sack);
    id_n = pool_str2id(pool, f->match, 0);
    id_evr = pool_str2id(pool, f->evr, 1);
    flags = type2relflags(f->filter_type);
    r = pool_rel2id(pool, id_n, id_evr, flags, 1);
    FOR_JOB_SELECT(p, pp, SOLVER_SOLVABLE_PROVIDES, r)
	MAPSET(m, p);
}

static void
filter_repo(Query q, struct _Filter *f, Map *m)
{
    Pool *pool = q->sack->pool;
    int i;
    Solvable *s;
    Repo *r;
    Id id, repoid = 0;

    FOR_REPOS(id, r) {
	if (!strcmp(r->name, f->match)) {
	    repoid = id;
	    break;
	}
    }
    if (!repoid)
	return; /* select nothing  */

    for (i = 1; i < pool->nsolvables; ++i) {
	s = pool_id2solvable(pool, i);
	switch (f->filter_type) {
	case FT_EQ:
	    if (s->repo && s->repo->repoid == repoid)
		MAPSET(m, i);
	    break;
	case FT_LT|FT_GT: /* i.e. not equal */
	    if (s->repo && s->repo->repoid != repoid)
		MAPSET(m, i);
	    break;
	default:
	    assert(0);
	}
    }
}

static void
filter_updates(Query q, Map *res)
{
    Sack sack = q->sack;
    Pool *pool = sack->pool;
    int i;
    Map m;
    Queue job;

    sack_make_provides_ready(q->sack);
    map_init(&m, pool->nsolvables);
    queue_init(&job);
    for (i = 1; i < pool->nsolvables; ++i) {
	if (!MAPTST(res, i))
	    continue;
	Solvable *s = pool_id2solvable(pool, i);
	if (s->repo != pool->installed) {
	    Id p = what_updates(pool, i);
	    if (p > 0)
		queue_push2(&job, SOLVER_SOLVABLE|SOLVER_UPDATE, p);
	} else
	    queue_push2(&job, SOLVER_SOLVABLE|SOLVER_UPDATE, i);
    }

    sack_solve(sack, &job, &m, SOLVER_TRANSACTION_UPGRADE);
    map_and(res, &m);
    map_free(&m);
    queue_free(&job);
}

static void
filter_latest(Query q, Map *res)
{
    Pool *pool = q->sack->pool;
    Queue samename;
    Id i, j, p, hp;
    Solvable *highest, *considered;
    Id name;
    int cmp;

    queue_init(&samename);
    for (i = 1; i < pool->nsolvables; ++i) {
	if (!MAPTST(res, i))
	    continue;

	queue_empty(&samename);
	name = pool_id2solvable(pool, i)->name;
	sack_same_names(q->sack, name, &samename);
	/* now find the highest versioned package of those selected */
	hp = i;
	highest = pool_id2solvable(pool, hp);
	for (j = 0; j < samename.count; ++j) {
	    p = samename.elements[j];
	    if (!MAPTST(res, p) || i == p)
		continue; /* out already */
	    considered = pool_id2solvable(pool, p);
	    cmp = pool_evrcmp(pool, highest->evr, considered->evr, EVRCMP_COMPARE);
	    if (cmp < 0) { /* new highest found */
		MAPCLR(res, hp);
		hp = p;
		highest = considered;
	    } else {
		/* note this is taken also for the same version case */
		MAPCLR(res, p);
	    }
	}
    }
    queue_free(&samename);
}

static void
filter_obsoleting(Query q, Map *res)
{
    Pool *pool = q->sack->pool;
    int obsprovides = pool_get_flag(pool, POOL_FLAG_OBSOLETEUSESPROVIDES);
    Id p, *pp;
    Solvable *s, *so;
    Map obsoleting;

    map_init(&obsoleting, pool->nsolvables);
    sack_make_provides_ready(q->sack);
    for (p = 1; p < pool->nsolvables; ++p) {
	if (!MAPTST(res, p))
	    continue;
	s = pool_id2solvable(pool, p);
	if (!s->repo)
	    continue;
	for (pp = s->repo->idarraydata + s->obsoletes; *pp; ++pp) {
	    Id r, rr;

	    FOR_PROVIDES(r, rr, *pp) {
		assert(r != SYSTEMSOLVABLE);
		so = pool_id2solvable(pool, r);
		if (!obsprovides && !pool_match_nevr(pool, so, *pp))
		    continue; /* only matching pkg names */
		if (so->repo != pool->installed)
		    continue;
		MAPSET(&obsoleting, p);
		break;
	    }
	}
    }
    map_and(res, &obsoleting);
    map_free(&obsoleting);
}

Query
query_create(Sack sack)
{
    Query q = solv_calloc(1, sizeof(*q));
    q->sack = sack;
    return q;
}

void
query_free(Query q)
{
    int i;
    for (i = 0; i < q->nfilters; ++i) {
	solv_free(q->filters[i].match);
	solv_free(q->filters[i].evr);
    }
    solv_free(q->filters);
    solv_free(q);
}

void
query_filter(Query q, int keyname, int filter_type, const char *match)
{
    struct _Filter filter = {
	.filter_type = filter_type,
	.keyname = keyname,
	.match = solv_strdup(match)
    };
    q->filters = solv_extend(q->filters, q->nfilters, 1, sizeof(filter),
			     BLOCK_SIZE);
    q->filters[q->nfilters++] = filter; /* structure assignment */
}

void
query_filter_provides(Query q, int filter_type, const char *name, const char *evr)
{
    struct _Filter filter = {
	.filter_type = filter_type,
	.keyname = KN_PKG_PROVIDES,
	.match = solv_strdup(name),
	.evr = solv_strdup(evr)
    };
    q->filters = solv_extend(q->filters, q->nfilters, 1, sizeof(filter),
			     BLOCK_SIZE);
    q->filters[q->nfilters++] = filter; /* structure assignment */
}

/**
 * Narrows to only packages updating installed packages.
 *
 * This requires resolving and so makes the final query expensive.
 */
void
query_filter_updates(Query q, int val)
{
    q->updates = val;
}

/**
 * Narrows to only the highest version of a package per arch.
 */
void
query_filter_latest(Query q, int val)
{
    q->latest = val;
}

/**
 * Narrows to only the packages obsoleting installed packages.
 *
 */
void
query_filter_obsoleting(Query q, int val)
{
    q->obsoleting = val;
}

PackageList
query_run(Query q)
{
    Pool *pool = q->sack->pool;
    PackageList plist;
    Map res, m;
    int i;

    assert(pool->installed);
    map_init(&m, pool->nsolvables);
    map_init(&res, pool->nsolvables);
    map_setall(&res);
    for (i = 0; i < q->nfilters; ++i) {
	struct _Filter *f = q->filters + i;

	map_empty(&m);
	if (f->keyname == KN_PKG_PROVIDES) {
	    filter_providers(q, f, &m);
	} else if (f->keyname == KN_PKG_REPO) {
	    filter_repo(q, f, &m);
	} else {
	    filter_dataiterator(q, f, &m);
	}
	map_and(&res, &m);
    }
    if (q->updates)
	filter_updates(q, &res);
    if (q->latest)
	filter_latest(q, &res);
    if (q->obsoleting)
	filter_obsoleting(q, &res);
    plist = packagelist_create();
    for (i = 1; i < pool->nsolvables; ++i)
	if (MAPTST(&res, i))
	    packagelist_push(plist, package_create(pool, i));
    map_free(&m);
    map_free(&res);

    return plist;
}

// internal/deprecated


/** Finds package of the name 'name'.
 *
 * FIXME: Currently only finds at most one result.
 */
static Solvable *
find_package_by_name(Sack sack, const char *name, Queue *job)
{
    Pool *pool = sack->pool;
    Id s_id = pool_str2id(pool, name, 0);
    Id p, pp;

    if (!s_id) {
	return NULL;
    }

    sack_make_provides_ready(sack);
    FOR_PROVIDES(p, pp, s_id) {
	Solvable *s = pool_id2solvable(pool, p);

	if (s->name == s_id) {
	    if (job)
		queue_push2(job, SOLVER_SOLVABLE_NAME, s_id);
	    return s;
	}
    }
    return NULL;
}

static void
find_package_by_summary(Pool *pool, const char *substr, Queue *qp)
{
    Dataiterator di;

    dataiterator_init(&di, pool, 0, 0, SOLVABLE_SUMMARY, substr, SEARCH_SUBSTRING);
    //    dataiterator_set_keyname(&di, SOLVABLE_SUMMARY);
    while (dataiterator_step(&di)) {
	queue_push(qp, di.solvid);
    }
    dataiterator_free(&di);
}

PackageList
sack_f_by_name(Sack sack, const char *name)
{
    PackageList plist = packagelist_create();
    Solvable *s;

    s = find_package_by_name(sack, name, NULL);
    if (s)
	packagelist_push(plist, package_from_solvable(s));
    return plist;
}

PackageList
sack_f_by_summary(Sack sack, const char *summary_substr)
{
    PackageList plist = packagelist_create();
    Queue q;

    queue_init(&q);
    find_package_by_summary(sack->pool, summary_substr, &q);
    queue2plist(sack, &q, plist);
    queue_free(&q);
    return plist;
}
