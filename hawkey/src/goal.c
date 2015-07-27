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

#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

// libsolv
#include <solv/evr.h>
#include <solv/selection.h>
#include <solv/solver.h>
#include <solv/solverdebug.h>
#include <solv/testcase.h>
#include <solv/transaction.h>
#include <solv/util.h>

// hawkey
#include "errno_internal.h"
#include "goal_internal.h"
#include "iutil.h"
#include "package_internal.h"
#include "query_internal.h"
#include "reldep_internal.h"
#include "repo_internal.h"
#include "sack_internal.h"
#include "selector_internal.h"
#include "util.h"

struct _HyGoal {
    HySack sack;
    Queue staging;
    Solver *solv;
    Transaction *trans;
    int actions;
};

struct _SolutionCallback {
    HyGoal goal;
    hy_solution_callback callback;
    void *callback_data;
};

struct InstallonliesSortCallback {
    Pool *pool;
    Id running_kernel;
};

static int
erase_flags2libsolv(int flags)
{
    int ret = 0;
    if (flags & HY_CLEAN_DEPS)
	ret |= SOLVER_CLEANDEPS;
    return ret;
}

static void
same_name_subqueue(Pool *pool, Queue *in, Queue *out)
{
    Id el = queue_pop(in);
    Id name = pool_id2solvable(pool, el)->name;
    queue_empty(out);
    queue_push(out, el);
    while (in->count &&
	   pool_id2solvable(pool, in->elements[in->count - 1])->name == name)
	// reverses the order so packages are sorted by descending version
	queue_push(out, queue_pop(in));
}

static int
can_depend_on(Pool *pool, Solvable *sa, Id b)
{
    // return 0 iff a does not depend on anything from b
    Queue requires;
    int ret = 1;

    queue_init(&requires);
    solvable_lookup_idarray(sa, SOLVABLE_REQUIRES, &requires);
    for (int i = 0; i < requires.count; ++i) {
	Id req_dep = requires.elements[i];
	Id p, pp;

	FOR_PROVIDES(p, pp, req_dep)
	    if (p == b)
		goto done;
    }

    ret = 0;
 done:
    queue_free(&requires);
    return ret;
}

static int
sort_packages(const void *ap, const void *bp, void *s_cb)
{
    Id a = *(Id*)ap;
    Id b = *(Id*)bp;
    Pool *pool = ((struct InstallonliesSortCallback*) s_cb)->pool;
    Id kernel = ((struct InstallonliesSortCallback*) s_cb)->running_kernel;
    Solvable *sa = pool_id2solvable(pool, a);
    Solvable *sb = pool_id2solvable(pool, b);

    /* if the names are different sort them differently, particular order does
       not matter as long as it's consistent. */
    int name_diff = sa->name - sb->name;
    if (name_diff)
	return name_diff;

    /* same name, if one is/depends on the running kernel put it last */
    if (kernel >= 0) {
	if (a == kernel || can_depend_on(pool, sa, kernel))
	    return 1;
	if (b == kernel || can_depend_on(pool, sb, kernel))
	    return -1;
    }

    return pool_evrcmp(pool, sa->evr, sb->evr, EVRCMP_COMPARE);
}

static int
limit_installonly_packages(HyGoal goal, Solver *solv, Queue *job)
{
    HySack sack = goal->sack;
    if (!sack->installonly_limit)
	return 0;

    Queue *onlies = &sack->installonly;
    Pool *pool = sack_pool(sack);
    int reresolve = 0;

    for (int i = 0; i < onlies->count; ++i) {
	Id p, pp;
	Queue q;
	queue_init(&q);

	FOR_PKG_PROVIDES(p, pp, onlies->elements[i])
	    if (solver_get_decisionlevel(solv, p) > 0)
		queue_push(&q, p);
	if (q.count <= sack->installonly_limit) {
	    queue_free(&q);
	    continue;
	}

	struct InstallonliesSortCallback s_cb = {pool, sack_running_kernel(sack)};
	qsort_r(q.elements, q.count, sizeof(q.elements[0]), sort_packages, &s_cb);
	Queue same_names;
	queue_init(&same_names);
	while (q.count > 0) {
	    same_name_subqueue(pool, &q, &same_names);
	    if (same_names.count <= sack->installonly_limit)
		continue;
	    reresolve = 1;
	    for (int j = 0; j < same_names.count; ++j) {
		Id id  = same_names.elements[j];
		Id action = SOLVER_ERASE;
		if (j < sack->installonly_limit)
		    action = SOLVER_INSTALL;
		queue_push2(job, action | SOLVER_SOLVABLE, id);
	    }
	}
	queue_free(&same_names);
	queue_free(&q);
    }
    return reresolve;
}

