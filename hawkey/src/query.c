#include <assert.h>
#include <string.h>

// libsolv
#include <solv/evr.h>
#include <solv/repo.h>
#include <solv/solver.h>
#include <solv/util.h>

// hawkey
#include "errno.h"
#include "iutil.h"
#include "query_internal.h"
#include "package_internal.h"
#include "packagelist.h"
#include "packageset_internal.h"
#include "reldep_internal.h"
#include "sack_internal.h"

#define BLOCK_SIZE 15

static int
match_type_num(int keyname) {
    switch (keyname) {
    case HY_PKG_EPOCH:
	return 1;
    default:
	return 0;
    }
}

static int
match_type_pkg(int keyname) {
    switch (keyname) {
    case HY_PKG:
    case HY_PKG_OBSOLETES:
	return 1;
    default:
	return 0;
    }
}

static int
match_type_reldep(int keyname) {
    switch (keyname) {
    case HY_PKG_PROVIDES:
	return 1;
    default:
	return 0;
    }
}

static int
match_type_str(int keyname) {
    switch (keyname) {
    case HY_PKG_ARCH:
    case HY_PKG_DESCRIPTION:
    case HY_PKG_EVR:
    case HY_PKG_FILE:
    case HY_PKG_NAME:
    case HY_PKG_PROVIDES:
    case HY_PKG_RELEASE:
    case HY_PKG_REPONAME:
    case HY_PKG_REQUIRES:
    case HY_PKG_SOURCERPM:
    case HY_PKG_SUMMARY:
    case HY_PKG_URL:
    case HY_PKG_VERSION:
    case HY_PKG_LOCATION:
	return 1;
    default:
	return 0;
    }
}


static int
keyname2id(int keyname)
{
    switch(keyname) {
    case HY_PKG_DESCRIPTION:
	return SOLVABLE_DESCRIPTION;
    case HY_PKG_NAME:
	return SOLVABLE_NAME;
    case HY_PKG_URL:
	return SOLVABLE_URL;
    case HY_PKG_ARCH:
	return SOLVABLE_ARCH;
    case HY_PKG_EVR:
	return SOLVABLE_EVR;
    case HY_PKG_SUMMARY:
	return SOLVABLE_SUMMARY;
    case HY_PKG_FILE:
	return SOLVABLE_FILELIST;
    default:
	assert(0);
	return 0;
    }
}

static int
type2flags(int type, int keyname)
{
    int ret = 0;
    if (keyname == HY_PKG_FILE)
	ret |= SEARCH_FILES | SEARCH_COMPLETE_FILELIST;
    if (type & HY_ICASE)
	ret |= SEARCH_NOCASE;

    type &= ~HY_COMPARISON_FLAG_MASK;
    switch (type) {
    case HY_EQ:
	return ret | SEARCH_STRING;
    case HY_SUBSTR:
	return ret | SEARCH_SUBSTRING;
    case HY_GLOB:
	return ret | SEARCH_GLOB;
    default:
	assert(0); // not implemented
	return 0;
    }
}

static int
valid_filter_str(int keyname, int cmp_type)
{
    if (!match_type_str(keyname))
	return 0;

    cmp_type &= ~HY_NOT; // hy_query_run always handles NOT
    switch (keyname) {
    case HY_PKG_SOURCERPM:
    case HY_PKG_LOCATION:
	return cmp_type == HY_EQ;
    default:
	return 1;
    }
}

static int
valid_filter_num(int keyname, int cmp_type)
{
    if (!match_type_num(keyname))
	return 0;

    cmp_type &= ~HY_NOT; // hy_query_run always handles NOT
    if (cmp_type & (HY_ICASE | HY_SUBSTR | HY_GLOB))
	return 0;
    switch (keyname) {
    case HY_PKG:
	return cmp_type == HY_EQ;
    default:
	return 1;
    }
}

static int
valid_filter_pkg(int keyname, int cmp_type)
{
    if (!match_type_pkg(keyname))
	return 0;
    return cmp_type == HY_EQ;
}

static int
valid_filter_reldep(int keyname)
{
    if (!match_type_reldep(keyname))
	return 0;
    return 1;
}

