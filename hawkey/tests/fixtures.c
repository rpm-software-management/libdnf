#include <check.h>
#include <stdarg.h>
#include <unistd.h>

// hawkey
#include "src/iutil.h"
#include "src/sack_internal.h"
#include "fixtures.h"
#include "testsys.h"

/* define the global variable */
struct TestGlobals_s test_globals;

static HySack
create_ut_sack(void)
{
    HySack sack = hy_sack_create(test_globals.tmpdir, TEST_FIXED_ARCH);
    test_globals.sack = sack;
    HY_LOG_INFO("HySack for UT created: %p", sack);
    return sack;
}

static int
setup_with(HySack sack, ...)
{
    Pool *pool = sack_pool(sack);
    va_list names;
    int ret = 0;

    va_start(names, sack);
    const char *name = va_arg(names, const char *);
    while (name) {
	const char *path = pool_tmpjoin(pool, test_globals.repo_dir,
					name, ".repo");
	int installed = !strcmp(name, HY_SYSTEM_REPO_NAME);

	ret |= load_repo(pool, name, path, installed);
	name = va_arg(names, const char *);
    }
    va_end(names);
    return ret;
}

void
fixture_empty(void)
{
    create_ut_sack();
}

void
fixture_greedy_only(void)
{
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, "greedy", NULL));
}

void
fixture_system_only(void)
{
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, NULL));
}

void
fixture_with_main(void)
{
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, "main", NULL));
}

void
fixture_with_updates(void)
{
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, "updates", NULL));
}

void
fixture_all(void)
{
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, "main", "updates", NULL));
}

void fixture_yum(void)
{
    HySack sack = create_ut_sack();
    setup_yum_sack(sack, YUM_REPO_NAME);
}

void setup_yum_sack(HySack sack, const char *yum_repo_name)
{
    Pool *pool = sack_pool(sack);
    const char *repo_path = pool_tmpjoin(pool, test_globals.repo_dir,
					 YUM_DIR_SUFFIX, NULL);
    fail_if(access(repo_path, X_OK));
    HyRepo repo = glob_for_repofiles(pool, yum_repo_name, repo_path);

    fail_if(hy_sack_load_yum_repo(sack, repo,
				  HY_BUILD_CACHE |
				  HY_LOAD_FILELISTS |
				  HY_LOAD_PRESTO));
    fail_unless(hy_sack_count(sack) == TEST_EXPECT_YUM_NSOLVABLES);
    hy_repo_free(repo);
}

void
teardown(void)
{
    hy_sack_free(test_globals.sack);
    test_globals.sack = NULL;
}