static int
internal_solver_callback(Solver *solv, void *data)
{
    struct _SolutionCallback *s_cb = (struct _SolutionCallback*)data;
    HyGoal goal = s_cb->goal;

    assert(goal->solv == solv);
    assert(goal->trans == NULL);
    goal->trans = solver_create_transaction(solv);
    int ret = s_cb->callback(goal, s_cb->callback_data);
    transaction_free(goal->trans);
    goal->trans = NULL;
    return ret;
}

static Solver *
init_solver(HyGoal goal, int flags)
{
    Pool *pool = sack_pool(goal->sack);
    Solver *solv = solver_create(pool);

    if (goal->solv)
	solver_free(goal->solv);
    goal->solv = solv;

    if (flags & HY_ALLOW_UNINSTALL)
	solver_set_flag(solv, SOLVER_FLAG_ALLOW_UNINSTALL, 1);
    /* no vendor locking */
    solver_set_flag(solv, SOLVER_FLAG_ALLOW_VENDORCHANGE, 1);
    /* don't erase packages that are no longer in repo during distupgrade */
    solver_set_flag(solv, SOLVER_FLAG_KEEP_ORPHANS, 1);
    /* no arch change for forcebest */
    solver_set_flag(solv, SOLVER_FLAG_BEST_OBEY_POLICY, 1);
    /* support package splits via obsoletes */
    solver_set_flag(solv, SOLVER_FLAG_YUM_OBSOLETES, 1);

    return solv;
}

static int
solve(HyGoal goal, Queue *job, int flags, hy_solution_callback user_cb,
      void * user_cb_data)
{
    HySack sack = goal->sack;
    struct _SolutionCallback cb_tuple;

    /* apply the excludes */
    sack_recompute_considered(sack);

    repo_internalize_all_trigger(sack_pool(sack));
    sack_make_provides_ready(sack);
    if (goal->trans) {
	transaction_free(goal->trans);
	goal->trans = NULL;
    }

    Solver *solv = init_solver(goal, flags);
    if (user_cb) {
	cb_tuple = (struct _SolutionCallback){goal, user_cb, user_cb_data};
	solv->solution_callback = internal_solver_callback;
	solv->solution_callback_data = &cb_tuple;
    }

    if (HY_IGNORE_WEAK_DEPS & flags)
        solver_set_flag(solv, SOLVER_FLAG_IGNORE_RECOMMENDED, 1);

    if (solver_solve(solv, job))
	return 1;
    // either allow solutions callback or installonlies, both at the same time
    // are not supported
    if (!user_cb && limit_installonly_packages(goal, solv, job)) {
	// allow erasing non-installonly packages that depend on a kernel about
	// to be erased
	solver_set_flag(solv, SOLVER_FLAG_ALLOW_UNINSTALL, 1);
	if (solver_solve(solv, job))
	    return 1;
    }
    goal->trans = solver_create_transaction(solv);
    return 0;
}

static Queue *
construct_job(HyGoal goal, int flags)
{
    HySack sack = goal->sack;
    Queue *job = solv_malloc(sizeof(*job));

    queue_init_clone(job, &goal->staging);

    /* apply forcebest */
    if (flags & HY_FORCE_BEST)
	for (int i = 0; i < job->count; i += 2)
	    job->elements[i] |= SOLVER_FORCEBEST;

    /* turn off implicit obsoletes for installonly packages */
    for (int i = 0; i < sack->installonly.count; i++)
	queue_push2(job, SOLVER_MULTIVERSION|SOLVER_SOLVABLE_PROVIDES,
		    sack->installonly.elements[i]);

    if (flags & HY_VERIFY)
        queue_push2(job, SOLVER_VERIFY|SOLVER_SOLVABLE_ALL, 0);

    return job;
}