struct _Filter *
filter_create(int nmatches)
{
    struct _Filter *f = solv_calloc(1, sizeof(struct _Filter));
    filter_reinit(f, nmatches);
    return f;
}

void
filter_reinit(struct _Filter *f, int nmatches)
{
    for (int m = 0; m < f->nmatches; ++m)
	switch (f->match_type) {
	case _HY_PKG:
	    hy_packageset_free(f->matches[m].pset);
	    break;
	case _HY_STR:
	    solv_free(f->matches[m].str);
	    break;
	case _HY_RELDEP:
	    hy_reldep_free(f->matches[m].reldep);
	    break;
	default:
	    break;
	}
    solv_free(f->matches);
    f->match_type = _HY_VOID;
    if (nmatches > 0)
	f->matches = solv_calloc(nmatches, sizeof(union _Match *));
    else
	f->matches = NULL;
    f->nmatches = nmatches;
    f->evr = solv_free(f->evr);
}

void
filter_free(struct _Filter *f)
{
    if (f) {
	filter_reinit(f, 0);
	solv_free(f);
    }
}

static struct _Filter *
query_add_filter(HyQuery q, int nmatches)
{
    struct _Filter filter;
    memset(&filter, 0, sizeof(filter));
    filter_reinit(&filter, nmatches);
    q->filters = solv_extend(q->filters, q->nfilters, 1, sizeof(filter),
			     BLOCK_SIZE);
    q->filters[q->nfilters] = filter; /* structure assignment */
    return q->filters + q->nfilters++;
}

static void
filter_dataiterator(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = sack_pool(q->sack);
    Dataiterator di;
    Id keyname = keyname2id(f->keyname);
    int flags = type2flags(f->cmp_type, f->keyname);

    assert(f->match_type == _HY_STR);
    /* do an OR over all matches: */
    for (int i = 0; i < f->nmatches; ++i) {
	dataiterator_init(&di, pool, 0, 0,
			  keyname,
			  f->matches[i].str,
			  flags);
	while (dataiterator_step(&di))
	    MAPSET(m, di.solvid);
	dataiterator_free(&di);
    }
}

static void
filter_pkg(HyQuery q, struct _Filter *f, Map *m)
{
    assert(f->nmatches == 1);
    assert(f->match_type == _HY_PKG);

    map_free(m);
    map_init_clone(m, packageset_get_map(f->matches[0].pset));
}

static void
filter_all(HyQuery q, struct _Filter *f, Map *m)
{
    assert(f->nmatches == 1);
    assert(f->match_type == _HY_NUM);
    assert(f->cmp_type == HY_EQ);
    assert(f->matches[0].num == -1);
    // just leaves m empty
}

static void
filter_epoch(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = sack_pool(q->sack);

    for (int mi = 0; mi < f->nmatches; ++mi) {
	unsigned long epoch = f->matches[mi].num;

	for (Id id = 1; id < pool->nsolvables; ++id) {
	    Solvable *s = pool_id2solvable(pool, id);
	    if (s->evr == ID_EMPTY)
		continue;

	    const char *evr = pool_id2str(pool, s->evr);
	    unsigned long pkg_epoch = pool_get_epoch(pool, evr);

	    int cmp_type = f->cmp_type;
	    if ((pkg_epoch > epoch && cmp_type & HY_GT) ||
		(pkg_epoch < epoch && cmp_type & HY_LT) ||
		(pkg_epoch == epoch && cmp_type & HY_EQ))
		MAPSET(m, id);
	}
    }
}

static void
filter_evr(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = sack_pool(q->sack);

    for (int mi = 0; mi < f->nmatches; ++mi) {
	Id match_evr = pool_str2id(pool, f->matches[mi].str, 1);

	for (Id id = 1; id < pool->nsolvables; ++id) {
	    Solvable *s = pool_id2solvable(pool, id);
	    int cmp = pool_evrcmp(pool, s->evr, match_evr, EVRCMP_COMPARE);

	    if ((cmp > 0 && f->cmp_type & HY_GT) ||
		(cmp < 0 && f->cmp_type & HY_LT) ||
		(cmp == 0 && f->cmp_type & HY_EQ))
		MAPSET(m, id);
	}
    }
}

