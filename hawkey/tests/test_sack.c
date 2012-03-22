#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

// libsolv
#include <solv/testcase.h>

// hawkey
#include "src/sack_internal.h"
#include "testsys.h"

START_TEST(test_environment)
{
    /* currently only regular user is supported in unit tests */
    fail_if(geteuid() == 0);
}
END_TEST

START_TEST(test_sack_create)
{
    HySack sack = hy_sack_create();
    fail_if(sack == NULL, NULL);
    fail_if(sack_pool(sack) == NULL, NULL);
    hy_sack_free(sack);
}
END_TEST

START_TEST(test_repo_load)
{
    HySack sack = hy_sack_create();
    Pool *pool = sack_pool(sack);
    Repo *r = repo_create(sack_pool(sack), SYSTEM_REPO_NAME);
    const char *repo = pool_tmpjoin(pool, test_globals.repo_dir, "system.repo", 0);
    FILE *fp = fopen(repo, "r");

    testcase_add_susetags(r,  fp, 0);
    fail_unless(pool->nsolvables == TEST_EXPECT_SYSTEM_NSOLVABLES);

    fclose(fp);
    hy_sack_free(sack);
}
END_TEST

START_TEST(test_sack_solv_path)
{
    HySack sack = hy_sack_create();
    char *path = hy_sack_solv_path(sack, NULL);
    fail_if(strstr(path, "/var/tmp/hawkey/") == NULL);
    fail_unless(strlen(path) > strlen("/var/tmp/hawkey/"));
    solv_free(path);

    path = hy_sack_solv_path(sack, "rain");
    fail_if(strstr(path, "rain.solv") == NULL);
    solv_free(path);
    hy_sack_free(sack);
}
END_TEST

Suite *
sack_suite(void)
{
    Suite *s = suite_create("HySack");
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_environment);
    tcase_add_test(tc_core, test_sack_create);
    tcase_add_test(tc_core, test_repo_load);
    tcase_add_test(tc_core, test_sack_solv_path);
    suite_add_tcase(s, tc_core);
    return s;
}
