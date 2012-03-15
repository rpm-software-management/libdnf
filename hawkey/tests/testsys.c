#include <check.h>

// libsolv
#include <solv/repo.h>
#include <solv/testcase.h>
#include <solv/util.h>

// hawkey
#include "testsys.h"

void
setup(void)
{
    Sack sack = sack_create();
    Pool *pool = sack->pool;
    Repo *r = repo_create(pool, SYSTEM_REPO_NAME);
    const char *repo = pool_tmpjoin(pool, test_globals.repo_dir,
				    "system.repo", 0);
    FILE *fp = fopen(repo, "r");

    testcase_add_susetags(r,  fp, 0);
    pool_set_installed(pool, r);
    fail_unless(pool->nsolvables == 5);

    fclose(fp);
    test_globals.sack = sack;
}

void
setup_with_updates(void)
{
    setup();
    Pool *pool = test_globals.sack->pool;
    Repo *r = repo_create(pool, "updates");
    const char *repo = pool_tmpjoin(pool, test_globals.repo_dir,
				    "updates.repo", 0);

    FILE *fp = fopen(repo, "r");

    testcase_add_susetags(r, fp, 0);
    fclose(fp);
}

void
teardown(void)
{
    sack_free(test_globals.sack);
    test_globals.sack = NULL;
}

void
dump_packagelist(PackageList plist)
{
    for (int i = 0; i < packagelist_count(plist); ++i) {
	Package pkg = packagelist_get(plist, i);
	char *nvra = package_get_nvra(pkg);
	printf("\t%s\n", nvra);
	solv_free(nvra);
    }
}
