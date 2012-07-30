#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

// libsolv
#include <solv/testcase.h>

// hawkey
#include "src/package_internal.h"
#include "src/repo_internal.h"
#include "src/sack_internal.h"
#include "src/util.h"
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
    HySack sack = hy_sack_create(test_globals.tmpdir, NULL);
    fail_if(sack == NULL, NULL);
    fail_if(sack_pool(sack) == NULL, NULL);
    hy_sack_free(sack);
}
END_TEST

START_TEST(test_give_cache_fn)
{
    HySack sack = hy_sack_create(test_globals.tmpdir, NULL);

    char *path = hy_sack_give_cache_fn(sack, "rain", NULL);
    fail_if(strstr(path, "rain.solv") == NULL);
    hy_free(path);

    path = hy_sack_give_cache_fn(sack, "rain", HY_EXT_FILENAMES);
    fail_if(strstr(path, "rain-filenames.solvx") == NULL);
    hy_free(path);
    hy_sack_free(sack);
}
END_TEST

START_TEST(test_load_yum_repo_err)
{
    HySack sack = hy_sack_create(test_globals.tmpdir, NULL);
    HyRepo repo = hy_repo_create();
    hy_repo_set_string(repo, HY_REPO_NAME, "crabalocker");
    hy_repo_set_string(repo, HY_REPO_MD_FN, "/non/existing");
    fail_unless(hy_sack_load_yum_repo(sack, repo, 0) == 1);
    hy_repo_free(repo);
    hy_sack_free(sack);
}
END_TEST

START_TEST(test_yum_repo_written)
{
    HySack sack = hy_sack_create(test_globals.tmpdir, NULL);
    Pool *pool = sack_pool(sack);
    char *filename = hy_sack_give_cache_fn(sack, "test_sack_written", NULL);

    fail_unless(access(filename, R_OK|W_OK));
    setup_yum_sack(sack, "test_sack_written");

    HyRepo repo = repo_by_name(pool, "test_sack_written");
    fail_if(repo == NULL);
    fail_unless(repo->state_main == _HY_WRITTEN);
    fail_unless(repo->state_filelists == _HY_WRITTEN);
    fail_unless(repo->state_presto == _HY_WRITTEN);
    fail_if(access(filename, R_OK|W_OK));

    hy_free(filename);
    hy_sack_free(sack);
}
END_TEST

START_TEST(test_repo_load)
{
    fail_unless(test_globals.sack->pool->nsolvables ==
		TEST_EXPECT_SYSTEM_NSOLVABLES);
}
END_TEST

static void
check_filelist(Pool *pool)
{
    Dataiterator di;
    int count;
    Id last_found_solvable = 0;
    dataiterator_init(&di, pool, 0, 0, SOLVABLE_FILELIST, "/usr/bin/ste",
		      SEARCH_STRING | SEARCH_FILES | SEARCH_COMPLETE_FILELIST);
    for (count = 0; dataiterator_step(&di); ++count)
	last_found_solvable = di.solvid;
    fail_unless(count == 1);
    fail_if(last_found_solvable == 0);
    dataiterator_free(&di);

    dataiterator_init(&di, pool, 0, last_found_solvable, SOLVABLE_FILELIST, "/",
		      SEARCH_STRINGSTART | SEARCH_FILES);
    for (count = 0; dataiterator_step(&di); ++count)
	fail_if(strncmp(di.kv.str, "/usr/bin/", strlen("/usr/bin/")));
    fail_unless(count == 3);
    dataiterator_free(&di);

    dataiterator_init(&di, pool, 0, 0, SOLVABLE_FILELIST,
		      "/usr/lib/python2.7/site-packages/tour/today.pyc",
		      SEARCH_STRING | SEARCH_FILES | SEARCH_COMPLETE_FILELIST);
    for (count = 0; dataiterator_step(&di); ++count) ;
    fail_unless(count == 1);
}

START_TEST(test_filelist)
{
    HySack sack = test_globals.sack;
    HyRepo repo = repo_by_name(sack_pool(sack), YUM_REPO_NAME);
    char *fn_solv = hy_sack_give_cache_fn(sack, YUM_REPO_NAME, HY_EXT_FILENAMES);

    fail_unless(repo->state_filelists == _HY_WRITTEN);
    fail_if(access(fn_solv, R_OK));
    hy_free(fn_solv);

    check_filelist(sack_pool(test_globals.sack));
}
END_TEST

START_TEST(test_filelist_from_cache)
{
    HySack sack = hy_sack_create(test_globals.tmpdir, NULL);
    setup_yum_sack(sack, YUM_REPO_NAME);

    HyRepo repo = repo_by_name(sack_pool(sack), YUM_REPO_NAME);
    fail_unless(repo->state_filelists == _HY_LOADED_CACHE);
    check_filelist(sack_pool(sack));
    hy_sack_free(sack);
}
END_TEST

static void
check_prestoinfo(Pool *pool)
{
    Dataiterator di;

    dataiterator_init(&di, pool, NULL, SOLVID_META, DELTA_PACKAGE_NAME, "tour",
		      SEARCH_STRING);
    dataiterator_prepend_keyname(&di, REPOSITORY_DELTAINFO);
    fail_unless(dataiterator_step(&di));
    dataiterator_setpos_parent(&di);
    const char *attr;
    attr = pool_lookup_str(pool, SOLVID_POS, DELTA_SEQ_NAME);
    ck_assert_str_eq(attr, "tour");
    attr = pool_lookup_str(pool, SOLVID_POS, DELTA_SEQ_EVR);
    ck_assert_str_eq(attr, "4-5");
    attr = pool_lookup_str(pool, SOLVID_POS, DELTA_LOCATION_DIR);
    ck_assert_str_eq(attr, "drpms");
    dataiterator_free(&di);
    return;
}

START_TEST(test_presto)
{
    HySack sack = test_globals.sack;
    HyRepo repo = repo_by_name(sack_pool(sack), YUM_REPO_NAME);
    char *fn_solv = hy_sack_give_cache_fn(sack, YUM_REPO_NAME, HY_EXT_PRESTO);

    fail_if(access(fn_solv, R_OK));
    fail_unless(repo->state_presto == _HY_WRITTEN);
    hy_free(fn_solv);
    check_prestoinfo(sack_pool(sack));
}
END_TEST

START_TEST(test_presto_from_cache)
{
    HySack sack = hy_sack_create(test_globals.tmpdir, TEST_FIXED_ARCH);
    setup_yum_sack(sack, YUM_REPO_NAME);

    HyRepo repo = repo_by_name(sack_pool(sack), YUM_REPO_NAME);
    fail_unless(repo->state_presto == _HY_LOADED_CACHE);
    check_prestoinfo(sack_pool(sack));
    hy_sack_free(sack);
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
    tcase_add_test(tc, test_load_yum_repo_err);
    tcase_add_test(tc, test_yum_repo_written);
    suite_add_tcase(s, tc);

    tc = tcase_create("Repos");
    tcase_add_unchecked_fixture(tc, fixture_system_only, teardown);
    tcase_add_test(tc, test_repo_load);
    suite_add_tcase(s, tc);

    tc = tcase_create("YumRepo");
    tcase_add_unchecked_fixture(tc, fixture_yum, teardown);
    tcase_add_test(tc, test_filelist);
    tcase_add_test(tc, test_filelist_from_cache);
    tcase_add_test(tc, test_presto);
    tcase_add_test(tc, test_presto_from_cache);
    suite_add_tcase(s, tc);

    return s;
}
