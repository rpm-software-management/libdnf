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
    HySack sack = hy_sack_create(NULL);
    fail_if(sack == NULL, NULL);
    fail_if(sack_pool(sack) == NULL, NULL);
    hy_sack_free(sack);
}
END_TEST

START_TEST(test_give_cache_fn)
{
    HySack sack = hy_sack_create(NULL);

    char *path = hy_sack_give_cache_fn(sack, "rain", NULL);
    fail_if(strstr(path, "rain.solv") == NULL);
    solv_free(path);

    path = hy_sack_give_cache_fn(sack, "rain", HY_EXT_FILENAMES);
    fail_if(strstr(path, "rain-filenames.solvx") == NULL);
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

    /* hy_sack_write_all repos needs HyRepo in every repo */
    Repo *repo;
    int i;
    FOR_REPOS(i, repo)
	if (!strcmp(repo->name, HY_SYSTEM_REPO_NAME)) {
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

START_TEST(test_filelist_from_cache)
{
    HySack sack = test_globals.sack;
    char *fn_solv = hy_sack_give_cache_fn(sack, "tfilenames", HY_EXT_FILENAMES);

    fail_if(hy_sack_write_filelists(sack));
    fail_if(access(fn_solv, R_OK));

    // create new sack, check it can work with the cached filenames OK
    sack = hy_sack_create(test_globals.tmpdir);
    HY_LOG_INFO("created custom sack, loading yum\n");
    setup_yum_sack(sack);

    int total_solvables = sack->pool->nsolvables;
    fail_if(hy_sack_load_filelists(sack));
    fail_unless(total_solvables == sack->pool->nsolvables);

    Pool *pool = sack_pool(sack);
    Dataiterator di;
    int count;
    dataiterator_init(&di, pool, 0, 0, SOLVABLE_FILELIST, "/usr/bin/ste",
		      SEARCH_STRING | SEARCH_FILES | SEARCH_COMPLETE_FILELIST);
    for (count = 0; dataiterator_step(&di); ++count)
	;
    fail_unless(count == 1);
    dataiterator_free(&di);
    hy_sack_free(sack);

    // remove the file so the remaining tests do no try to use it:
    fail_if(unlink(fn_solv));
    solv_free(fn_solv);
}
END_TEST

Suite *
sack_suite(void)
{
    Suite *s = suite_create("Sack");
    TCase *tc = tcase_create("Core");
    tcase_add_test(tc, test_environment);
    tcase_add_test(tc, test_sack_create);
    tcase_add_test(tc, test_give_cache_fn);
    suite_add_tcase(s, tc);

    tc = tcase_create("Repos");
    tcase_add_unchecked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_repo_load);
    tcase_add_test(tc, test_write_all_repos);
    suite_add_tcase(s, tc);

    tc = tcase_create("YumRepo");
    tcase_add_unchecked_fixture(tc, setup_yum, teardown);
    tcase_add_test(tc, test_yum_repo);
    tcase_add_test(tc, test_filelist_from_cache);
    suite_add_tcase(s, tc);

    return s;
}
