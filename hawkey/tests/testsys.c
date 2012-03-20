#include <check.h>

// libsolv
#include <solv/repo.h>
#include <solv/testcase.h>
#include <solv/util.h>

// hawkey
#include "src/sack_internal.h"
#include "testsys.h"

/* define the global variable */
struct TestGlobals_s test_globals;

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
setup(void)
{
    HySack sack = sack_create();
    Pool *pool = sack_pool(sack);
    const char *path = pool_tmpjoin(pool, test_globals.repo_dir,
				    "system.repo", 0);

    fail_if(load_repo(pool, SYSTEM_REPO_NAME, path, 1));
    test_globals.sack = sack;
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

void
teardown(void)
{
    sack_free(test_globals.sack);
    test_globals.sack = NULL;
}

void
dump_packagelist(HyPackageList plist)
{
    for (int i = 0; i < packagelist_count(plist); ++i) {
	HyPackage pkg = packagelist_get(plist, i);
	char *nvra = package_get_nvra(pkg);
	printf("\t%s\n", nvra);
	solv_free(nvra);
    }
}