static void
free_job(Queue *job)
{
    queue_free(job);
    solv_free(job);
}

static HyPackageList
list_results(HyGoal goal, Id type_filter1, Id type_filter2)
{
    Queue transpkgs;
    Transaction *trans = goal->trans;
    HyPackageList plist;

    if (!trans) {
	if (!goal->solv)
	    hy_errno = HY_E_OP;
	else
	    hy_errno = HY_E_NO_SOLUTION;
	return NULL;
    }
    queue_init(&transpkgs);
    plist = hy_packagelist_create();
    const int common_mode = SOLVER_TRANSACTION_SHOW_OBSOLETES |
	SOLVER_TRANSACTION_CHANGE_IS_REINSTALL;

    for (int i = 0; i < trans->steps.count; ++i) {
	Id p = trans->steps.elements[i];
	Id type;

	switch (type_filter1) {
	case SOLVER_TRANSACTION_OBSOLETED:
	    type =  transaction_type(trans, p, common_mode);
	    break;
	default:
	    type  = transaction_type(trans, p, common_mode |
				     SOLVER_TRANSACTION_SHOW_ACTIVE|
				     SOLVER_TRANSACTION_SHOW_ALL);
	    break;
	}

	if (type == type_filter1 || (type_filter2 && type == type_filter2))
	    hy_packagelist_push(plist, package_create(goal->sack, p));
    }
    return plist;
}

// internal functions to translate Selector into libsolv Job

static int
job_has(Queue *job, Id what, Id id)
{
    for (int i = 0; i < job->count; i += 2)
	if (job->elements[i] == what && job->elements[i + 1] == id)
	    return 1;
    return 0;
}

static int
filter_arch2job(HySack sack, const struct _Filter *f, Queue *job)
{
    if (f == NULL)
	return 0;

    assert(f->cmp_type == HY_EQ);
    assert(f->nmatches == 1);
    Pool *pool = sack_pool(sack);
    const char *arch = f->matches[0].str;
    Id archid = str2archid(pool, arch);

    if (archid == 0)
	return HY_E_ARCH;
    for (int i = 0; i < job->count; i += 2) {
	Id dep;
	assert((job->elements[i] & SOLVER_SELECTMASK) == SOLVER_SOLVABLE_NAME);
	dep = pool_rel2id(pool, job->elements[i + 1],
			  archid, REL_ARCH, 1);
	job->elements[i] |= SOLVER_SETARCH;
	job->elements[i + 1] = dep;
    }
    return 0;
}

static int
filter_evr2job(HySack sack, const struct _Filter *f, Queue *job)
{
    if (f == NULL)
	return 0;

    assert(f->cmp_type == HY_EQ);
    assert(f->nmatches == 1);

    Pool *pool = sack_pool(sack);
    Id evr = pool_str2id(pool, f->matches[0].str, 1);
    Id constr = f->keyname == HY_PKG_VERSION ? SOLVER_SETEV : SOLVER_SETEVR;
    for (int i = 0; i < job->count; i += 2) {
	Id dep;
	assert((job->elements[i] & SOLVER_SELECTMASK) == SOLVER_SOLVABLE_NAME);
	dep = pool_rel2id(pool, job->elements[i + 1],
			  evr, REL_EQ, 1);
	job->elements[i] |= constr;
	job->elements[i + 1] = dep;
    }
    return 0;
}

static int
filter_file2job(HySack sack, const struct _Filter *f, Queue *job)
{
    if (f == NULL)
	return 0;
    assert(f->nmatches == 1);

    const char *file = f->matches[0].str;
    Pool *pool = sack_pool(sack);

    int flags = f->cmp_type & HY_GLOB ? SELECTION_GLOB : 0;
    if (f->cmp_type & HY_GLOB)
	flags |= SELECTION_NOCASE;
    if (selection_make(pool, job, file, flags | SELECTION_FILELIST) == 0)
	return 1;
    return 0;
}