static void
filter_version(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = sack_pool(q->sack);

    for (int mi = 0; mi < f->nmatches; ++mi) {
	char *filter_vr = solv_dupjoin(f->matches[mi].str, "-0", NULL);

	for (Id id = 1; id < pool->nsolvables; ++id) {
	    char *e, *v, *r;
	    Solvable *s = pool_id2solvable(pool, id);
	    if (s->evr == ID_EMPTY)
		continue;
	    const char *evr = pool_id2str(pool, s->evr);

	    pool_split_evr(pool, evr, &e, &v, &r);
	    char *vr = pool_tmpjoin(pool, v, "-0", NULL);
	    int cmp = pool_evrcmp_str(pool, vr, filter_vr, EVRCMP_COMPARE);

	    if ((cmp > 0 && f->cmp_type & HY_GT) ||
		(cmp < 0 && f->cmp_type & HY_LT) ||
		(cmp == 0 && f->cmp_type & HY_EQ))
		MAPSET(m, id);
	}
	solv_free(filter_vr);
    }
}

static void
filter_release(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = sack_pool(q->sack);

    for (int mi = 0; mi < f->nmatches; ++mi) {
	char *filter_vr = solv_dupjoin("0-", f->matches[mi].str, NULL);

	for (Id id = 1; id < pool->nsolvables; ++id) {
	    char *e, *v, *r;
	    Solvable *s = pool_id2solvable(pool, id);
	    if (s->evr == ID_EMPTY)
		continue;
	    const char *evr = pool_id2str(pool, s->evr);

	    pool_split_evr(pool, evr, &e, &v, &r);
	    char *vr = pool_tmpjoin(pool, "0-", r, NULL);

	    int cmp = pool_evrcmp_str(pool, vr, filter_vr, EVRCMP_COMPARE);

	    if ((cmp > 0 && f->cmp_type & HY_GT) ||
		(cmp < 0 && f->cmp_type & HY_LT) ||
		(cmp == 0 && f->cmp_type & HY_EQ))
		MAPSET(m, id);
	}
	solv_free(filter_vr);
    }
}

static void
filter_sourcerpm(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = sack_pool(q->sack);

    for (int mi = 0; mi < f->nmatches; ++mi) {
	const char *match = f->matches[mi].str;

	for (Id id = 1; id < pool->nsolvables; ++id) {
	    Solvable *s = pool_id2solvable(pool, id);

	    const char *name = solvable_lookup_str(s, SOLVABLE_SOURCENAME);
	    if (name == NULL)
		name = pool_id2str(pool, s->name);
	    if (!str_startswith(match, name)) // early check
		continue;

	    HyPackage pkg = package_create(pool, id);
	    char *srcrpm = hy_package_get_sourcerpm(pkg);
	    if (srcrpm && !strcmp(match, srcrpm))
		MAPSET(m, id);
	    solv_free(srcrpm);
	    hy_package_free(pkg);
	}
    }
}

static void
filter_obsoletes(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = sack_pool(q->sack);
    int obsprovides = pool_get_flag(pool, POOL_FLAG_OBSOLETEUSESPROVIDES);
    Map *target;

    assert(f->match_type == _HY_PKG);
    assert(f->nmatches == 1);
    target = packageset_get_map(f->matches[0].pset);
    sack_make_provides_ready(q->sack);
    for (Id p = 1; p < pool->nsolvables; ++p) {
	Solvable *s = pool_id2solvable(pool, p);
	if (!s->repo)
	    continue;
	for (Id *r_id = s->repo->idarraydata + s->obsoletes; *r_id; ++r_id) {
	    Id r, rr;

	    FOR_PROVIDES(r, rr, *r_id) {
		if (!MAPTST(target, r))
		    continue;
		assert(r != SYSTEMSOLVABLE);
		Solvable *so = pool_id2solvable(pool, r);
		if (!obsprovides && !pool_match_nevr(pool, so, *r_id))
		    continue; /* only matching pkg names */
		MAPSET(m, p);
		break;
	    }
	}
    }
}

