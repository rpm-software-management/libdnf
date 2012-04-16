#define _GNU_SOURCE
#include <assert.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

// libsolv
#include "solv/chksum.h"
#include "solv/evr.h"
#include "solv/solver.h"
#include "solv/solverdebug.h"

// hawkey
#include "iutil.h"
#include "package_internal.h"
#include "sack_internal.h"

#define CHKSUM_TYPE REPOKEY_TYPE_SHA256
#define CHKSUM_IDENT "H000"
#define CACHEDIR_PERMISSIONS 0700

int
checksum_cmp(const unsigned char *cs1, const unsigned char *cs2)
{
    return memcmp(cs1, cs2, CHKSUM_BYTES);
}

/* calls rewind(fp) before returning */
int
checksum_fp(unsigned char *out, FILE *fp)
{
    /* based on calc_checksum_fp in libsolv's solv.c */
    char buf[4096];
    void *h = solv_chksum_create(CHKSUM_TYPE);
    int l;

    rewind(fp);
    solv_chksum_add(h, CHKSUM_IDENT, strlen(CHKSUM_IDENT));
    while ((l = fread(buf, 1, sizeof(buf), fp)) > 0)
	solv_chksum_add(h, buf, l);
    rewind(fp);
    solv_chksum_free(h, out);
    return 0;
}

/* calls rewind(fp) before returning */
int
checksum_read(unsigned char *csout, FILE *fp)
{
    if (fseek(fp, -32, SEEK_END) ||
	fread(csout, CHKSUM_BYTES, 1, fp) != 1)
	return 1;
    rewind(fp);
    return 0;
}

/* does not move the fp position */
int
checksum_stat(unsigned char *out, FILE *fp)
{
    assert(fp);

    struct stat stat;
    if (fstat(fileno(fp), &stat))
	return 1;

    /* based on calc_checksum_stat in libsolv's solv.c */
    void *h = solv_chksum_create(CHKSUM_TYPE);
    solv_chksum_add(h, CHKSUM_IDENT, strlen(CHKSUM_IDENT));
    solv_chksum_add(h, &stat.st_dev, sizeof(stat.st_dev));
    solv_chksum_add(h, &stat.st_ino, sizeof(stat.st_ino));
    solv_chksum_add(h, &stat.st_size, sizeof(stat.st_size));
    solv_chksum_add(h, &stat.st_mtime, sizeof(stat.st_mtime));
    solv_chksum_free(h, out);
    return 0;
}

/* moves fp to the end of file */
int checksum_write(const unsigned char *cs, FILE *fp)
{
    if (fseek(fp, 0, SEEK_END) ||
	fwrite(cs, CHKSUM_BYTES, 1, fp) != 1)
	return 1;
    return 0;
}

void
checksum_dump(const unsigned char *cs)
{
    for (int i = 0; i < CHKSUM_BYTES; i+=4) {
	printf("%02x%02x%02x%02x", cs[i], cs[i+1], cs[i+2], cs[i+3]);
	if (i + 4 >= CHKSUM_BYTES)
	    printf("\n");
	else
	    printf(" : ");
    }
}

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

int
mkcachedir(const char *path)
{
    int ret = 1;
    char *p = solv_strdup(path);
    int len = strlen(p);

    if (p[len-1] == '/')
	p[len-1] = '\0';

    if (access(p, X_OK)) {
	*(strrchr(p, '/')) = '\0';
	ret = mkcachedir(p);
	ret |= mkdir(path, CACHEDIR_PERMISSIONS);
    } else {
	ret = 0;
    }

    solv_free(p);
    return ret;
}

char *this_username(void)
{
    const struct passwd *pw = getpwuid(getuid());
    return solv_strdup(pw->pw_name);
}

void
repo_internalize_trigger(Repo *repo)
{
    if (strcmp(repo->name, HY_CMDLINE_REPO_NAME))
	return; /* this is only done for the cmdline repo, the ordinary ones get
		   internalized immediately */
    repo_internalize(repo);
}

Transaction *
job2transaction(HySack sack, Queue *job, Queue *errors)
{
    Pool *pool = sack_pool(sack);
    Solver *solv;
    Transaction *trans = NULL;

    sack_make_provides_ready(sack);
    solv = solver_create(pool);

    /* turn off implicit obsoletes for installonly packages */
    for (int i = 0; i < sack->installonly.count; i++)
	queue_push2(job, SOLVER_NOOBSOLETES|SOLVER_SOLVABLE_PROVIDES,
		    sack->installonly.elements[i]);

    /* installonly notwithstanding, process explicit obsoletes */
    solver_set_flag(solv, SOLVER_FLAG_KEEP_EXPLICIT_OBSOLETES, 1);

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
queue2plist(HySack sack, Queue *q, HyPackageList plist)
{
    Solvable *s;
    int i;

    for (i = 0; i < q->count; ++i) {
	s = pool_id2solvable(sack_pool(sack), q->elements[i]);
	hy_packagelist_push(plist, package_from_solvable(s));
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