static int
filter_name2job(HySack sack, const struct _Filter *f, Queue *job)
{
    if (f == NULL)
	return 0;
    assert(f->nmatches == 1);

    Pool *pool = sack_pool(sack);
    const char *name = f->matches[0].str;
    Id id;
    Dataiterator di;

    switch (f->cmp_type) {
    case HY_EQ:
	id = pool_str2id(pool, name, 0);
	if (id)
	    queue_push2(job, SOLVER_SOLVABLE_NAME, id);
	break;
    case HY_GLOB:
	dataiterator_init(&di, pool, 0, 0, SOLVABLE_NAME, name, SEARCH_GLOB);
	while (dataiterator_step(&di)) {
	    if (!is_package(pool, pool_id2solvable(pool, di.solvid)))
	        continue;
	    assert(di.idp);
	    id = *di.idp;
	    if (job_has(job, SOLVABLE_NAME, id))
		continue;
	    queue_push2(job, SOLVER_SOLVABLE_NAME, id);
	}
	dataiterator_free(&di);
	break;
    default:
	assert(0);
	return 1;
    }
    return 0;
}

static int
filter_provides2job(HySack sack, const struct _Filter *f, Queue *job)
{
    if (f == NULL)
	return 0;
    assert(f->nmatches == 1);

    Pool *pool = sack_pool(sack);
    const char *name = f->matches[0].str;
    Id id;
    Dataiterator di;

    switch (f->cmp_type) {
    case HY_EQ:
	id = reldep_id(f->matches[0].reldep);
	queue_push2(job, SOLVER_SOLVABLE_PROVIDES, id);
	break;
    case HY_GLOB:
	dataiterator_init(&di, pool, 0, 0, SOLVABLE_PROVIDES, name, SEARCH_GLOB);
	while (dataiterator_step(&di)) {
	    if (is_package(pool, pool_id2solvable(pool, di.solvid)))
		break;
	}
	assert(di.idp);
	id = *di.idp;
	if (!job_has(job, SOLVABLE_PROVIDES, id))
	    queue_push2(job, SOLVER_SOLVABLE_PROVIDES, id);
	dataiterator_free(&di);
	break;
    default:
	assert(0);
	return 1;
    }
    return 0;
}

static int
filter_reponame2job(HySack sack, const struct _Filter *f, Queue *job)
{
    Queue repo_sel;
    Id i;
    Repo *repo;

    if (f == NULL)
        return 0;

    assert(f->cmp_type == HY_EQ);
    assert(f->nmatches == 1);

    queue_init(&repo_sel);
    Pool *pool = sack_pool(sack);
    FOR_REPOS(i, repo)
        if (!strcmp(f->matches[0].str, repo->name)) {
            queue_push2(&repo_sel, SOLVER_SOLVABLE_REPO | SOLVER_SETREPO, repo->repoid);
        }

    selection_filter(pool, job, &repo_sel);

    queue_free(&repo_sel);
    return 0;
}

/**
 * Build job queue from a Query.
 *
 * Returns 0 on success. Otherwise it returns non-zero and sets hy_errno.
 */
int
sltr2job(const HySelector sltr, Queue *job, int solver_action)
{
    HySack sack = selector_sack(sltr);
    int ret = 0;
    Queue job_sltr;
    int any_opt_filter = sltr->f_arch || sltr->f_evr || sltr->f_reponame;
    int any_req_filter = sltr->f_name || sltr->f_provides || sltr->f_file;

    queue_init(&job_sltr);

    if (!any_req_filter) {
	if (any_opt_filter)
	    // no name or provides or file in the selector is an error
	    ret = HY_E_SELECTOR;
	goto finish;
    }

    sack_recompute_considered(sack);
    sack_make_provides_ready(sack);
    ret = filter_name2job(sack, sltr->f_name, &job_sltr);
    if (ret)
	goto finish;
    ret = filter_file2job(sack, sltr->f_file, &job_sltr);
    if (ret)
	goto finish;
    ret = filter_provides2job(sack, sltr->f_provides, &job_sltr);
    if (ret)
	goto finish;
    ret = filter_arch2job(sack, sltr->f_arch, &job_sltr);
    if (ret)
	goto finish;
    ret = filter_evr2job(sack, sltr->f_evr, &job_sltr);
    if (ret)
	goto finish;
    ret = filter_reponame2job(sack, sltr->f_reponame, &job_sltr);
    if (ret)
        goto finish;

    for (int i = 0; i < job_sltr.count; i += 2)
 	queue_push2(job,
 		    job_sltr.elements[i] | solver_action,
 		    job_sltr.elements[i + 1]);

 finish:
    if (ret)
 	hy_errno = ret;
    queue_free(&job_sltr);
    return ret;
}

