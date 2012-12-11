#include <assert.h>
#include <stdio.h>

// libsolv
#include <solv/solver.h>
#include <solv/solverdebug.h>
#include <solv/transaction.h>
#include <solv/util.h>

// hawkey
#include "errno.h"
#include "goal.h"
#include "iutil.h"
#include "package_internal.h"
#include "query_internal.h"
#include "sack_internal.h"
#include "selector_internal.h"

struct _HyGoal {
    HySack sack;
    Queue job;
    Solver *solv;
    Transaction *trans;
};

struct _SolutionCallback {
    HyGoal goal;
    hy_solution_callback callback;
    void *callback_data;
};

static int
erase_flags2libsolv(int flags)
{
    int ret = 0;
    if (flags & HY_CLEAN_DEPS)
	ret |= SOLVER_CLEANDEPS;
    return ret;
}

static int
solve(HyGoal goal, int flags)
{
    HySack sack = goal->sack;
    Solver *solv = goal->solv;

    sack_make_provides_ready(sack);
    if (solver_solve(solv, &goal->job))
	return 1;
    goal->trans = solver_create_transaction(solv);
    return 0;
}

static Solver *
construct_solver(HyGoal goal, int flags)
{
    HySack sack = goal->sack;
    Pool *pool = sack_pool(sack);
    Solver *solv = solver_create(pool);

    assert(goal->solv == NULL);

    if (flags & HY_ALLOW_UNINSTALL)
	solver_set_flag(solv, SOLVER_FLAG_ALLOW_UNINSTALL, 1);

    /* turn off implicit obsoletes for installonly packages */
    for (int i = 0; i < sack->installonly.count; i++)
	queue_push2(&goal->job, SOLVER_NOOBSOLETES|SOLVER_SOLVABLE_PROVIDES,
		    sack->installonly.elements[i]);

    /* installonly notwithstanding, process explicit obsoletes */
    solver_set_flag(solv, SOLVER_FLAG_KEEP_EXPLICIT_OBSOLETES, 1);
    goal->solv = solv;
    return solv;
}

static HyPackageList
list_results(HyGoal goal, Id type_filter)
{
    Pool *pool = sack_pool(goal->sack);
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

    for (int i = 0; i < trans->steps.count; ++i) {
	Id p = trans->steps.elements[i];
	Id type = transaction_type(trans, p, SOLVER_TRANSACTION_SHOW_ACTIVE);

	if (type == type_filter)
	    hy_packagelist_push(plist, package_create(pool, p));
    }
    return plist;
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
	assert(job->elements[i] & SOLVER_SOLVABLE_NAME);
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
    for (int i = 0; i < job->count; i += 2) {
	Id dep;
	assert(job->elements[i] & SOLVER_SOLVABLE_NAME);
	dep = pool_rel2id(pool, job->elements[i + 1],
			  evr, REL_EQ, 1);
	job->elements[i] |= SOLVER_SETEVR;
	job->elements[i + 1] = dep;
    }
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
    const char *provide = f->matches[0].str;
    Id id;

    switch (f->cmp_type) {
    case HY_EQ:
	id = pool_str2id(pool, provide, 0);
	if (id)
	    queue_push2(job, SOLVER_SOLVABLE_PROVIDES, id);
	break;
    default:
	assert(0);
	return 1;
    }
    return 0;
}

/**
 * Build job queue from a Query.
 *
 * Returns 0 on success. Otherwise it returns non-zero and sets hy_errno.
 */
