#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

// libsolv
#include "solv/pool.h"
#include "solv/poolarch.h"
#include "solv/repo.h"
#include "solv/repo_repomdxml.h"
#include "solv/repo_rpmmd.h"
#include "solv/repo_rpmdb.h"
#include "solv/repo_solv.h"
#include "solv/repo_write.h"
#include "solv/solv_xfopen.h"
#include "solv/solver.h"
#include "solv/solverdebug.h"

// hawkey
#include "iutil.h"
#include "sack.h"

#define SOLVCACHE_PATH "/home/akozumpl/tmp/sapi"

static Repo *
get_cmdline_repo(Sack sack)
{
    Pool *pool = sack->pool;
    Repo *repo;
    int repoid;

    FOR_REPOS(repoid, repo) {
	if (!strcmp(repo->name, CMDLINE_REPO_NAME))
	    return repo;
    }
    return NULL;
}

static char *
solvcache_path(const char *reponame)
{
    char *fn, *fnt;

    fnt = solv_dupjoin(SOLVCACHE_PATH, "/", reponame);
    fn = solv_dupjoin(fnt, ".solv", NULL);
    free(fnt);

    return fn;
}

static Repo *
read_rpm_repo(Pool *pool)
{
    Repo *repo;
    char *fn;
    FILE *fp;

    repo = repo_create(pool, SYSTEM_REPO_NAME);

    fn = solvcache_path(SYSTEM_REPO_NAME);
    fp = fopen(fn, "r");
    free(fn);
    if (!fp || repo_add_solv(repo, fp)) {
	printf("fetching rpmdb\n");
	repo_add_rpmdb(repo, 0, 0, REPO_REUSE_REPODATA);
    } else {
	fclose(fp);
	printf("using cached rpmdb\n");
    }
    pool_set_installed(pool, repo);

    return repo;
}

static Repo *
read_yum_repo(Pool *pool, const char *name,
	      const char *md_repo_fn,
	      const char *md_primary_fn)
{
    Repo *repo = repo_create(pool, name);
    char *fn = solvcache_path(name);
    FILE *fp = fopen(fn, "r");

    free(fn);

    if (fp) {
	printf("using cached %s\n", name);
	if (repo_add_solv(repo, fp)) {
	    perror(__func__);
	    return NULL;
	}
	fclose(fp);
    } else {
	FILE *f_repo = fopen(md_repo_fn, "r");
	FILE *f_primary = solv_xfopen(md_primary_fn, "r");

	if (!(f_repo && f_primary)) {
	    perror(__func__);
	    return NULL;
	}

	printf("fetching %s\n", name);
	repo_add_repomdxml(repo, f_repo, 0);
	repo_add_rpmmd(repo, f_primary, 0, 0);

	fclose(f_repo);
	fclose(f_primary);
    }

    return repo;
}

static void
setarch(Pool *pool)
{
  struct utsname un;
  if (uname(&un))
    {
      perror(__func__);
      exit(1);
    }
  pool_setarch(pool, un.machine);
}

static void
transaction2map(Transaction *trans, Map *m)
{
    Queue classes, pkgs;
    int i;

    queue_init(&classes);
    queue_init(&pkgs);

    transaction_classify(trans, 0, &classes);
    for (i = 0; i < classes.count; i += 4) {
	Id class = classes.elements[i];

	switch (class) {
	case SOLVER_TRANSACTION_UPGRADED:
	    transaction_classify_pkgs(trans, 0, class,
				      classes.elements[i+2],
				      classes.elements[i+3], &pkgs);
	    break;
	default:
	    break;
	}
    }
    for (i = 0; i < pkgs.count; ++i) {
	Id p = transaction_obs_pkg(trans, pkgs.elements[i]);
	MAPSET(m, p);
    }
    queue_free(&classes);
    queue_free(&pkgs);
}

