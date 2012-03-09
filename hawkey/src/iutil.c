#include <assert.h>
#include <string.h>
#include <unistd.h>

// libsolv
#include "evr.h"
#include "solver.h"
#include "solverdebug.h"

// hawkey
#include "iutil.h"

int
is_readable_rpm(const char *fn)
{
    int len = strlen(fn);

    if (access(fn, R_OK))
	return 0;
    if (len <= 4 || strcmp(fn + len - 4, ".rpm"))
	return 0;

    return 1;
}

void
repo_internalize_trigger(Repo *repo)
{
    if (strcmp(repo->name, CMDLINE_REPO_NAME))
	return; /* this is only done for the cmdline repo, the ordinary ones get
		   internalized immediately */
    repo_internalize(repo);
}

Transaction *
job2transaction(Sack sack, Queue *job, Queue *errors)
{
    Pool *pool = sack->pool;
    Solver *solv;
    Transaction *trans = NULL;

    sack_make_provides_ready(sack);
    solv = solver_create(pool);
    solver_set_flag(solv, SOLVER_FLAG_IGNORE_ALREADY_RECOMMENDED, 1);
    if (solver_solve(solv, job)) {
	int i;
	Id rule, source, target, dep;
	SolverRuleinfo type;
	int problem_cnt = solver_problem_count(solv);

	assert(errors);
	for (i = 1; i <= problem_cnt; ++i) {
	    rule = solver_findproblemrule(solv, i);
	    type = solver_ruleinfo(solv, rule, &source, &target, &dep);
	    queue_push2(errors, type, source);
	    queue_push2(errors, target, dep);
	}
    } else
	trans = solver_create_transaction(solv);

    solver_free(solv);
    return trans;
}

void
queue2plist(Sack sack, Queue *q, PackageList plist)
{
    Solvable *s;
    int i;

    for (i = 0; i < q->count; ++i) {
	s = pool_id2solvable(sack->pool, q->elements[i]);
	packagelist_push(plist, package_from_solvable(s));
    }
}

char *
problemruleinfo2str(Pool *pool, SolverRuleinfo type, Id source, Id target, Id dep)
{
    /* also see solverdbug.c:solver_problemruleinfo2str(). This is a copy
       because that function needs Solver* instead of pool. It is probably not
       worth it to introduce pool_problemruleinfo2str() into libsolv API. */

  char *s;
  switch (type) {
  case SOLVER_RULE_DISTUPGRADE:
      s = pool_tmpjoin(pool, pool_solvid2str(pool, source), " does not belong to a distupgrade repository", 0);
      break;
  case SOLVER_RULE_INFARCH:
      s = pool_tmpjoin(pool, pool_solvid2str(pool, source), " has inferior architecture", 0);
      break;
  case SOLVER_RULE_UPDATE:
      s = pool_tmpjoin(pool, "problem with installed package ", pool_solvid2str(pool, source), 0);
      break;
  case SOLVER_RULE_JOB:
      s = "conflicting requests";
      break;
  case SOLVER_RULE_JOB_NOTHING_PROVIDES_DEP:
      s = pool_tmpjoin(pool, "nothing provides requested ", pool_dep2str(pool, dep), 0);
      break;
  case SOLVER_RULE_RPM:
      s = "some dependency problem";
      break;
  case SOLVER_RULE_RPM_NOT_INSTALLABLE:
      s = pool_tmpjoin(pool, "package ", pool_solvid2str(pool, source), " is not installable");
      break;
  case SOLVER_RULE_RPM_NOTHING_PROVIDES_DEP:
      s = pool_tmpjoin(pool, "nothing provides ", pool_dep2str(pool, dep), 0);
      s = pool_tmpappend(pool, s, " needed by ", pool_solvid2str(pool, source));
      break;
  case SOLVER_RULE_RPM_SAME_NAME:
      s = pool_tmpjoin(pool, "cannot install both ", pool_solvid2str(pool, source), 0);
      s = pool_tmpappend(pool, s, " and ", pool_solvid2str(pool, target));
      break;
  case SOLVER_RULE_RPM_PACKAGE_CONFLICT:
      s = pool_tmpjoin(pool, "package ", pool_solvid2str(pool, source), 0);
      s = pool_tmpappend(pool, s, " conflicts with ", pool_dep2str(pool, dep));
      s = pool_tmpappend(pool, s, " provided by ", pool_solvid2str(pool, target));
      break;
  case SOLVER_RULE_RPM_PACKAGE_OBSOLETES:
      s = pool_tmpjoin(pool, "package ", pool_solvid2str(pool, source), 0);
      s = pool_tmpappend(pool, s, " obsoletes ", pool_dep2str(pool, dep));
      s = pool_tmpappend(pool, s, " provided by ", pool_solvid2str(pool, target));
      break;
  case SOLVER_RULE_RPM_INSTALLEDPKG_OBSOLETES:
      s = pool_tmpjoin(pool, "installed package ", pool_solvid2str(pool, source), 0);
      s = pool_tmpappend(pool, s, " obsoletes ", pool_dep2str(pool, dep));
      s = pool_tmpappend(pool, s, " provided by ", pool_solvid2str(pool, target));
      break;
  case SOLVER_RULE_RPM_IMPLICIT_OBSOLETES:
      s = pool_tmpjoin(pool, "package ", pool_solvid2str(pool, source), 0);
      s = pool_tmpappend(pool, s, " implicitely obsoletes ", pool_dep2str(pool, dep));
      s = pool_tmpappend(pool, s, " provided by ", pool_solvid2str(pool, target));
      break;
  case SOLVER_RULE_RPM_PACKAGE_REQUIRES:
      s = pool_tmpjoin(pool, "package ", pool_solvid2str(pool, source), " requires ");
      s = pool_tmpappend(pool, s, pool_dep2str(pool, dep), ", but none of the providers can be installed");
      break;
  case SOLVER_RULE_RPM_SELF_CONFLICT:
      s = pool_tmpjoin(pool, "package ", pool_solvid2str(pool, source), " conflicts with ");
      s = pool_tmpappend(pool, s, pool_dep2str(pool, dep), " provided by itself");
      break;
  default:
      s = "bad problem rule type";
      break;
  }
  return solv_strdup(s);
}

Id
what_updates(Pool *pool, Id pkg)
{
    Id p, pp;
    Solvable *updated, *s = pool_id2solvable(pool, pkg);

    assert(pool->whatprovides);
    FOR_PROVIDES(p, pp, s->name) {
	updated = pool_id2solvable(pool, p);
	if (pool->installed && updated->repo == pool->installed &&
	    pool_evrcmp(pool, s->evr, updated->evr, EVRCMP_COMPARE) > 0)
	    return p;
    }
    return 0;
}
