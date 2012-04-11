#define _GNU_SOURCE
#include <check.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <wordexp.h>
#include <sys/stat.h>
#include <sys/types.h>

// libsolv
#include <solv/repo.h>
#include <solv/testcase.h>
#include <solv/util.h>

// hawkey
#include "src/iutil.h"
#include "src/sack_internal.h"
#include "testsys.h"

/* define the global variable */
struct TestGlobals_s test_globals;


void
dump_packagelist(HyPackageList plist)
{
    for (int i = 0; i < hy_packagelist_count(plist); ++i) {
	HyPackage pkg = hy_packagelist_get(plist, i);
	char *nvra = hy_package_get_nvra(pkg);
	printf("\t%s\n", nvra);
	solv_free(nvra);
    }
}

HyRepo
glob_for_repofiles(Pool *pool, const char *repo_name, const char *path)
{
    HyRepo repo = hy_repo_create();
    const char *tmpl;
    wordexp_t word_vector;

    hy_repo_set_string(repo, HY_REPO_NAME, repo_name);

    tmpl = pool_tmpjoin(pool, path, "/repomd.xml", NULL);
    if (wordexp(tmpl, &word_vector, 0) || word_vector.we_wordc < 1)
	goto fail;
    hy_repo_set_string(repo, HY_REPO_MD_FN, word_vector.we_wordv[0]);

    tmpl = pool_tmpjoin(pool, path, "/*primary.xml.gz", NULL);
    if (wordexp(tmpl, &word_vector, WRDE_REUSE) || word_vector.we_wordc < 1)
	goto fail;
    hy_repo_set_string(repo, HY_REPO_PRIMARY_FN, word_vector.we_wordv[0]);

    tmpl = pool_tmpjoin(pool, path, "/*filelists.xml.gz", NULL);
    if (wordexp(tmpl, &word_vector, WRDE_REUSE) || word_vector.we_wordc < 1)
	goto fail;
    hy_repo_set_string(repo, HY_REPO_FILELISTS_FN, word_vector.we_wordv[0]);
    wordfree(&word_vector);
    return repo;

 fail:
    wordfree(&word_vector);
    hy_repo_free(repo);
    return NULL;
}

int
load_repo(Pool *pool, const char *name, const char *path, int installed)
{
    Repo *r = repo_create(pool, name);
    FILE *fp = fopen(path, "r");

    if (!fp)
	return 1;
    testcase_add_susetags(r,  fp, 0);
    if (installed)
	pool_set_installed(pool, r);
    fclose(fp);
    return 0;
}

void
setup_empty_sack(void)
{
    HySack sack = hy_sack_create();
    test_globals.sack = sack;
    hy_sack_set_cache_path(sack, test_globals.tmpdir);
    HY_LOG_INFO("HySack for UT created: %p", sack);
}

void
setup(void)
{
    setup_empty_sack();
    Pool *pool = sack_pool(test_globals.sack);
    const char *path = pool_tmpjoin(pool, test_globals.repo_dir,
				    "system.repo", 0);

    fail_if(load_repo(pool, HY_SYSTEM_REPO_NAME, path, 1));
}

void
setup_with_updates(void)
{
    setup();
    Pool *pool = sack_pool(test_globals.sack);
    const char *path = pool_tmpjoin(pool, test_globals.repo_dir,
				    "updates.repo", 0);
    fail_if(load_repo(pool, "updates", path, 0));
}

void setup_all(void)
{
    setup_with_updates();
    Pool *pool = sack_pool(test_globals.sack);
    const char *path = pool_tmpjoin(pool, test_globals.repo_dir,
				    "main.repo", 0);

    fail_if(load_repo(pool, "main", path, 0));
}

void setup_yum_sack(HySack sack)
{
    Pool *pool = sack_pool(sack);
    const char *repo_path = pool_tmpjoin(pool, test_globals.repo_dir,
					 YUM_DIR_SUFFIX, NULL);
    fail_if(access(repo_path, X_OK));
    HyRepo repo = glob_for_repofiles(pool, "tfilenames", repo_path);

    hy_sack_load_yum_repo(sack, repo);
    fail_unless(pool->nsolvables ==
		TEST_META_SOLVABLES_COUNT + TEST_EXPECT_YUM_NSOLVABLES);
    hy_repo_free(repo);
}

void setup_yum(void)
{
    setup_empty_sack();
    HySack sack = test_globals.sack;
    setup_yum_sack(sack);

    fail_if(hy_sack_write_all_repos(sack)); // the next setup_yum() will use the cache
    fail_if(hy_sack_load_filelists(sack));
}

void
teardown(void)
{
    hy_sack_free(test_globals.sack);
    test_globals.sack = NULL;
}