static void
transaction2obs_map(Transaction *trans, Map *m)
{
    Pool *pool = trans->pool;
    int i, j;
    Id p, type;
    Queue obsoleted;

    queue_init(&obsoleted);
    for (i = 0; i < trans->steps.count; i++) {
	p = trans->steps.elements[i];
	type = transaction_type(trans, p,
				SOLVER_TRANSACTION_SHOW_ACTIVE|
				SOLVER_TRANSACTION_SHOW_OBSOLETES|
				SOLVER_TRANSACTION_SHOW_MULTIINSTALL);
	if (type == SOLVER_TRANSACTION_OBSOLETES) {
	    printf("%s obsoletes\n", pool_solvid2str(pool, p));
	    transaction_all_obs_pkgs(trans, p, &obsoleted);
	    assert(obsoleted.count);
	    for (j = 0; j < obsoleted.count; ++j) {
		printf("\t%s\n", pool_solvid2str(pool, obsoleted.elements[j]));
	    }
	}
    }
    queue_free(&obsoleted);
}

Sack
sack_create(void)
{
    Sack sack = solv_calloc(1, sizeof(*sack));
    sack->pool = pool_create();
    setarch(sack->pool);
    return sack;
}

void
sack_free(Sack sack)
{
    pool_free(sack->pool);
    solv_free(sack);
}

/**
 * Creates repo for command line rpms.
 */
void
sack_create_cmdline_repo(Sack sack)
{
    repo_create(sack->pool, CMDLINE_REPO_NAME);
}

/**
 * Adds the given .rpm file to the command line repo.
 */
Package
sack_add_cmdline_rpm(Sack sack, const char *fn)
{
    Repo *repo = get_cmdline_repo(sack);
    Id p;

    assert(repo);
    if (!is_readable_rpm(fn)) {
	printf("not a readable RPM file: %s, skipping\n", fn);
	return NULL;
    }
    p = repo_add_rpm(repo, fn, REPO_REUSE_REPODATA|REPO_NO_INTERNALIZE);
    sack->provides_ready = 0;    /* triggers internalizing later */
    return package_create(sack->pool, p);
}

void
sack_load_rpm_repo(Sack sack)
{
    read_rpm_repo(sack->pool);
    sack->provides_ready = 0;
}

void sack_load_yum_repo(Sack sack, FRepo repo)
{
    read_yum_repo(sack->pool,
		  repo->name,
		  repo->repomd_fn,
		  repo->primary_fn);
    sack->provides_ready = 0;
}

void
sack_make_provides_ready(Sack sack)
{
    if (!sack->provides_ready) {
	pool_addfileprovides(sack->pool);
	pool_createwhatprovides(sack->pool);
	sack->provides_ready = 1;
    }
}

int
sack_write_all_repos(Sack sack)
{
    Pool *pool = sack->pool;
    Repo *repo;
    int i;

    FOR_REPOS(i, repo) {
	struct stat st;
	const char *name = repo->name;

	if (!strcmp(name, CMDLINE_REPO_NAME))
	    continue;

	char *fn = solvcache_path(name);
	if (!stat(fn, &st)) {
	    free(fn);
	    continue;
	}

	printf("caching repo: %s\n", repo->name);
	FILE *fp = fopen(fn, "w+x");
	free(fn);
	if (!fp) {
	    perror(__func__);
	    return 1;
	}
	repo_write(repo, fp, repo_write_stdkeyfilter, 0, 0);
	fclose(fp);
    }
    return 0;
}

void
sack_solve(Sack sack, Queue *job, Map *res_map, int mode)
{
    Transaction *trans = job2transaction(sack, job, NULL);

    if (!trans) {
	;
    } else if (res_map) {
	switch (mode) {
	case SOLVER_TRANSACTION_UPGRADE:
	    transaction2map(trans, res_map);
	    break;
	case SOLVER_TRANSACTION_OBSOLETES:
	    transaction2obs_map(trans, res_map);
	    break;
	default:
	    assert(0);
	    break;
	}
    } else {
	printf("(solver succeeded with %d step transaction. results ignored.)\n",
	       trans->steps.count);
#if 0
	transaction_print(trans);
#endif
    }
    if (trans)
	transaction_free(trans);
}

// internal

void
sack_same_names(Sack sack, Id name, Queue *same)
{
    Pool *pool = sack->pool;
    Id p, pp;

    sack_make_provides_ready(sack);
    FOR_PROVIDES(p, pp, name) {
	Solvable *s = pool_id2solvable(pool, p);
	if (s->name == name)
	    queue_push(same, p);
    }
}
