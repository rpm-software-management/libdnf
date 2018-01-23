/*
 * Copyright (C) 2012-2018 Red Hat, Inc.
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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

// libsolv
extern "C" {
#include <solv/evr.h>
#include <solv/policy.h>
#include <solv/selection.h>
#include <solv/solver.h>
#include <solv/solverdebug.h>
#include <solv/testcase.h>
#include <solv/transaction.h>
#include <solv/util.h>
#include <solv/poolid.h>
}

// hawkey
#include "dnf-types.h"
#include "hy-goal-private.hpp"
#include "hy-iutil-private.hpp"
#include "hy-package-private.hpp"
#include "hy-packageset-private.hpp"
#include "hy-query-private.hpp"
#include "hy-repo-private.hpp"
#include "dnf-sack-private.hpp"
#include "dnf-goal.h"
#include "dnf-package.h"
#include "hy-selector-private.hpp"
#include "hy-util-private.hpp"
#include "dnf-package.h"
#include "hy-package.h"
#include "sack/packageset.hpp"

#include "utils/bgettext/bgettext-lib.h"

#define BLOCK_SIZE 15

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

static gboolean
protected_in_removals(HyGoal goal) {
    guint i = 0;
    gboolean ret = FALSE;
    if (goal->removal_of_protected != NULL)
        g_ptr_array_free(goal->removal_of_protected, TRUE);
    if (goal->protected_pkgs == NULL)
        return FALSE;
    goal->removal_of_protected = dnf_goal_get_packages(goal,
                                     DNF_PACKAGE_INFO_REMOVE,
                                     DNF_PACKAGE_INFO_OBSOLETE,
                                     -1);
    while (i < goal->removal_of_protected->len) {
        DnfPackage *pkg = (DnfPackage*)g_ptr_array_index(goal->removal_of_protected, i);

        if (MAPTST(goal->protected_pkgs, dnf_package_get_id(pkg))) {
            ret = TRUE;
            i++;
        } else {
            g_ptr_array_remove_index(goal->removal_of_protected, i);
        }
    }
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

    /* move available packages to end of the list */
    if (pool->installed != sa->repo)
        return 1;

    if (pool->installed != sb->repo)
        return -1;

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
    DnfSack *sack = goal->sack;
    if (!dnf_sack_get_installonly_limit(sack))
        return 0;

    Queue *onlies = dnf_sack_get_installonly(sack);
    Pool *pool = dnf_sack_get_pool(sack);
    int reresolve = 0;

    for (int i = 0; i < onlies->count; ++i) {
        Id p, pp;
        Queue q, installing;
        queue_init(&q);
        queue_init(&installing);

        FOR_PKG_PROVIDES(p, pp, onlies->elements[i])
            if (solver_get_decisionlevel(solv, p) > 0)
                queue_push(&q, p);
        if (q.count <= (int) dnf_sack_get_installonly_limit(sack)) {
            queue_free(&q);
            queue_free(&installing);
            continue;
        }
        for (int k = 0; k < q.count; ++k) {
            Id id  = q.elements[k];
            Solvable *s = pool_id2solvable(pool, id);
            if (pool->installed != s->repo) {
                queue_push(&installing, id);
                break;
            }
        }
        if (!installing.count) {
            queue_free(&q);
            queue_free(&installing);
            continue;
        }

        struct InstallonliesSortCallback s_cb = {pool, dnf_sack_running_kernel(sack)};
        solv_sort(q.elements, q.count, sizeof(q.elements[0]), sort_packages, &s_cb);
        Queue same_names;
        queue_init(&same_names);
        while (q.count > 0) {
            same_name_subqueue(pool, &q, &same_names);
            if (same_names.count <= (int) dnf_sack_get_installonly_limit(sack))
                continue;
            reresolve = 1;
            for (int j = 0; j < same_names.count; ++j) {
                Id id  = same_names.elements[j];
                Id action = SOLVER_ERASE;
                if (j < (int) dnf_sack_get_installonly_limit(sack))
                    action = SOLVER_INSTALL;
                queue_push2(job, action | SOLVER_SOLVABLE, id);
            }
        }
        queue_free(&same_names);
        queue_free(&q);
        queue_free(&installing);
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
init_solver(HyGoal goal, DnfGoalActions flags)
{
    Pool *pool = dnf_sack_get_pool(goal->sack);
    Solver *solv = solver_create(pool);

    if (goal->solv)
        solver_free(goal->solv);
    goal->solv = solv;

    /* no vendor locking */
    solver_set_flag(solv, SOLVER_FLAG_ALLOW_VENDORCHANGE, 1);
    /* don't erase packages that are no longer in repo during distupgrade */
    solver_set_flag(solv, SOLVER_FLAG_KEEP_ORPHANS, 1);
    /* no arch change for forcebest */
    solver_set_flag(solv, SOLVER_FLAG_BEST_OBEY_POLICY, 1);
    /* support package splits via obsoletes */
    solver_set_flag(solv, SOLVER_FLAG_YUM_OBSOLETES, 1);

#if defined(LIBSOLV_FLAG_URPMREORDER)
    /* support urpm-like solution reordering */
    solver_set_flag(solv, SOLVER_FLAG_URPM_REORDER, 1);
#endif

    return solv;
}

static void
allow_uninstall_all_but_protected(HyGoal goal, Queue *job, DnfGoalActions flags)
{
    Pool *pool = dnf_sack_get_pool(goal->sack);

    if (goal->protected_pkgs == NULL) {
        goal->protected_pkgs = (Map*)g_malloc0(sizeof(Map));
        map_init(goal->protected_pkgs, pool->nsolvables);
    } else
        map_grow(goal->protected_pkgs, pool->nsolvables);

    assert(goal->protected_pkgs != NULL);
    Id kernel = dnf_sack_running_kernel(goal->sack);
    if (kernel > 0)
        MAPSET(goal->protected_pkgs, kernel);

    if (DNF_ALLOW_UNINSTALL & flags)
        for (Id id = 1; id < pool->nsolvables; ++id) {
            Solvable *s = pool_id2solvable(pool, id);
            if (pool->installed == s->repo && !MAPTST(goal->protected_pkgs, id) &&
                (pool->considered == NULL || MAPTST(pool->considered, id))) {
                queue_push2(job, SOLVER_ALLOWUNINSTALL|SOLVER_SOLVABLE, id);
            }
        }
}

static int
solve(HyGoal goal, Queue *job, DnfGoalActions flags,
      hy_solution_callback user_cb, void * user_cb_data) {
    DnfSack *sack = goal->sack;
    struct _SolutionCallback cb_tuple;

    /* apply the excludes */
    dnf_sack_recompute_considered(sack);

    dnf_sack_make_provides_ready(sack);
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

    /* Removal of SOLVER_WEAK to allow report errors*/
    if (DNF_IGNORE_WEAK & flags) {
        for (int i = 0; i < job->count; i += 2) {
            job->elements[i] &= ~SOLVER_WEAK;
        }
    }

    if (DNF_IGNORE_WEAK_DEPS & flags)
        solver_set_flag(solv, SOLVER_FLAG_IGNORE_RECOMMENDED, 1);

    if (DNF_ALLOW_DOWNGRADE & goal->actions)
        solver_set_flag(solv, SOLVER_FLAG_ALLOW_DOWNGRADE, 1);

    if (solver_solve(solv, job))
        return 1;
    // either allow solutions callback or installonlies, both at the same time
    // are not supported
    if (!user_cb && limit_installonly_packages(goal, solv, job)) {
        // allow erasing non-installonly packages that depend on a kernel about
        // to be erased
        allow_uninstall_all_but_protected(goal, job, DNF_ALLOW_UNINSTALL);
        if (solver_solve(solv, job))
            return 1;
    }
    goal->trans = solver_create_transaction(solv);

    if (protected_in_removals(goal))
        return 1;

    return 0;
}

static Queue *
construct_job(HyGoal goal, DnfGoalActions flags)
{
    DnfSack *sack = goal->sack;
    Queue *job = (Queue*)g_malloc(sizeof(*job));

    queue_init_clone(job, &goal->staging);

    /* apply forcebest */
    if (flags & DNF_FORCE_BEST)
        for (int i = 0; i < job->count; i += 2)
            job->elements[i] |= SOLVER_FORCEBEST;

    /* turn off implicit obsoletes for installonly packages */
    for (int i = 0; i < (int) dnf_sack_get_installonly(sack)->count; i++)
        queue_push2(job, SOLVER_MULTIVERSION|SOLVER_SOLVABLE_PROVIDES,
                    dnf_sack_get_installonly(sack)->elements[i]);

    allow_uninstall_all_but_protected(goal, job, flags);

    if (flags & DNF_VERIFY)
        queue_push2(job, SOLVER_VERIFY|SOLVER_SOLVABLE_ALL, 0);

    return job;
}

static void
free_job(Queue *job)
{
    queue_free(job);
    g_free(job);
}

static GPtrArray *
list_results(HyGoal goal, Id type_filter1, Id type_filter2, GError **error)
{
    Queue transpkgs;
    Transaction *trans = goal->trans;
    GPtrArray *plist;

    /* no transaction */
    if (trans == NULL) {
        if (goal->solv == NULL) {
            g_set_error_literal (error,
                                 DNF_ERROR,
                                 DNF_ERROR_INTERNAL_ERROR,
                                 _("no solv in the goal"));
            return NULL;
        } else if (goal->removal_of_protected->len) {
            g_set_error_literal (error,
                                 DNF_ERROR,
                                 DNF_ERROR_REMOVAL_OF_PROTECTED_PKG,
                                 _("no solution, cannot remove protected package"));
            return NULL;
        }
        g_set_error_literal (error,
                             DNF_ERROR,
                             DNF_ERROR_NO_SOLUTION,
                             _("no solution possible"));
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
            g_ptr_array_add(plist, dnf_package_new(goal->sack, p));
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
filter_arch2job(DnfSack *sack, const libdnf::Filter *f, Queue *job)
{
    if (f == nullptr)
        return 0;

    auto matches = f->getMatches();

    assert(f->getCmpType() == HY_EQ);
    assert(matches.size() == 1);
    Pool *pool = dnf_sack_get_pool(sack);
    const char *arch = matches[0].str;
    Id archid = str2archid(pool, arch);

    if (archid == 0)
        return DNF_ERROR_INVALID_ARCHITECTURE;
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
filter_evr2job(DnfSack *sack, const libdnf::Filter *f, Queue *job)
{
    if (f == nullptr)
        return 0;
    auto matches = f->getMatches();

    assert(f->getCmpType() == HY_EQ);
    assert(matches.size() == 1);

    Pool *pool = dnf_sack_get_pool(sack);
    Id evr = pool_str2id(pool, matches[0].str, 1);
    Id constr = f->getKeyname() == HY_PKG_VERSION ? SOLVER_SETEV : SOLVER_SETEVR;
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
filter_file2job(DnfSack *sack, const libdnf::Filter *f, Queue *job)
{
    if (f == nullptr)
        return 0;

    auto matches = f->getMatches();

    assert(matches.size() == 1);

    const char *file = matches[0].str;
    Pool *pool = dnf_sack_get_pool(sack);

    int flags = f->getCmpType() & HY_GLOB ? SELECTION_GLOB : 0;
    if (f->getCmpType() & HY_GLOB)
        flags |= SELECTION_NOCASE;
    if (selection_make(pool, job, file, flags | SELECTION_FILELIST) == 0)
        return 1;
    return 0;
}

static int
filter_pkg2job(DnfSack *sack, const libdnf::Filter *f, Queue *job)
{
    if (f == nullptr)
        return 0;
    assert(f->getMatches().size() == 1);
    Pool *pool = dnf_sack_get_pool(sack);
    DnfPackageSet *pset = f->getMatches()[0].pset;
    Id what;
    Id id = -1;
    Queue pkgs;
    queue_init(&pkgs);
    while(true) {
        id = pset->next(id);
        if (id == -1)
            break;
        queue_push(&pkgs, id);
    }
    what = pool_queuetowhatprovides(pool, &pkgs);
    queue_push2(job, SOLVER_SOLVABLE_ONE_OF|SOLVER_SETARCH|SOLVER_SETEVR, what);
    queue_free(&pkgs);
    return 0;
}

static int
filter_name2job(DnfSack *sack, const libdnf::Filter *f, Queue *job)
{
    if (f == nullptr)
        return 0;
    assert(f->getMatches().size() == 1);

    Pool *pool = dnf_sack_get_pool(sack);
    const char *name = f->getMatches()[0].str;
    Id id;
    Dataiterator di;

    switch (f->getCmpType()) {
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
filter_provides2job(DnfSack *sack, const libdnf::Filter *f, Queue *job)
{
    if (f == nullptr)
        return 0;
    auto matches = f->getMatches();
    assert(matches.size() == 1);
    const char *name;
    Pool *pool = dnf_sack_get_pool(sack);
    Id id;
    Dataiterator di;

    switch (f->getCmpType()) {
        case HY_EQ:
            id = dnf_reldep_get_id(matches[0].reldep);
            queue_push2(job, SOLVER_SOLVABLE_PROVIDES, id);
            break;
        case HY_GLOB:
            name = matches[0].str;
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
filter_reponame2job(DnfSack *sack, const libdnf::Filter *f, Queue *job)
{
    Queue repo_sel;
    Id i;
    Repo *repo;

    if (f == nullptr)
        return 0;
    auto matches = f->getMatches();

    assert(f->getCmpType() == HY_EQ);
    assert(matches.size() == 1);

    queue_init(&repo_sel);
    Pool *pool = dnf_sack_get_pool(sack);
    FOR_REPOS(i, repo)
        if (!strcmp(matches[0].str, repo->name)) {
            queue_push2(&repo_sel, SOLVER_SOLVABLE_REPO | SOLVER_SETREPO, repo->repoid);
        }

    selection_filter(pool, job, &repo_sel);

    queue_free(&repo_sel);
    return 0;
}

/**
 * Build job queue from a Query.
 *
 * Returns an error code
 */
int
sltr2job(const HySelector sltr, Queue *job, int solver_action)
{
    DnfSack *sack = selector_sack(sltr);
    int ret = 0;
    Queue job_sltr;
    int any_opt_filter = sltr->getFilterArch() || sltr->getFilterEvr()
        || sltr->getFilterReponame();
    int any_req_filter = sltr->getFilterName() || sltr->getFilterProvides()
        || sltr->getFilterFile() || sltr->getFilterPkg();

    queue_init(&job_sltr);

    if (!any_req_filter) {
        if (any_opt_filter)
            // no name or provides or file in the selector is an error
            ret = DNF_ERROR_BAD_SELECTOR;
        goto finish;
    }

    dnf_sack_recompute_considered(sack);
    dnf_sack_make_provides_ready(sack);
    ret = filter_pkg2job(sack, sltr->getFilterPkg(), &job_sltr);
    if (ret)
        goto finish;
    ret = filter_name2job(sack, sltr->getFilterName(), &job_sltr);
    if (ret)
        goto finish;
    ret = filter_file2job(sack, sltr->getFilterFile(), &job_sltr);
    if (ret)
        goto finish;
    ret = filter_provides2job(sack, sltr->getFilterProvides(), &job_sltr);
    if (ret)
        goto finish;
    ret = filter_arch2job(sack, sltr->getFilterArch(), &job_sltr);
    if (ret)
        goto finish;
    ret = filter_evr2job(sack, sltr->getFilterEvr(), &job_sltr);
    if (ret)
        goto finish;
    ret = filter_reponame2job(sack, sltr->getFilterReponame(), &job_sltr);
    if (ret)
        goto finish;

    for (int i = 0; i < job_sltr.count; i += 2)
         queue_push2(job,
                     job_sltr.elements[i] | solver_action,
                     job_sltr.elements[i + 1]);

 finish:
    queue_free(&job_sltr);
    return ret;
}

// public functions

HyGoal
hy_goal_clone(HyGoal goal)
{
    HyGoal gn = hy_goal_create(goal->sack);
    queue_init_clone(&gn->staging, &goal->staging);
    gn->protected_pkgs = (Map*)g_malloc0(sizeof(Map));
    if (goal->protected_pkgs != NULL)
        map_init_clone(gn->protected_pkgs, goal->protected_pkgs);
    gn->actions = goal->actions;
    g_ptr_array_unref(gn->removal_of_protected);
    gn->removal_of_protected = g_ptr_array_ref(goal->removal_of_protected);
    return gn;
}

HyGoal
hy_goal_create(DnfSack *sack)
{
    HyGoal goal = (HyGoal)g_malloc0(sizeof(*goal));
    goal->sack = sack;
    goal->removal_of_protected = g_ptr_array_new();
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
    free_map_fully(goal->protected_pkgs);
    g_ptr_array_unref(goal->removal_of_protected);
    g_free(goal);
}

DnfSack *
hy_goal_get_sack(HyGoal goal)
{
    return goal->sack;
}

static void
package2job(DnfPackage *package, Queue *job, int solver_action)
{
    Queue pkgs;
    queue_init(&pkgs);

    Pool *pool = dnf_package_get_pool(package);
    DnfSack *sack = dnf_package_get_sack(package);

    dnf_sack_recompute_considered(sack);
    dnf_sack_make_provides_ready(sack);

    queue_push(&pkgs, dnf_package_get_id(package));

    Id what = pool_queuetowhatprovides(pool, &pkgs);
    queue_push2(job, SOLVER_SOLVABLE_ONE_OF|SOLVER_SETARCH|SOLVER_SETEVR|solver_action,
                what);
    queue_free(&pkgs);
}

int
hy_goal_distupgrade_all(HyGoal goal)
{
    goal->actions |= DNF_DISTUPGRADE_ALL;
    queue_push2(&goal->staging, SOLVER_DISTUPGRADE|SOLVER_SOLVABLE_ALL, 0);
    return 0;
}

int
hy_goal_distupgrade(HyGoal goal, DnfPackage *new_pkg)
{
    goal->actions |= DNF_DISTUPGRADE;
    package2job(new_pkg, &goal->staging, SOLVER_DISTUPGRADE);
    return 0;
}

int
hy_goal_distupgrade_selector(HyGoal goal, HySelector sltr)
{
    goal->actions |= DNF_DISTUPGRADE;
    return sltr2job(sltr, &goal->staging, SOLVER_DISTUPGRADE);
}

int
hy_goal_downgrade_to(HyGoal goal, DnfPackage *new_pkg)
{
    goal->actions |= DNF_DOWNGRADE|DNF_ALLOW_DOWNGRADE;
    package2job(new_pkg, &goal->staging, SOLVER_INSTALL);
    return 0;
}

int
hy_goal_erase(HyGoal goal, DnfPackage *pkg)
{
    goal->actions |= DNF_ERASE;
    return hy_goal_erase_flags(goal, pkg, 0);
}

int
hy_goal_erase_flags(HyGoal goal, DnfPackage *pkg, int flags)
{
#ifndef NDEBUG
    Pool *pool = dnf_sack_get_pool(goal->sack);
    assert(pool->installed &&
           pool_id2solvable(pool, dnf_package_get_id(pkg))->repo == pool->installed);
#endif
    int additional = erase_flags2libsolv(flags);
    goal->actions |= DNF_ERASE;
    queue_push2(&goal->staging, SOLVER_SOLVABLE|SOLVER_ERASE|additional,
                dnf_package_get_id(pkg));
    return 0;
}

int
hy_goal_erase_selector_flags(HyGoal goal, HySelector sltr, int flags)
{
    int additional = erase_flags2libsolv(flags);
    goal->actions |= DNF_ERASE;
    return sltr2job(sltr, &goal->staging, SOLVER_ERASE|additional);
}

int
hy_goal_has_actions(HyGoal goal, DnfGoalActions action)
{
    return goal->actions & action;
}

int
hy_goal_install(HyGoal goal, DnfPackage *new_pkg)
{
    goal->actions |= DNF_INSTALL|DNF_ALLOW_DOWNGRADE;
    package2job(new_pkg, &goal->staging, SOLVER_INSTALL);
    return 0;
}

int
hy_goal_install_optional(HyGoal goal, DnfPackage *new_pkg)
{
    goal->actions |= DNF_INSTALL|DNF_ALLOW_DOWNGRADE;
    package2job(new_pkg, &goal->staging, SOLVER_INSTALL|SOLVER_WEAK);
    return 0;
}

gboolean
hy_goal_install_selector(HyGoal goal, HySelector sltr, GError **error)
{
    int rc;
    goal->actions |= DNF_INSTALL|DNF_ALLOW_DOWNGRADE;
    rc = sltr2job(sltr, &goal->staging, SOLVER_INSTALL);
    if (rc != 0) {
        g_set_error_literal (error,
                             DNF_ERROR,
                             rc,
                             _("failed to install selector"));
        return FALSE;
    }
    return TRUE;
}

gboolean
hy_goal_install_selector_optional(HyGoal goal, HySelector sltr, GError **error)
{
    int rc;
    goal->actions |= DNF_INSTALL|DNF_ALLOW_DOWNGRADE;
    rc = sltr2job(sltr, &goal->staging, SOLVER_INSTALL|SOLVER_WEAK);
    if (rc != 0) {
        g_set_error_literal (error,
                             DNF_ERROR,
                             rc,
                             _("failed to install optional selector"));
        return FALSE;
    }
    return TRUE;
}

int
hy_goal_upgrade_all(HyGoal goal)
{
    goal->actions |= DNF_UPGRADE_ALL;
    queue_push2(&goal->staging, SOLVER_UPDATE|SOLVER_SOLVABLE_ALL, 0);
    return 0;
}

int
hy_goal_upgrade_to(HyGoal goal, DnfPackage *new_pkg)
{
    goal->actions |= DNF_UPGRADE;
    package2job(new_pkg, &goal->staging, SOLVER_UPDATE);
    return 0;
}

int
hy_goal_upgrade_selector(HyGoal goal, HySelector sltr)
{
    goal->actions |= DNF_UPGRADE;
    return sltr2job(sltr, &goal->staging, SOLVER_UPDATE);
}

int
hy_goal_userinstalled(HyGoal goal, DnfPackage *pkg)
{
    queue_push2(&goal->staging, SOLVER_SOLVABLE|SOLVER_USERINSTALLED,
                dnf_package_get_id(pkg));
    return 0;
}

int hy_goal_req_length(HyGoal goal)
{
    return goal->staging.count / 2;
}

int
hy_goal_run_flags(HyGoal goal, DnfGoalActions flags)
{
    return hy_goal_run_all_flags(goal, NULL, NULL, flags);
}

int
hy_goal_run_all_flags(HyGoal goal, hy_solution_callback cb, void *cb_data,
                      DnfGoalActions flags)
{
    Queue *job = construct_job(goal, flags);
    goal->actions |= flags;
    int ret = solve(goal, job, flags, cb, cb_data);
    free_job(job);
    return ret;
}

int
hy_goal_count_problems(HyGoal goal)
{
    assert(goal->solv);
    return solver_problem_count(goal->solv) + MIN(1, goal->removal_of_protected->len);
}

static DnfPackageSet *
remove_pkgs_with_same_nevra_from_pset(DnfPackageSet* pset, DnfPackageSet* remove_musters, DnfSack* sack)
{
    DnfPackageSet *final_pset = dnf_packageset_new(sack);
    Id id1 = -1;
    while(true) {
        id1 = pset->next(id1);
        if (id1 == -1)
            break;
        DnfPackage *pkg1 = dnf_package_new(sack, id1);
        Id id2 = -1;
        gboolean found = FALSE;
        while(true) {
            id2 = remove_musters->next(id2);
            if (id2 == -1)
                break;
            DnfPackage *pkg2 = dnf_package_new(sack, id2);
            if (!dnf_package_cmp(pkg1, pkg2)) {
                found = TRUE;
                break;
            }
        }
        if (!found)
            dnf_packageset_add(final_pset, pkg1);
    }
    return final_pset;
}

/**
 * Reports packages that has a conflict
 *
 * Returns Queue with Ids of packages with conflict
 */
static Queue *
hy_goal_conflict_pkgs(HyGoal goal, unsigned i)
{
    SolverRuleinfo type;
    Id rid, source, target, dep;

    Queue pq;
    Queue* conflict = (Queue*)g_malloc(sizeof(Queue));
    int j;

    queue_init(conflict);

    if (i >= solver_problem_count(goal->solv))
        return conflict;

    queue_init(&pq);
    // this libsolv interface indexes from 1 (we do from 0), so:
    solver_findallproblemrules(goal->solv, i+1, &pq);
    for (j = 0; j < pq.count; j++) {
        rid = pq.elements[j];
        type = solver_ruleinfo(goal->solv, rid, &source, &target, &dep);
        if (type == SOLVER_RULE_PKG_CONFLICTS)
            queue_push2(conflict, source, target);
        else if (type == SOLVER_RULE_PKG_SELF_CONFLICT)
            queue_push(conflict, source);
        else if (type == SOLVER_RULE_PKG_SAME_NAME)
            queue_push2(conflict, source, target);
    }
    queue_free(&pq);
    return conflict;
}

/**
 * Reports packages that has a conflict
 *
 * available - if available it returns set with available packages with conflicts
 * available - if package installed it also excludes available packages with same NEVRA
 *
 * Returns DnfPackageSet with all packages that have a conflict.
 */
DnfPackageSet *
hy_goal_conflict_all_pkgs(HyGoal goal, DnfPackageState pkg_type)
{
    DnfPackageSet *pset = dnf_packageset_new(goal->sack);
    DnfPackageSet *temporary_pset = dnf_packageset_new(goal->sack);

    int count_problems = hy_goal_count_problems(goal);
    for (int i = 0; i < count_problems; i++) {
        Queue *conflict = hy_goal_conflict_pkgs(goal, i);
        for (int j = 0; j < conflict->count; j++) {
            Id id = conflict->elements[j];
            DnfPackage *pkg = dnf_package_new(goal->sack, id);
            if (pkg == NULL)
                continue;
            if (pkg_type ==  DNF_PACKAGE_STATE_AVAILABLE && dnf_package_installed(pkg)) {
                dnf_packageset_add(temporary_pset, pkg);
                continue;
            }
            if (pkg_type ==  DNF_PACKAGE_STATE_INSTALLED && !dnf_package_installed(pkg))
                continue;
            dnf_packageset_add(pset, pkg);
        }
        queue_free(conflict);
    }
    unsigned int count = dnf_packageset_count(temporary_pset);
    if (!count) {
        delete temporary_pset;
        return pset;
    }

    DnfPackageSet *final_pset = remove_pkgs_with_same_nevra_from_pset(pset, temporary_pset,
                                                                      goal->sack);
    delete pset;
    delete temporary_pset;
    return final_pset;
}
/**
 * Reports packages that have broken dependency
 *
 * Returns Queue with Ids of packages with broken dependency
 */
static Queue *
hy_goal_broken_dependency_pkgs(HyGoal goal, unsigned i)
{
    SolverRuleinfo type;
    Id rid, source, target, dep;

    Queue pq;
    Queue* broken_dependency = (Queue*)g_malloc(sizeof(Queue));
    int j;
    queue_init(broken_dependency);
    if (i >= solver_problem_count(goal->solv))
        return broken_dependency;
    queue_init(&pq);
    // this libsolv interface indexes from 1 (we do from 0), so:
    solver_findallproblemrules(goal->solv, i+1, &pq);
    for (j = 0; j < pq.count; j++) {
        rid = pq.elements[j];
        type = solver_ruleinfo(goal->solv, rid, &source, &target, &dep);
        if (type == SOLVER_RULE_PKG_NOTHING_PROVIDES_DEP)
            queue_push(broken_dependency, source);
        else if (type == SOLVER_RULE_PKG_REQUIRES)
            queue_push(broken_dependency, source);
    }
    queue_free(&pq);
    return broken_dependency;
}

/**
 * Reports all packages that have broken dependency
 * available - if available returns only available packages with broken dependencies
 * available - if package installed it also excludes available packages with same NEVRA
 * Returns DnfPackageSet with all packages with broken dependency
 */
DnfPackageSet *
hy_goal_broken_dependency_all_pkgs(HyGoal goal, DnfPackageState pkg_type)
{
    DnfPackageSet *pset = dnf_packageset_new(goal->sack);
    DnfPackageSet *temporary_pset = dnf_packageset_new(goal->sack);

    int count_problems = hy_goal_count_problems(goal);
    for (int i = 0; i < count_problems; i++) {
        Queue *broken_dependency = hy_goal_broken_dependency_pkgs(goal, i);
        for (int j = 0; j < broken_dependency->count; j++) {
            Id id = broken_dependency->elements[j];
            DnfPackage *pkg = dnf_package_new(goal->sack, id);
            if (pkg == NULL)
                continue;
            if (pkg_type ==  DNF_PACKAGE_STATE_AVAILABLE && dnf_package_installed(pkg)) {
                dnf_packageset_add(temporary_pset, pkg);
                continue;
            }
            if (pkg_type ==  DNF_PACKAGE_STATE_INSTALLED && !dnf_package_installed(pkg))
                continue;
            dnf_packageset_add(pset, pkg);
        }
        queue_free(broken_dependency);
    }
    unsigned int count = dnf_packageset_count(temporary_pset);
    if (!count) {
        delete temporary_pset;
        return pset;
    }
    DnfPackageSet *final_pset = remove_pkgs_with_same_nevra_from_pset(pset, temporary_pset,
                                                                      goal->sack);
    delete pset;
    delete temporary_pset;
    return final_pset;
}

/**
 * String describing the removal of protected packages.
 *
 * Caller is responsible for freeing the returned string using g_free().
 */
static char *
hy_goal_describe_protected_removal(HyGoal goal)
{
    g_autoptr(GString) string = NULL;
    DnfPackage *pkg;
    guint j;
    const char *name;
    DnfPackageSet *pset;
    Pool *pool = goal->solv->pool;
    Solvable *s;

    string = g_string_new(_("The operation would result in removing"
                            " the following protected packages: "));

    if ((goal->removal_of_protected != NULL) && (0 < goal->removal_of_protected->len)) {
        for (j = 0; j < goal->removal_of_protected->len; j++) {
            pkg = (DnfPackage*)g_ptr_array_index(goal->removal_of_protected, j);
            name = dnf_package_get_name(pkg);
            if (j == 0) {
                g_string_append(string, name);
            } else {
                g_string_append_printf(string, ", %s", name);
            }
        }
        return g_strdup(string->str);
    }
    pset = hy_goal_broken_dependency_all_pkgs(goal, DNF_PACKAGE_STATE_INSTALLED);
    Id id = -1;
    gboolean found = FALSE;
    while(true) {
        id = pset->next(id);
        if (id == -1)
            break;
        if (MAPTST(goal->protected_pkgs, id)) {
            s = pool_id2solvable(pool, id);
            name = pool_id2str(pool, s->name);
            if (!found) {
                g_string_append(string, name);
                found = TRUE;
            } else {
                g_string_append_printf(string, ", %s", name);
            }
        }
    }
    if (found)
        return g_strdup(string->str);
    return NULL;
}

char **
hy_goal_describe_problem_rules(HyGoal goal, unsigned i)
{
    char **problist = NULL;
    int p = 0;
    /* internal error */
    if (i >= (unsigned) hy_goal_count_problems(goal))
        return NULL;
    // problem is not in libsolv - removal of protected packages
    char *problem = hy_goal_describe_protected_removal(goal);
    if (problem) {
        problist = (char**)solv_extend(problist, p, 1, sizeof(char*), BLOCK_SIZE);
        problist[p++] = problem;
        problist = (char**)solv_extend(problist, p, 1, sizeof(char*), BLOCK_SIZE);
        problist[p++] = NULL;
        return problist;
    }

    Queue pq;
    Id rid, source, target, dep;
    SolverRuleinfo type;
    int j;
    gboolean unique;

    if (i >= solver_problem_count(goal->solv))
        return problist;

    queue_init(&pq);
    // this libsolv interface indexes from 1 (we do from 0), so:
    solver_findallproblemrules(goal->solv, i+1, &pq);
    for (j = 0; j < pq.count; j++) {
        rid = pq.elements[j];
        type = solver_ruleinfo(goal->solv, rid, &source, &target, &dep);
        const char *problem_str = solver_problemruleinfo2str(goal->solv, type, source, target, dep);
        unique = TRUE;
        if (problist != NULL) {
            for (int k = 0; problist[k] != NULL; k++) {
                if (g_strcmp0(problem_str, problist[k]) == 0) {
                    unique = FALSE;
                    break;
                }
            }
        }
        if (unique) {
            if (problist == NULL)
                problist = (char**)solv_extend(problist, p, 1, sizeof(char*), BLOCK_SIZE);
            problist[p++] = g_strdup(problem_str);
            problist = (char**)solv_extend(problist, p, 1, sizeof(char*), BLOCK_SIZE);
            problist[p] = NULL;
        }
    }
    queue_free(&pq);
    return problist;
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

/**
 * hy_goal_write_debugdata:
 * @goal: A #HyGoal
 * @dir: The directory to write to
 * @error: A #GError, or %NULL
 *
 * Writes details about the testcase to a directory.
 *
 * Returns: %FALSE if an error was set
 *
 * Since: 0.7.0
 */
gboolean
hy_goal_write_debugdata(HyGoal goal, const char *dir, GError **error)
{
    Solver *solv = goal->solv;
    if (solv == NULL) {
        g_set_error_literal (error,
                             DNF_ERROR,
                             DNF_ERROR_INTERNAL_ERROR,
                             _("no solver set"));
        return FALSE;
    }

    int flags = TESTCASE_RESULT_TRANSACTION | TESTCASE_RESULT_PROBLEMS;
    g_autofree char *absdir = abspath(dir);
    if (absdir == NULL) {
        g_set_error (error,
                     DNF_ERROR,
                     DNF_ERROR_FILE_INVALID,
                     _("failed to make %s absolute"), dir);
        return FALSE;
    }
    g_debug("writing solver debugdata to %s", absdir);
    int ret = testcase_write(solv, absdir, flags, NULL, NULL);
    if (!ret) {
        g_set_error (error,
                     DNF_ERROR,
                     DNF_ERROR_FILE_INVALID,
                     _("failed writing debugdata to %1$s: %2$s"),
                     absdir, strerror(errno));
        return FALSE;
    }
    return TRUE;
}

GPtrArray *
hy_goal_list_erasures(HyGoal goal, GError **error)
{
    return list_results(goal, SOLVER_TRANSACTION_ERASE, 0, error);
}

GPtrArray *
hy_goal_list_installs(HyGoal goal, GError **error)
{
    return list_results(goal, SOLVER_TRANSACTION_INSTALL,
                        SOLVER_TRANSACTION_OBSOLETES, error);
}

GPtrArray *
hy_goal_list_obsoleted(HyGoal goal, GError **error)
{
    return list_results(goal, SOLVER_TRANSACTION_OBSOLETED, 0, error);
}

GPtrArray *
hy_goal_list_reinstalls(HyGoal goal, GError **error)
{
    return list_results(goal, SOLVER_TRANSACTION_REINSTALL, 0, error);
}

GPtrArray *
hy_goal_list_unneeded(HyGoal goal, GError **error)
{
    GPtrArray *plist = hy_packagelist_create();
    Queue q;
    Solver *solv = goal->solv;

    queue_init(&q);
    solver_get_unneeded(solv, &q, 0);
    queue2plist(goal->sack, &q, plist);
    queue_free(&q);
    return plist;
}

GPtrArray *
hy_goal_list_upgrades(HyGoal goal, GError **error)
{
    return list_results(goal, SOLVER_TRANSACTION_UPGRADE, 0, error);
}

GPtrArray *
hy_goal_list_downgrades(HyGoal goal, GError **error)
{
    return list_results(goal, SOLVER_TRANSACTION_DOWNGRADE, 0, error);
}

GPtrArray *
hy_goal_list_obsoleted_by_package(HyGoal goal, DnfPackage *pkg)
{
    DnfSack *sack = goal->sack;
    Transaction *trans = goal->trans;
    Queue obsoletes;
    GPtrArray *plist = hy_packagelist_create();

    assert(trans);
    queue_init(&obsoletes);

    transaction_all_obs_pkgs(trans, dnf_package_get_id(pkg), &obsoletes);
    queue2plist(sack, &obsoletes, plist);

    queue_free(&obsoletes);
    return plist;
}

int
hy_goal_get_reason(HyGoal goal, DnfPackage *pkg)
{
    //solver_get_recommendations
    if (goal->solv == NULL)
        return HY_REASON_USER;
    Id info;
    int reason = solver_describe_decision(goal->solv, dnf_package_get_id(pkg), &info);

    if ((reason == SOLVER_REASON_UNIT_RULE ||
         reason == SOLVER_REASON_RESOLVE_JOB) &&
        (solver_ruleclass(goal->solv, info) == SOLVER_RULE_JOB ||
         solver_ruleclass(goal->solv, info) == SOLVER_RULE_BEST))
        return HY_REASON_USER;
    if (reason == SOLVER_REASON_CLEANDEPS_ERASE)
        return HY_REASON_CLEAN;
    if (reason == SOLVER_REASON_WEAKDEP)
        return HY_REASON_WEAKDEP;
    return HY_REASON_DEP;
}