static int
sltr2job(const HySelector sltr, Queue *job, int solver_action)
{
    HySack sack = selector_sack(sltr);
    int ret = 0;
    Queue job_sltr;

    queue_init(&job_sltr);
    if (sltr->f_name == NULL && sltr->f_provides == NULL) {
	// no name or provides in the selector is an error
	ret = HY_E_SELECTOR;
	goto finish;
    }

    sack_make_provides_ready(sack);
    ret = filter_name2job(sack, sltr->f_name, &job_sltr);
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
hy_goal_create(HySack sack)
{
    HyGoal goal = solv_calloc(1, sizeof(*goal));
    goal->sack = sack;
    queue_init(&goal->job);
    return goal;
}

void
hy_goal_free(HyGoal goal)
{
    if (goal->trans)
	transaction_free(goal->trans);
    if (goal->solv)
	solver_free(goal->solv);
    queue_free(&goal->job);
    solv_free(goal);
}

int
hy_goal_distupgrade_all(HyGoal goal)
{
    queue_push2(&goal->job, SOLVER_DISTUPGRADE|SOLVER_SOLVABLE_ALL, 0);
    return 0;
}

int
hy_goal_downgrade_to(HyGoal goal, HyPackage new_pkg)
{
    return hy_goal_install(goal, new_pkg);
}

int
hy_goal_erase(HyGoal goal, HyPackage pkg)
{
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
    queue_push2(&goal->job, SOLVER_SOLVABLE|SOLVER_ERASE|additional,
		package_id(pkg));
    return 0;
}

int
hy_goal_erase_selector(HyGoal goal, HySelector sltr)
{
    return hy_goal_erase_selector_flags(goal, sltr, 0);
}

int
hy_goal_erase_selector_flags(HyGoal goal, HySelector sltr, int flags)
{
    int additional = erase_flags2libsolv(flags);
    return sltr2job(sltr, &goal->job, SOLVER_ERASE|additional);
}

int
hy_goal_install(HyGoal goal, HyPackage new_pkg)
{
    queue_push2(&goal->job, SOLVER_SOLVABLE|SOLVER_INSTALL, package_id(new_pkg));
    return 0;
}

int
hy_goal_install_selector(HyGoal goal, HySelector sltr)
{
    return sltr2job(sltr, &goal->job, SOLVER_INSTALL);
}

int
hy_goal_upgrade_all(HyGoal goal)
{
    queue_push2(&goal->job, SOLVER_UPDATE|SOLVER_SOLVABLE_ALL, 0);
    return 0;
}

int
hy_goal_upgrade_to(HyGoal goal, HyPackage new_pkg)
{
    return hy_goal_upgrade_to_flags(goal, new_pkg, 0);
}

int
hy_goal_upgrade_to_selector(HyGoal goal, HySelector sltr)
{
    if (sltr->f_evr == NULL)
	return sltr2job(sltr, &goal->job, SOLVER_UPDATE);
    return sltr2job(sltr, &goal->job, SOLVER_INSTALL);
}

int
hy_goal_upgrade_selector(HyGoal goal, HySelector sltr)
{
    return sltr2job(sltr, &goal->job, SOLVER_UPDATE);
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

    return hy_goal_install(goal, new_pkg);
}

int
hy_goal_userinstalled(HyGoal goal, HyPackage pkg)
{
    queue_push2(&goal->job, SOLVER_SOLVABLE|SOLVER_USERINSTALLED,
		package_id(pkg));
    return 0;
}

int
hy_goal_run(HyGoal goal)
{
    return hy_goal_run_flags(goal, 0);
}

int
hy_goal_run_flags(HyGoal goal, int flags)
{
    construct_solver(goal, flags);
    return solve(goal, flags);
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
    struct _SolutionCallback cb_tuple = {goal, cb, cb_data};
    Solver *solv = construct_solver(goal, flags);

    solv->solution_callback = internal_solver_callback;
    solv->solution_callback_data = &cb_tuple;

    return solve(goal, flags);
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
hy_goal_list_reinstalls(HyGoal goal)
{
    return list_results(goal, SOLVER_TRANSACTION_REINSTALL);
}

HyPackageList
hy_goal_list_upgrades(HyGoal goal)
{
    return list_results(goal, SOLVER_TRANSACTION_UPGRADE);
}

HyPackageList
hy_goal_list_downgrades(HyGoal goal)
{
    return list_results(goal, SOLVER_TRANSACTION_DOWNGRADE);
}

/**
 * Return the package upgraded or downgraded by 'pkg'.
 *
 * The returned package has to be freed via hy_package_free().
 */
HyPackage
hy_goal_package_obsoletes(HyGoal goal, HyPackage pkg)
{
    Pool *pool = sack_pool(goal->sack);
    Transaction *trans = goal->trans;
    Id p;

    assert(trans);
    p = transaction_obs_pkg(trans, package_id(pkg));
    assert(p); // todo: handle no upgrades case
    return package_create(pool, p);
}

int
hy_goal_get_reason(HyGoal goal, HyPackage pkg)
{
    assert(goal->solv);
    Id info;
    int reason = solver_describe_decision(goal->solv, package_id(pkg), &info);

    if (reason == SOLVER_REASON_UNIT_RULE &&
	solver_ruleclass(goal->solv, info) == SOLVER_RULE_JOB)
	return HY_REASON_USER;
    return HY_REASON_DEP;
}