static void
filter_provides_reldep(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = sack_pool(q->sack);
    Id p, pp;

    sack_make_provides_ready(q->sack);
    for (int i = 0; i < f->nmatches; ++i) {
	Id r_id = reldep_id(f->matches[i].reldep);
	FOR_PROVIDES(p, pp, r_id)
	    MAPSET(m, p);
    }
}

static void
filter_provides_str(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = sack_pool(q->sack);
    Id id, id_evr, r, p, pp;
    int flags;

    assert(f->nmatches == 1);
    sack_make_provides_ready(q->sack);

    id = pool_str2id(pool, f->matches[0].str, 0);
    if (id == STRID_NULL || id == STRID_EMPTY)
	return;
    id_evr = pool_str2id(pool, f->evr, 1);
    flags = cmptype2relflags(f->cmp_type);
    r = pool_rel2id(pool, id, id_evr, flags, 1);
    FOR_JOB_SELECT(p, pp, SOLVER_SOLVABLE_PROVIDES, r)
	MAPSET(m, p);
}

static void
filter_requires(HyQuery q, struct _Filter *f, Map *m)
{
    assert(f->nmatches == 1);
    Pool *pool = sack_pool(q->sack);
    Id id = pool_str2id(pool, f->matches[0].str, 0);

    if (id == STRID_NULL || id == STRID_EMPTY)
	return;

    if (f->evr) {
        Id evr = pool_str2id(pool, f->evr, 1);
        int flags = cmptype2relflags(f->cmp_type);
        id = pool_rel2id(pool, id, evr, flags, 1);
    }

    Queue requires;
    queue_init(&requires);
    for (Id s_id = 1; s_id < pool->nsolvables; ++s_id) {
	Solvable *s = pool_id2solvable(pool, s_id);

	queue_empty(&requires);
	solvable_lookup_idarray(s, SOLVABLE_REQUIRES, &requires);
	for (int x = 0; requires.count; x++) {
	    Id r_id = queue_pop(&requires);
	    if (pool_match_dep(pool, id, r_id)) {
		MAPSET(m, s_id);
		break;
	    }
	}
    }
    queue_free(&requires);
}

static void
filter_reponame(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = sack_pool(q->sack);
    int i;
    Solvable *s;
    Repo *r;
    Id id, ourids[pool->nrepos];

    for (id = 0; id < pool->nrepos; ++id)
	ourids[id] = 0;
    FOR_REPOS(id, r) {
	for (i = 0; i < f->nmatches; i++) {
	    if (!strcmp(r->name, f->matches[i].str)) {
		ourids[id] = 1;
		break;
	    }
	}
    }

    for (i = 1; i < pool->nsolvables; ++i) {
	s = pool_id2solvable(pool, i);
	switch (f->cmp_type & ~HY_COMPARISON_FLAG_MASK) {
	case HY_EQ:
	    if (s->repo && ourids[s->repo->repoid])
		MAPSET(m, i);
	    break;
	default:
	    assert(0);
	}
    }
}

static void
filter_location(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = sack_pool(q->sack);

    for (int mi = 0; mi < f->nmatches; ++mi) {
	const char *match = f->matches[mi].str;

	for (Id id = 1; id < pool->nsolvables; ++id) {
	    Solvable *s = pool_id2solvable(pool, id);

	    const char *location = solvable_get_location(s, NULL);
	    if (location == NULL)
		continue;
	    if (!strcmp(match, location))
		MAPSET(m, id);
	}
    }
}

static void
filter_updown(HyQuery q, int downgrade, Map *res)
{
    HySack sack = q->sack;
    Pool *pool = sack_pool(sack);
    int i;
    Map m;

    assert(pool->installed);
    sack_make_provides_ready(q->sack);
    map_init(&m, pool->nsolvables);
    for (i = 1; i < pool->nsolvables; ++i) {
	if (!MAPTST(res, i))
	    continue;
	Solvable *s = pool_id2solvable(pool, i);
	if (s->repo == pool->installed)
	    continue;
	if (downgrade && what_downgrades(pool, i) > 0)
	    MAPSET(&m, i);
	else if (!downgrade && what_upgrades(pool, i) > 0)
	    MAPSET(&m, i);
    }

    map_and(res, &m);
    map_free(&m);
}

