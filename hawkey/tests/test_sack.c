#define _GNU_SOURCE
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

START_TEST(test_repo_load)
{
    fail_unless(test_globals.sack->pool->nsolvables ==
		TEST_EXPECT_SYSTEM_NSOLVABLES);
}
END_TEST

START_TEST(test_write_all_repos)
{
    HySack sack = test_globals.sack;
    char *tmpdir = solv_dupjoin(UNITTEST_DIR, "XXXXXX", NULL);
    fail_if(mkdtemp(tmpdir) == NULL);
    hy_sack_set_cache_path(sack, tmpdir);
    fail_if(hy_sack_write_all_repos(sack));

    fail_if(access(tmpdir, R_OK|W_OK|X_OK));
    char *filename = solv_dupjoin(tmpdir, "@System.solv", NULL);
    fail_if(access(tmpdir, R_OK|W_OK|X_OK));
    solv_free(filename);
    solv_free(tmpdir);
}
END_TEST

Suite *
sack_suite(void)
{
    Suite *s = suite_create("HySack");
    TCase *tc = tcase_create("Core");
    tcase_add_test(tc, test_environment);
    tcase_add_test(tc, test_sack_create);
    tcase_add_test(tc, test_sack_solv_path);
    suite_add_tcase(s, tc);

    tc = tcase_create("Repos");
    tcase_add_unchecked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_repo_load);
    tcase_add_test(tc, test_write_all_repos);
    suite_add_tcase(s, tc);

    return s;
}
