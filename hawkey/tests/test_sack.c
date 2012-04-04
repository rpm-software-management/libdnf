#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

// libsolv
#include <solv/testcase.h>

// hawkey
#include "src/repo_internal.h"
#include "src/sack_internal.h"
#include "testsys.h"

START_TEST(test_environment)
{
    /* currently only regular user is supported in unit tests */
    fail_if(geteuid() == 0);
    /* check the tmpdir was created */
    fail_if(access(test_globals.tmpdir, W_OK));
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
    Pool *pool = sack->pool;
    char *tmpdir = test_globals.tmpdir;

    hy_sack_set_cache_path(sack, tmpdir);

    /* hy_sack_write_all repos needs HyRepo in every repo */
    Repo *repo;
    int i;
    FOR_REPOS(i, repo)
	if (!strcmp(repo->name, SYSTEM_REPO_NAME)) {
	    repo->appdata = hy_repo_create();
	}

    fail_if(hy_sack_write_all_repos(sack));
    fail_if(access(tmpdir, R_OK|W_OK|X_OK));
    char *filename = solv_dupjoin(tmpdir, "/", "@System.solv");
    fail_unless(access(filename, R_OK|W_OK));

    ((HyRepo)repo->appdata)->state = _HY_LOADED_FETCH;
    fail_if(hy_sack_write_all_repos(sack));
    fail_if(access(filename, R_OK|W_OK));

    solv_free(filename);
}
END_TEST

START_TEST(test_yum_repo)
{
    Pool *pool = sack_pool(test_globals.sack);

    Dataiterator di;
    int count;
    Id last_found_solvable;
    dataiterator_init(&di, pool, 0, 0, SOLVABLE_FILELIST, "/usr/bin/ste",
		      SEARCH_STRING | SEARCH_FILES | SEARCH_COMPLETE_FILELIST);
    for (count = 0; dataiterator_step(&di); ++count)
	last_found_solvable = di.solvid;
    fail_unless(count == 1);
    dataiterator_free(&di);

    dataiterator_init(&di, pool, 0, last_found_solvable, SOLVABLE_FILELIST, "/",
		      SEARCH_STRINGSTART | SEARCH_FILES);
    for (count = 0; dataiterator_step(&di); ++count)
	fail_if(strncmp(di.kv.str, "/usr/bin/", strlen("/usr/bin/")));
    fail_unless(count == 3);
    dataiterator_free(&di);
}
END_TEST

Suite *
sack_suite(void)
{
    Suite *s = suite_create("Sack");
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

    tc = tcase_create("YumRepo");
    tcase_add_unchecked_fixture(tc, setup_yum, teardown);
    tcase_add_test(tc, test_yum_repo);
    suite_add_tcase(s, tc);

    return s;
}