static void
filter_latest(HyQuery q, Map *res)
{
    Pool *pool = sack_pool(q->sack);
    Queue samename;
    Id i, j, p, hp;
    Solvable *highest, *considered;
    int cmp;

    queue_init(&samename);
    for (i = 1; i < pool->nsolvables; ++i) {
	if (!MAPTST(res, i))
	    continue;

	hp = i;
	highest = pool_id2solvable(pool, hp);
	queue_empty(&samename);
	sack_same_names(q->sack, highest->name, highest->arch, &samename);
	/* now find the highest versioned package of those selected */
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
compute(HyQuery q)
{
    Pool *pool = sack_pool(q->sack);
    Map m;

    q->result = solv_calloc(1, sizeof(Map));
    map_init(q->result, pool->nsolvables);
    map_setall(q->result);
    MAPCLR(q->result, 0);
    MAPCLR(q->result, SYSTEMSOLVABLE);
    if (q->sack->excludes && !(q->flags & HY_IGNORE_EXCLUDES))
	map_subtract(q->result, q->sack->excludes);

    // make sure the odd bits are cleared:
    unsigned total_bits = q->result->size << 3;
    for (int i = pool->nsolvables; i < total_bits; ++i)
	MAPCLR(q->result, i);

    map_init(&m, pool->nsolvables);
    for (int i = 0; i < q->nfilters; ++i) {
	struct _Filter *f = q->filters + i;

	map_empty(&m);
	switch (f->keyname) {
	case HY_PKG:
	    filter_pkg(q, f, &m);
	    break;
	case HY_PKG_ALL:
	    filter_all(q, f, &m);
	    break;
	case HY_PKG_EPOCH:
	    filter_epoch(q, f, &m);
	    break;
	case HY_PKG_EVR:
	    filter_evr(q, f, &m);
	    break;
	case HY_PKG_VERSION:
	    filter_version(q, f, &m);
	    break;
	case HY_PKG_RELEASE:
	    filter_release(q, f, &m);
	    break;
	case HY_PKG_SOURCERPM:
	    filter_sourcerpm(q, f, &m);
	    break;
	case HY_PKG_OBSOLETES:
	    filter_obsoletes(q, f, &m);
	    break;
	case HY_PKG_PROVIDES:
	    if (f->match_type == _HY_RELDEP)
		filter_provides_reldep(q, f, &m);
	    else {
		assert(f->match_type == _HY_STR);
		filter_provides_str(q, f, &m);
	    }
	    break;
	case HY_PKG_REQUIRES:
	    filter_requires(q, f, &m);
	    break;
	case HY_PKG_REPONAME:
	    filter_reponame(q, f, &m);
	    break;
	case HY_PKG_LOCATION:
	    filter_location(q, f, &m);
	    break;
	default:
	    filter_dataiterator(q, f, &m);
	}
	if (f->cmp_type & HY_NOT)
	    map_subtract(q->result, &m);
	else
	    map_and(q->result, &m);
    }
    map_free(&m);
    if (q->downgrades)
	filter_updown(q, 1, q->result);
    if (q->updates)
	filter_updown(q, 0, q->result);
    if (q->latest)
	filter_latest(q, q->result);
}

static void
clear_result(HyQuery q)
{
    if (q->result) {
	map_free(q->result);
	q->result = solv_free(q->result);
    }
}


HyQuery
hy_query_create(HySack sack)
{
    return hy_query_create_flags(sack, 0);
}

HyQuery
hy_query_create_flags(HySack sack, int flags)
{
    HyQuery q = solv_calloc(1, sizeof(*q));
    q->sack = sack;
    q->flags = flags;
    return q;
}

void
hy_query_free(HyQuery q)
{
    hy_query_clear(q);
    solv_free(q);
}

void
hy_query_clear(HyQuery q)
{
    clear_result(q);
    for (int i = 0; i < q->nfilters; ++i) {
	struct _Filter *filterp = q->filters + i;
	filter_reinit(filterp, 0);
    }
    solv_free(q->filters);
    q->filters = NULL;
    q->nfilters = 0;
}

HyQuery
hy_query_clone(HyQuery q)
{
    HyQuery qn = hy_query_create(q->sack);

    qn->flags = q->flags;
    qn->downgrades = q->downgrades;
    qn->updates = q->updates;
    qn->latest = q->latest;

    for (int i = 0; i < q->nfilters; ++i) {
	struct _Filter *filterp = query_add_filter(qn, q->filters[i].nmatches);

	filterp->cmp_type = q->filters[i].cmp_type;
	filterp->keyname = q->filters[i].keyname;
	filterp->match_type = q->filters[i].match_type;
	for (int j = 0; j < q->filters[i].nmatches; ++j) {
	    char *str_copy;
	    HyPackageSet pset;
	    HyReldep reldep;

	    switch (filterp->match_type) {
	    case _HY_NUM:
		filterp->matches[j].num = q->filters[i].matches[j].num;
		break;
	    case _HY_PKG:
		pset = q->filters[i].matches[j].pset;
		filterp->matches[j].pset = hy_packageset_clone(pset);
		break;
	    case _HY_RELDEP:
		reldep = q->filters[i].matches[j].reldep;
		filterp->matches[j].reldep = hy_reldep_clone(reldep);
		break;
	    case _HY_STR:
		str_copy = solv_strdup(q->filters[i].matches[j].str);
		filterp->matches[j].str = str_copy;
		break;
	    default:
		assert(0);
	    }
	}
	filterp->evr = solv_strdup(q->filters[i].evr);
    }
    assert(qn->nfilters == q->nfilters);
    if (q->result) {
	qn->result = solv_calloc(1, sizeof(Map));
	map_init_clone(qn->result, q->result);
    }

    return qn;
}

int
hy_query_filter(HyQuery q, int keyname, int cmp_type, const char *match)
{
    if (!valid_filter_str(keyname, cmp_type))
	return HY_E_QUERY;
    clear_result(q);

    struct _Filter *filterp = query_add_filter(q, 1);
    filterp->cmp_type = cmp_type;
    filterp->keyname = keyname;
    filterp->match_type = _HY_STR;
    filterp->matches[0].str = solv_strdup(match);
    return 0;
}

int
hy_query_filter_empty(HyQuery q)
{
    struct _Filter *filterp = query_add_filter(q, 1);
    filterp->cmp_type = HY_EQ;
    filterp->keyname = HY_PKG_ALL;
    filterp->match_type = _HY_NUM;
    filterp->matches[0].num = -1;
    return 0;
}

int
hy_query_filter_in(HyQuery q, int keyname, int cmp_type,
		   const char **matches)
{
    if (!valid_filter_str(keyname, cmp_type))
	return HY_E_QUERY;
    clear_result(q);

    const unsigned count = count_nullt_array(matches);
    struct _Filter *filterp = query_add_filter(q, count);

    filterp->cmp_type = cmp_type;
    filterp->keyname = keyname;
    filterp->match_type = _HY_STR;
    for (int i = 0; i < count; ++i)
	filterp->matches[i].str = solv_strdup(matches[i]);
    return 0;
}

int
hy_query_filter_num(HyQuery q, int keyname, int cmp_type, int match)
{
    if (!valid_filter_num(keyname, cmp_type))
	return HY_E_QUERY;
    clear_result(q);

    struct _Filter *filterp = query_add_filter(q, 1);
    filterp->cmp_type = cmp_type;
    filterp->keyname = keyname;
    filterp->match_type = _HY_NUM;
    filterp->matches[0].num = match;
    return 0;
}

int
hy_query_filter_num_in(HyQuery q, int keyname, int cmp_type, int nmatches,
		       const int *matches)
{
    if (!valid_filter_num(keyname, cmp_type))
	return HY_E_QUERY;
    clear_result(q);

    struct _Filter *filterp = query_add_filter(q, nmatches);

    filterp->cmp_type = cmp_type;
    filterp->keyname = keyname;
    filterp->match_type = _HY_NUM;
    for (int i = 0; i < nmatches; ++i)
	filterp->matches[i].num = matches[i];
    return 0;
}

int
hy_query_filter_package_in(HyQuery q, int keyname, int cmp_type,
			   const HyPackageSet pset)
{
    if (!valid_filter_pkg(keyname, cmp_type))
	return HY_E_QUERY;
    clear_result(q);

    struct _Filter *filterp = query_add_filter(q, 1);
    filterp->cmp_type = cmp_type;
    filterp->keyname = keyname;
    filterp->match_type = _HY_PKG;
    filterp->matches[0].pset = hy_packageset_clone(pset);
    return 0;
}

int
hy_query_filter_reldep(HyQuery q, int keyname, const HyReldep reldep)
{
    if (!valid_filter_reldep(keyname))
	return HY_E_QUERY;
    clear_result(q);

    struct _Filter *filterp = query_add_filter(q, 1);
    filterp->cmp_type = HY_EQ;
    filterp->keyname = keyname;
    filterp->match_type = _HY_RELDEP;
    filterp->matches[0].reldep = hy_reldep_clone(reldep);
    return 0;
}

int
hy_query_filter_reldep_in(HyQuery q, int keyname, const HyReldepList reldeplist)
{
    if (!valid_filter_reldep(keyname))
	return HY_E_QUERY;
    clear_result(q);

    const int nmatches = hy_reldeplist_count(reldeplist);
    struct _Filter *filterp = query_add_filter(q, nmatches);
    filterp->cmp_type = HY_EQ;
    filterp->keyname = keyname;
    filterp->match_type = _HY_RELDEP;

    for (int i = 0; i < nmatches; ++i)
	filterp->matches[i].reldep = hy_reldeplist_get_clone(reldeplist, i);
    return 0;
}

int
hy_query_filter_provides(HyQuery q, int cmp_type, const char *name, const char *evr)
{
    struct _Filter *filterp = query_add_filter(q, 1);

    clear_result(q);
    filterp->cmp_type = cmp_type;
    filterp->keyname = HY_PKG_PROVIDES;
    filterp->match_type = _HY_STR;
    filterp->evr = solv_strdup(evr);
    filterp->matches[0].str = solv_strdup(name);
    return 0;
}

int
hy_query_filter_requires(HyQuery q, int cmp_type, const char *name, const char *evr)
{
    struct _Filter *filterq = query_add_filter(q, 1);

    clear_result(q);
    filterq->cmp_type = cmp_type;
    filterq->keyname = HY_PKG_REQUIRES;
    filterq->match_type = _HY_STR;
    filterq->evr = solv_strdup(evr);
    filterq->matches[0].str = solv_strdup(name);
    return 0;
}

/**
 * Narrows to only packages updating installed packages.
 *
 * This requires resolving and so makes the final query expensive.
 */
void
hy_query_filter_downgrades(HyQuery q, int val)
{
    clear_result(q);
    q->downgrades = val;
}

/**
 * Narrows to only packages updating installed packages.
 *
 * This requires resolving and so makes the final query expensive.
 */
void
hy_query_filter_upgrades(HyQuery q, int val)
{
    clear_result(q);
    q->updates = val;
}

/**
 * Narrows to only the highest version of a package per arch.
 */
void
hy_query_filter_latest(HyQuery q, int val)
{
    clear_result(q);
    q->latest = val;
}

HyPackageList
hy_query_run(HyQuery q)
{
    Pool *pool = sack_pool(q->sack);
    HyPackageList plist = hy_packagelist_create();

    if (!q->result)
	compute(q);
    for (int i = 1; i < pool->nsolvables; ++i)
	if (MAPTST(q->result, i))
	    hy_packagelist_push(plist, package_create(pool, i));
    return plist;
}

HyPackageSet
hy_query_run_set(HyQuery q)
{
    if (!q->result)
	compute(q);
    return packageset_from_bitmap(q->sack, q->result);
}
