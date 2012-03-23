#define _GNU_SOURCE
#include <check.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

// libsolv
#include <solv/repo.h>
#include <solv/testcase.h>
#include <solv/util.h>

// hawkey
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
    test_globals.sack = hy_sack_create();
}

void
setup(void)
{
    setup_empty_sack();
    Pool *pool = sack_pool(test_globals.sack);
    const char *path = pool_tmpjoin(pool, test_globals.repo_dir,
				    "system.repo", 0);

    fail_if(load_repo(pool, SYSTEM_REPO_NAME, path, 1));
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
    hy_sack_free(test_globals.sack);
    test_globals.sack = NULL;
}
