#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
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
#include "package_internal.h"
#include "repo_internal.h"
#include "sack_internal.h"

#define DEFAULT_CACHE_ROOT "/var/cache/hawkey"
#define DEFAULT_CACHE_USER "/var/tmp/hawkey"

static int
can_use_rpmdb_cache(FILE *fp_cache)
{
    FILE *fp_rpmdb = fopen(SYSTEM_RPMDB, "r");
    unsigned char cs_cache[CHKSUM_BYTES];
    unsigned char cs_rpmdb[CHKSUM_BYTES];
    int ret = 0;

    if (fp_cache && fp_rpmdb && \
	!checksum_read(cs_cache, fp_cache) && \
	!checksum_stat(cs_rpmdb, fp_rpmdb) && \
	!checksum_cmp(cs_cache, cs_rpmdb))
	ret = 1;

    if (fp_rpmdb)
	fclose(fp_rpmdb);
    return ret;
}

static int
can_use_repomd_cache(FILE *fp_cache, FILE *fp_repomd)
{
    unsigned char cs_cache[CHKSUM_BYTES];
    unsigned char cs_repomd[CHKSUM_BYTES];

    if (fp_cache && fp_repomd && \
	!checksum_read(cs_cache, fp_cache) && \
	!checksum_fp(cs_repomd, fp_repomd) && \
	!checksum_cmp(cs_cache, cs_repomd))
	return 1;

    return 0;
}

static Repo *
get_cmdline_repo(HySack sack)
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
    int i;
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
	    transaction_all_obs_pkgs(trans, p, &obsoleted);
	    assert(obsoleted.count);
	}
    }
    queue_free(&obsoleted);
}

static void
log_cb(Pool *pool, void *cb_data, int type, const char *buf)
{
    HySack sack = cb_data;

    if (sack->log_out == NULL) {
	char *dir = hy_sack_solv_path(sack, NULL);
	const char *fn = pool_tmpjoin(pool, dir, "/hawkey.log", NULL);
	int res = mkcachedir(dir);

	solv_free(dir);
	assert(res == 0);
	sack->log_out = fopen(fn, "a");
	const char *msg = "log started\n";
	fwrite(msg, strlen(msg), 1, sack->log_out);
    }
    fwrite(buf, strlen(buf), 1, sack->log_out);
}

HySack
hy_sack_create(void)
{
    HySack sack = solv_calloc(1, sizeof(*sack));
    Pool *pool = pool_create();

    setarch(pool);
    sack->pool = pool;

    int is_reg_user = geteuid();
    char *username = this_username();
    if (is_reg_user)
	sack->cache_dir = solv_dupjoin(DEFAULT_CACHE_USER, "/", username);
    else
	sack->cache_dir = solv_strdup(DEFAULT_CACHE_ROOT);
    solv_free(username);

    pool_setdebugcallback(pool, log_cb, sack);
    pool_setdebugmask(pool, SOLV_ERROR | SOLV_FATAL | SOLV_WARN |
		      HY_LL_INFO | HY_LL_ERROR);

    return sack;
}

void
hy_sack_free(HySack sack)
{
    Pool *pool = sack->pool;
    Repo *repo;
    int i;

    FOR_REPOS(i, repo) {
	HyRepo hrepo = repo->appdata;
	if (hrepo == NULL)
	    continue;
	hy_repo_free(hrepo);
    }

    solv_free(sack->cache_dir);
    pool_free(sack->pool);
    if (sack->log_out) {
	const char *msg = "finished.\n";
	fwrite(msg, strlen(msg), 1, sack->log_out);
	fclose(sack->log_out);
    }
    solv_free(sack);
}

char *
hy_sack_solv_path(HySack sack, const char *reponame)
{
    if (reponame == NULL)
	return solv_strdup(sack->cache_dir);
    char *fn = solv_dupjoin(sack->cache_dir, "/", reponame);
    return solv_dupappend(fn, ".solv", NULL);
}

void
hy_sack_set_cache_path(HySack sack, const char *path)
{
    solv_free(sack->cache_dir);
    sack->cache_dir = solv_strdup(path);
}

/**
 * Creates repo for command line rpms.
 */
void
hy_sack_create_cmdline_repo(HySack sack)
{
    repo_create(sack->pool, CMDLINE_REPO_NAME);
}

/**
 * Adds the given .rpm file to the command line repo.
 */
HyPackage
hy_sack_add_cmdline_rpm(HySack sack, const char *fn)
{
    Repo *repo = get_cmdline_repo(sack);
    Id p;

    assert(repo);
    if (!is_readable_rpm(fn)) {
	HY_LOG_ERROR("not a readable RPM file: %s, skipping\n", fn);
	return NULL;
    }
    p = repo_add_rpm(repo, fn, REPO_REUSE_REPODATA|REPO_NO_INTERNALIZE);
    sack->provides_ready = 0;    /* triggers internalizing later */
    return package_create(sack->pool, p);
}

