#include <assert.h>
#include <stdio.h>

// libsolv
#include "solv/solver.h"
#include "solv/solverdebug.h"
#include "solv/util.h"

// hawkey
#include "goal.h"
#include "iutil.h"
#include "query.h"
#include "package_internal.h"
#include "sack_internal.h"

struct _HyGoal {
    HySack sack;
    Queue job;
    Queue problems;
    Transaction *trans;
};

static HyPackageList
list_results(HyGoal goal, Id type_filter)
{
    Pool *pool = sack_pool(goal->sack);
    Queue transpkgs;
    Transaction *trans = goal->trans;
    HyPackageList plist;

    assert(trans);
    queue_init(&transpkgs);
    plist = hy_packagelist_create();

    for (int i = 0; i < trans->steps.count; ++i) {
	Id p = trans->steps.elements[i];
	Id type = transaction_type(trans, p, SOLVER_TRANSACTION_SHOW_ACTIVE);

	if (type == type_filter)
	    hy_packagelist_push(plist, package_create(pool, p));
    }
    return plist;
}

HyGoal
hy_goal_create(HySack sack)
{
    HyGoal goal = solv_calloc(1, sizeof(*goal));
    goal->sack = sack;
    queue_init(&goal->job);
    queue_init(&goal->problems);
    return goal;
}

void
hy_goal_free(HyGoal goal)
{
    queue_free(&goal->problems);
    queue_free(&goal->job);
    if (goal->trans)
	transaction_free(goal->trans);
    solv_free(goal);
}

int
hy_goal_erase(HyGoal goal, HyPackage pkg)
{
#ifndef NDEBUG
    Pool *pool = sack_pool(goal->sack);
    assert(pool->installed &&
	   pool_id2solvable(pool, package_id(pkg))->repo == pool->installed);
#endif
    queue_push2(&goal->job, SOLVER_SOLVABLE|SOLVER_ERASE, package_id(pkg));
    return 0;
}

int
hy_goal_install(HyGoal goal, HyPackage new_pkg)
{
    queue_push2(&goal->job, SOLVER_SOLVABLE|SOLVER_INSTALL, package_id(new_pkg));
    return 0;
}

int
hy_goal_update(HyGoal goal, HyPackage new_pkg)
{
    HyQuery q = hy_query_create(goal->sack);
    const char *name = hy_package_get_name(new_pkg);
    HyPackageList installed;
    int count;

    hy_query_filter(q, KN_PKG_NAME, FT_EQ, name);
    hy_query_filter(q, KN_PKG_REPO, FT_EQ, SYSTEM_REPO_NAME);
    installed = hy_query_run(q);
    count = hy_packagelist_count(installed);
    hy_packagelist_free(installed);
    hy_query_free(q);

    if (count)
	return hy_goal_install(goal, new_pkg);
    return 1;
}

int
hy_goal_go(HyGoal goal)
{
    Transaction *trans;

    trans = job2transaction(goal->sack, &goal->job, &goal->problems);
    if (trans == NULL)
	return 1;
#if 0
    transaction_print(trans);
    Pool *pool = sack_pool(goal->sack);
    int i;

    for (i = 0; i < trans->steps.count; ++i) {
	Id p = trans->steps.elements[i];
	Solvable *s = pool_id2solvable(pool, p);
	Id type = transaction_type(trans, p, SOLVER_TRANSACTION_RPM_ONLY);
	switch (type) {
	case SOLVER_TRANSACTION_ERASE:
	    printf("erasing id %d\n", p);
	    break;
	case SOLVER_TRANSACTION_INSTALL:
	    printf("installing %d\n", p);
	    printf("\t %s %s\n", s->repo->name,
		   solvable_get_location(s, NULL));
	    break;
	default:
	    printf("unrecognized type 0x%x for %d\n", type, p);
	    printf("\t %s\n", pool_solvable2str(pool, s));
	    break;
	}
    }
#endif

    goal->trans = trans;
    return 0;
}

int
hy_goal_count_problems(HyGoal goal)
{
    return (goal->problems.count/4);
}

char *
hy_goal_describe_problem(HyGoal goal, unsigned i)
{
    Pool *pool = sack_pool(goal->sack);
    Id *ps = goal->problems.elements;

    assert(goal->problems.count/4 > i);
    return problemruleinfo2str(pool, ps[i], ps[i+1], ps[i+2], ps[i+3]);
}

HyPackageList
hy_goal_list_erasures(HyGoal goal)
{
    return list_results(goal, SOLVER_TRANSACTION_ERASE);
}

HyPackageList
hy_goal_list_installs(HyGoal goal)
{
    return list_results(goal, SOLVER_TRANSACTION_INSTALL);
}

HyPackageList
hy_goal_list_upgrades(HyGoal goal)
{
    return list_results(goal, SOLVER_TRANSACTION_UPGRADE);
}

HyPackage
hy_goal_package_upgrades(HyGoal goal, HyPackage pkg)
{
    Pool *pool = sack_pool(goal->sack);
    Transaction *trans = goal->trans;
    Id p;

    assert(trans);
    p = transaction_obs_pkg(trans, package_id(pkg));
    assert(p); // todo: handle no upgrades case
    return package_create(pool, p);
}