// public functions

HyGoal
hy_goal_clone(HyGoal goal)
{
    HyGoal gn = hy_goal_create(goal->sack);
    queue_init_clone(&gn->staging, &goal->staging);
    gn->actions = goal->actions;
    return gn;
}

HyGoal
hy_goal_create(HySack sack)
{
    HyGoal goal = solv_calloc(1, sizeof(*goal));
    goal->sack = sack;
    queue_init(&goal->staging);
    return goal;
}

void
hy_goal_free(HyGoal goal)
{
    if (goal->trans)
	transaction_free(goal->trans);
    if (goal->solv)
	solver_free(goal->solv);
    queue_free(&goal->staging);
    solv_free(goal);
}

int
hy_goal_distupgrade_all(HyGoal goal)
{
    goal->actions |= HY_DISTUPGRADE_ALL;
    queue_push2(&goal->staging, SOLVER_DISTUPGRADE|SOLVER_SOLVABLE_ALL, 0);
    return 0;
}

int
hy_goal_distupgrade(HyGoal goal, HyPackage new_pkg)
{
    goal->actions |= HY_DISTUPGRADE;
    queue_push2(&goal->staging, SOLVER_SOLVABLE|SOLVER_DISTUPGRADE,
	package_id(new_pkg));
    return 0;
}

int
hy_goal_distupgrade_selector(HyGoal goal, HySelector sltr)
{
    goal->actions |= HY_DISTUPGRADE;
    return sltr2job(sltr, &goal->staging, SOLVER_DISTUPGRADE);
}

int
hy_goal_downgrade_to(HyGoal goal, HyPackage new_pkg)
{
    goal->actions |= HY_DOWNGRADE;
    return hy_goal_install(goal, new_pkg);
}

int
hy_goal_erase(HyGoal goal, HyPackage pkg)
{
    goal->actions |= HY_ERASE;
    return hy_goal_erase_flags(goal, pkg, 0);
}

int
hy_goal_erase_flags(HyGoal goal, HyPackage pkg, int flags)
{
#ifndef NDEBUG
    Pool *pool = sack_pool(goal->sack);
    assert(pool->installed &&
	   pool_id2solvable(pool, package_id(pkg))->repo == pool->installed);
#endif
    int additional = erase_flags2libsolv(flags);
    goal->actions |= HY_ERASE;
    queue_push2(&goal->staging, SOLVER_SOLVABLE|SOLVER_ERASE|additional,
		package_id(pkg));
    return 0;
}

int
hy_goal_erase_selector(HyGoal goal, HySelector sltr)
{
    goal->actions |= HY_ERASE;
    return hy_goal_erase_selector_flags(goal, sltr, 0);
}

int
hy_goal_erase_selector_flags(HyGoal goal, HySelector sltr, int flags)
{
    int additional = erase_flags2libsolv(flags);
    goal->actions |= HY_ERASE;
    return sltr2job(sltr, &goal->staging, SOLVER_ERASE|additional);
}

int
hy_goal_has_actions(HyGoal goal, int action)
{
    return goal->actions & action;
}

int
hy_goal_install(HyGoal goal, HyPackage new_pkg)
{
    goal->actions |= HY_INSTALL;
    queue_push2(&goal->staging, SOLVER_SOLVABLE|SOLVER_INSTALL, package_id(new_pkg));
    return 0;
}

int
hy_goal_install_optional(HyGoal goal, HyPackage new_pkg)
{
    goal->actions |= HY_INSTALL;
    queue_push2(&goal->staging, SOLVER_SOLVABLE|SOLVER_INSTALL|SOLVER_WEAK,
		package_id(new_pkg));
    return 0;
}

int
hy_goal_install_selector(HyGoal goal, HySelector sltr)
{
    goal->actions |= HY_INSTALL;
    return sltr2job(sltr, &goal->staging, SOLVER_INSTALL);
}