void
hy_sack_load_rpm_repo(HySack sack)
{
    Pool *pool = sack->pool;
    Repo *repo = repo_create(pool, SYSTEM_REPO_NAME);
    char *cache_fn = hy_sack_solv_path(sack, SYSTEM_REPO_NAME);
    FILE *cache_fp = fopen(cache_fn, "r");
    HyRepo hrepo = hy_repo_create();

    free(cache_fn);
    hy_repo_set_string(hrepo, HY_REPO_NAME, SYSTEM_REPO_NAME);
    if (can_use_rpmdb_cache(cache_fp)) {
	HY_LOG_INFO("using cached rpmdb\n");
	if (repo_add_solv(repo, cache_fp, 0))
	    assert(0);
	hrepo->from_cache = 1;
    } else {
	HY_LOG_INFO("fetching rpmdb\n");
	repo_add_rpmdb(repo, 0, 0, REPO_REUSE_REPODATA);
	hrepo->from_cache = 0;
    }

    if (cache_fp)
	fclose(cache_fp);
    pool_set_installed(pool, repo);
    repo->appdata = hrepo;
    sack->provides_ready = 0;
}

void hy_sack_load_yum_repo(HySack sack, HyRepo hrepo)
{
    Pool *pool = sack->pool;
    const char *name = hy_repo_get_string(hrepo, HY_REPO_NAME);
    Repo *repo = repo_create(pool, name);
    char *fn_cache = hy_sack_solv_path(sack, name);

    FILE *fp_cache = fopen(fn_cache, "r");
    FILE *fp_repomd = fopen(hy_repo_get_string(hrepo, HY_REPO_MD_FN), "r");

    if (can_use_repomd_cache(fp_cache, fp_repomd)) {
	HY_LOG_INFO("using cached %s\n", name);
	if (repo_add_solv(repo, fp_cache, 0))
	    assert(0);
	hrepo->from_cache = 1;
    } else {
	FILE *fp_primary = solv_xfopen(hy_repo_get_string(hrepo, HY_REPO_PRIMARY_FN), "r");

	if (!(fp_repomd && fp_primary))
	    assert(0);

	HY_LOG_INFO("fetching %s\n", name);
	repo_add_repomdxml(repo, fp_repomd, 0);
	repo_add_rpmmd(repo, fp_primary, 0, 0);

	fclose(fp_primary);
	hrepo->from_cache = 0;
    }
    if (fp_cache)
	fclose(fp_cache);
    if (fp_repomd)
	fclose(fp_repomd);
    free(fn_cache);

    repo->appdata = hy_repo_link(hrepo);
    sack->provides_ready = 0;
}

int
hy_sack_load_filelists(HySack sack)
{
    Pool *pool = sack->pool;
    int ret = 0;
    Repo *repo;
    Id rid;

    FOR_REPOS(rid, repo) {
	HyRepo hrepo = repo->appdata;
	const char *fn;
	FILE *fp;

	if (hrepo == NULL)\
	    continue;
	fn = hy_repo_get_string(hrepo, HY_REPO_FILELISTS_FN);
	if (fn == NULL)
	    continue;
	fp = solv_xfopen(fn, "r");
	if (fp == NULL)
	    continue;
	ret |= repo_add_rpmmd(repo, fp, "FL", REPO_EXTEND_SOLVABLES);
	fclose(fp);
    }
    return ret;
}

void
sack_make_provides_ready(HySack sack)
{
    if (!sack->provides_ready) {
	pool_addfileprovides(sack->pool);
	pool_createwhatprovides(sack->pool);
	sack->provides_ready = 1;
    }
}

int
hy_sack_write_all_repos(HySack sack)
{
    Pool *pool = sack->pool;
    Repo *repo;
    int i;

    char *dir = hy_sack_solv_path(sack, NULL);
    int res = mkcachedir(dir);
    solv_free(dir);
    assert(res == 0);
    if (res)
	return res;

    FOR_REPOS(i, repo) {
	const char *name = repo->name;
	HyRepo hrepo = repo->appdata;
	unsigned char checksum[CHKSUM_BYTES];
	FILE *fp = NULL;

	if (!strcmp(name, CMDLINE_REPO_NAME))
	    continue;
	if (hrepo && hrepo->from_cache)
	    continue;

	if (!strcmp(name, SYSTEM_REPO_NAME)) {
	    if ((fp = fopen(SYSTEM_RPMDB, "r")) != NULL)
		checksum_stat(checksum, fp);
	} else if (hrepo) {
	    if ((fp = fopen(hy_repo_get_string(hrepo, HY_REPO_MD_FN), "r")) != NULL)
		checksum_fp(checksum, fp);
	} else {
	    assert(0);
	}
	if (fp)
	    fclose(fp);

	char *fn = hy_sack_solv_path(sack, name);
	fp = fopen(fn, "w+");
	free(fn);
	HY_LOG_INFO("caching repo: %s\n", name);
	if (!fp) {
	    perror(__func__);
	    return 1;
	}
	repo_write(repo, fp);
	checksum_write(checksum, fp);
	fclose(fp);
    }
    return 0;
}

void
hy_sack_solve(HySack sack, Queue *job, Map *res_map, int mode)
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
	HY_LOG_INFO("(solver succeeded with %d step transaction. results ignored.)\n",
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
sack_log(HySack sack, int level, const char *format, ...)
{
    Pool *pool = sack_pool(sack);
    char buf[1024];
    va_list args;
    const char *pref_format = pool_tmpjoin(pool, "hawkey: ", format, NULL);

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), pref_format, args);
    va_end(args);
    POOL_DEBUG(level, buf);
}

void
sack_same_names(HySack sack, Id name, Queue *same)
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