int
hy_goal_install_selector_optional(HyGoal goal, HySelector sltr)
{
    goal->actions |= HY_INSTALL;
    return sltr2job(sltr, &goal->staging, SOLVER_INSTALL|SOLVER_WEAK);
}

int
hy_goal_upgrade_all(HyGoal goal)
{
    goal->actions |= HY_UPGRADE_ALL;
    queue_push2(&goal->staging, SOLVER_UPDATE|SOLVER_SOLVABLE_ALL, 0);
    return 0;
}

int
hy_goal_upgrade_to(HyGoal goal, HyPackage new_pkg)
{
    goal->actions |= HY_UPGRADE;
    return hy_goal_upgrade_to_flags(goal, new_pkg, 0);
}

int
hy_goal_upgrade_to_selector(HyGoal goal, HySelector sltr)
{
    goal->actions |= HY_UPGRADE;
    if (sltr->f_evr == NULL)
	return sltr2job(sltr, &goal->staging, SOLVER_UPDATE);
    return sltr2job(sltr, &goal->staging, SOLVER_INSTALL);
}

int
hy_goal_upgrade_selector(HyGoal goal, HySelector sltr)
{
    goal->actions |= HY_UPGRADE;
    return sltr2job(sltr, &goal->staging, SOLVER_UPDATE);
}

int
hy_goal_upgrade_to_flags(HyGoal goal, HyPackage new_pkg, int flags)
{
    int count = 0;

    if (flags & HY_CHECK_INSTALLED) {
	HyQuery q = hy_query_create(goal->sack);
	const char *name = hy_package_get_name(new_pkg);
	HyPackageList installed;

	hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
	hy_query_filter(q, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
	installed = hy_query_run(q);
	count = hy_packagelist_count(installed);
	hy_packagelist_free(installed);
	hy_query_free(q);
	if (!count) {
	    hy_errno = HY_E_VALIDATION;
	    return HY_E_VALIDATION;
	}
    }
    goal->actions |= HY_UPGRADE;

    return hy_goal_install(goal, new_pkg);
}

int
hy_goal_userinstalled(HyGoal goal, HyPackage pkg)
{
    queue_push2(&goal->staging, SOLVER_SOLVABLE|SOLVER_USERINSTALLED,
		package_id(pkg));
    return 0;
}

// deprecated in 0.5.9, will be removed in 1.0.0
// use hy_goal_has_actions(goal, HY_DISTUPGRADE_ALL) instead
int
hy_goal_req_has_distupgrade_all(HyGoal goal)
{
    return hy_goal_has_actions(goal, HY_DISTUPGRADE_ALL);
}

// deprecated in 0.5.9, will be removed in 1.0.0
// use hy_goal_has_actions(goal, HY_ERASE) instead
int
hy_goal_req_has_erase(HyGoal goal)
{
    return hy_goal_has_actions(goal, HY_ERASE);
}

// deprecated in 0.5.9, will be removed in 1.0.0
// use hy_goal_has_actions(goal, HY_UPGRADE_ALL) instead
int
hy_goal_req_has_upgrade_all(HyGoal goal)
{
    return hy_goal_has_actions(goal, HY_UPGRADE_ALL);
}

int hy_goal_req_length(HyGoal goal)
{
    return goal->staging.count / 2;
}

int
hy_goal_run(HyGoal goal)
{
    return hy_goal_run_flags(goal, 0);
}

int
hy_goal_run_flags(HyGoal goal, int flags)
{
    return hy_goal_run_all_flags(goal, NULL, NULL, flags);
}

int
hy_goal_run_all(HyGoal goal, hy_solution_callback cb, void *cb_data)
{
    return hy_goal_run_all_flags(goal, cb, cb_data, 0);
}

int
hy_goal_run_all_flags(HyGoal goal, hy_solution_callback cb, void *cb_data,
		      int flags)
{
    Queue *job = construct_job(goal, flags);
    int ret = solve(goal, job, flags, cb, cb_data);
    free_job(job);
    return ret;
}

int
hy_goal_count_problems(HyGoal goal)
{
    assert(goal->solv);
    return solver_problem_count(goal->solv);
}

/**
 * String describing the encountered solving problem 'i'.
 *
 * Caller is responsible for freeing the returned string using hy_free().
 */
char *
hy_goal_describe_problem(HyGoal goal, unsigned i)
{
    Id rid, source, target, dep;
    SolverRuleinfo type;

    if (i >= hy_goal_count_problems(goal)) {
	hy_errno = HY_E_OP;
	return NULL;
    }

    // this libsolv interface indexes from 1 (we do from 0), so:
    rid = solver_findproblemrule(goal->solv, i + 1);
    type = solver_ruleinfo(goal->solv, rid, &source, &target, &dep);

    const char *problem = solver_problemruleinfo2str(goal->solv,
						     type, source, target, dep);
    return solv_strdup(problem);
}

/**
 * Write all the solving decisions to the hawkey logfile.
 */
int
hy_goal_log_decisions(HyGoal goal)
{
    if (goal->solv == NULL)
	return 1;
    solver_printdecisionq(goal->solv, SOLV_DEBUG_RESULT);
    return 0;
}

int
hy_goal_write_debugdata(HyGoal goal, const char *dir)
{
    HySack sack = goal->sack;
    Solver *solv = goal->solv;
    if (solv == NULL)
	return HY_E_OP;

    int flags = TESTCASE_RESULT_TRANSACTION | TESTCASE_RESULT_PROBLEMS;
    char *absdir = abspath(dir);
    if (absdir == NULL)
	return hy_errno;
    HY_LOG_INFO("writing solver debugdata to %s", absdir);
    int ret = testcase_write(solv, absdir, flags, NULL, NULL);
    if (!ret) {
	format_err_str("Failed writing debugdata to %s: %s.", absdir,
		       strerror(errno));
	return HY_E_IO;
    }
    hy_free(absdir);
    return 0;
}

HyPackageList
hy_goal_list_erasures(HyGoal goal)
{
    return list_results(goal, SOLVER_TRANSACTION_ERASE, 0);
}

HyPackageList
hy_goal_list_installs(HyGoal goal)
{
    return list_results(goal, SOLVER_TRANSACTION_INSTALL,
			SOLVER_TRANSACTION_OBSOLETES);
}

HyPackageList
hy_goal_list_obsoleted(HyGoal goal)
{
    return list_results(goal, SOLVER_TRANSACTION_OBSOLETED, 0);
}

HyPackageList
hy_goal_list_reinstalls(HyGoal goal)
{
    return list_results(goal, SOLVER_TRANSACTION_REINSTALL, 0);
}

HyPackageList
hy_goal_list_unneeded(HyGoal goal)
{
    HyPackageList plist = hy_packagelist_create();
    Queue q;
    Solver *solv = goal->solv;

    queue_init(&q);
    solver_get_unneeded(solv, &q, 0);
    queue2plist(goal->sack, &q, plist);
    queue_free(&q);
    return plist;
}

HyPackageList
hy_goal_list_upgrades(HyGoal goal)
{
    return list_results(goal, SOLVER_TRANSACTION_UPGRADE, 0);
}

HyPackageList
hy_goal_list_downgrades(HyGoal goal)
{
    return list_results(goal, SOLVER_TRANSACTION_DOWNGRADE, 0);
}

HyPackageList
hy_goal_list_obsoleted_by_package(HyGoal goal, HyPackage pkg)
{
    HySack sack = goal->sack;
    Transaction *trans = goal->trans;
    Queue obsoletes;
    HyPackageList plist = hy_packagelist_create();

    assert(trans);
    queue_init(&obsoletes);

    transaction_all_obs_pkgs(trans, package_id(pkg), &obsoletes);
    queue2plist(sack, &obsoletes, plist);

    queue_free(&obsoletes);
    return plist;
}

int
hy_goal_get_reason(HyGoal goal, HyPackage pkg)
{
    assert(goal->solv);
    Id info;
    int reason = solver_describe_decision(goal->solv, package_id(pkg), &info);

    if ((reason == SOLVER_REASON_UNIT_RULE ||
	 reason == SOLVER_REASON_RESOLVE_JOB) &&
	solver_ruleclass(goal->solv, info) == SOLVER_RULE_JOB)
	return HY_REASON_USER;
    return HY_REASON_DEP;
}
