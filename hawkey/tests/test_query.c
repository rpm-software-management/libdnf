#include <check.h>

// libsolv
#include <solv/testcase.h>

// hawkey
#include "src/query.h"
#include "src/sack_internal.h"
#include "testsys.h"
#include "test_query.h"

START_TEST(test_query_sanity)
{
    HySack sack = test_globals.sack;
    fail_unless(sack != NULL);
    fail_unless(sack_pool(sack)->nsolvables == TEST_EXPECT_SYSTEM_NSOLVABLES);
    fail_unless(sack_pool(sack)->installed != NULL);

    HyQuery query = hy_query_create(sack);
    fail_unless(query != NULL);
    hy_query_free(query);
}
END_TEST

START_TEST(test_query_clear)
{
    HyQuery q;

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_NEQ, "fool");
    hy_query_clear(q);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "fool");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_repo)
{
    HyQuery q;

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_REPO, HY_EQ, HY_SYSTEM_REPO_NAME);

    fail_unless(query_count_results(q) == TEST_EXPECT_SYSTEM_NSOLVABLES - 2);

    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_REPO, HY_NEQ, HY_SYSTEM_REPO_NAME);
    fail_if(query_count_results(q));

    hy_query_free(q);
}
END_TEST

START_TEST(test_query_name)
{
    HyQuery q;

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_SUBSTR, "penny");
    fail_unless(query_count_results(q) == 2);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "lane");
    fail_if(query_count_results(q));
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_evr)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_EVR, HY_EQ, "5.1-0");
    fail_unless(query_count_results(q) == 0);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_EVR, HY_EQ, "5.0-0");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_glob)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_GLOB, "pen*");
    fail_unless(query_count_results(q) == 2);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_case)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "Penny-lib");
    fail_unless(query_count_results(q) == 0);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ|HY_ICASE, "Penny-lib");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_anded)
{
    HyQuery q;

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_SUBSTR, "penny");
    hy_query_filter(q, HY_PKG_SUMMARY, HY_SUBSTR, "ears");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_neq)
{
    HyQuery q;

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_NEQ, "penny-lib");
    fail_unless(query_count_results(q) == TEST_EXPECT_SYSTEM_PKGS - 1);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_in)
{
    HyQuery q;
    const char *namelist[] = {"penny", "fool", NULL};

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_NAME, HY_EQ, namelist);
    fail_unless(query_count_results(q) == 2);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_fileprovides)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_FILE, HY_EQ, "/no/answers");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);
}
END_TEST

START_TEST(test_upgrades_sanity)
{
    Pool *pool = sack_pool(test_globals.sack);
    Repo *r = NULL;
    int i;

    FOR_REPOS(i, r)
	if (!strcmp(r->name, "updates"))
	    break;
    fail_unless(r != NULL);
    fail_unless(r->nsolvables == TEST_EXPECT_UPDATES_NSOLVABLES);
}
END_TEST

START_TEST(test_upgrades)
{
    const char *installonly[] = {"fool", NULL};
    hy_sack_set_installonly(test_globals.sack, installonly);

    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter_upgrades(q, 1);
    fail_unless(query_count_results(q) == TEST_EXPECT_UPDATES_NSOLVABLES);
    hy_query_free(q);
}
END_TEST

START_TEST(test_filter_latest)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "fool");
    hy_query_filter_latest(q, 1);
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 1);
    HyPackage pkg = hy_packagelist_get(plist, 0);
    fail_if(strcmp(hy_package_get_name(pkg), "fool"));
    fail_if(strcmp(hy_package_get_evr(pkg), "1-5"));
    hy_query_free(q);
    hy_packagelist_free(plist);
}
END_TEST

START_TEST(test_filter_latest2)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "flying");
    hy_query_filter_latest(q, 1);
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 1);
    HyPackage pkg = hy_packagelist_get(plist, 0);
    fail_if(strcmp(hy_package_get_name(pkg), "flying"));
    fail_if(strcmp(hy_package_get_evr(pkg), "3-0"));
    hy_query_free(q);
    hy_packagelist_free(plist);

}
END_TEST

START_TEST(test_filter_latest_archs)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "penny-lib");
    hy_query_filter_latest(q, 1);
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 2); /* both architectures */
    hy_query_free(q);
    hy_packagelist_free(plist);
}
END_TEST

START_TEST(test_upgrade_already_installed)
{
    /* if pkg is installed in two versions and the later is available in repos,
       it shouldn't show as a possible update. */
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "jay");
    hy_query_filter_upgrades(q, 1);
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 0);
    hy_query_free(q);
    hy_packagelist_free(plist);
}
END_TEST

START_TEST(test_downgrade)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "jay");
    hy_query_filter_downgrades(q, 1);
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 1);
    HyPackage pkg = hy_packagelist_get(plist, 0);
    ck_assert_str_eq(hy_package_get_evr(pkg), "4.9-0");
    hy_query_free(q);
    hy_packagelist_free(plist);
}
END_TEST

START_TEST(test_filter_files)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_FILE, HY_EQ, "/etc/takeyouaway");
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 1);
    HyPackage pkg = hy_packagelist_get(plist, 0);
    fail_if(strcmp(hy_package_get_name(pkg), "tour"));
    hy_packagelist_free(plist);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_FILE, HY_GLOB, "/usr/*");
    plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 2);
    hy_packagelist_free(plist);
    hy_query_free(q);
}
END_TEST

START_TEST(test_filter_reponames)
{
    HyQuery q;
    const char *repolist[]  = {"main", "updates", NULL};
    const char *repolist2[] = {"main",  NULL};
    const char *repolist3[] = {"foo", "bar",  NULL};

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_REPO, HY_EQ, repolist);
    printf("%d\n", query_count_results(q));
    fail_unless(query_count_results(q) == 15);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_REPO, HY_EQ, repolist2);
    printf("%d\n", query_count_results(q));
    fail_unless(query_count_results(q) == 13);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_REPO, HY_EQ, repolist3);
    printf("%d\n", query_count_results(q));
    fail_unless(query_count_results(q) == 0);
    hy_query_free(q);
}
END_TEST

Suite *
query_suite(void)
{
    Suite *s = suite_create("Query");
    TCase *tc;

    tc = tcase_create("Core");
    tcase_add_unchecked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_query_sanity);
    tcase_add_test(tc, test_query_clear);
    tcase_add_test(tc, test_query_repo);
    tcase_add_test(tc, test_query_name);
    tcase_add_test(tc, test_query_evr);
    tcase_add_test(tc, test_query_glob);
    tcase_add_test(tc, test_query_case);
    tcase_add_test(tc, test_query_anded);
    tcase_add_test(tc, test_query_neq);
    tcase_add_test(tc, test_query_in);
    tcase_add_test(tc, test_query_fileprovides);
    suite_add_tcase(s, tc);

    tc = tcase_create("Updates");
    tcase_add_unchecked_fixture(tc, setup_with_updates, teardown);
    tcase_add_test(tc, test_upgrades_sanity);
    tcase_add_test(tc, test_upgrades);
    tcase_add_test(tc, test_filter_latest);
    suite_add_tcase(s, tc);

    tc = tcase_create("OnlyMain");
    tcase_add_unchecked_fixture(tc, setup_with_main, teardown);
    tcase_add_test(tc, test_upgrade_already_installed);
    tcase_add_test(tc, test_downgrade);
    suite_add_tcase(s, tc);

    tc = tcase_create("Full");
    tcase_add_unchecked_fixture(tc, setup_all, teardown);
    tcase_add_test(tc, test_filter_latest2);
    tcase_add_test(tc, test_filter_latest_archs);
    tcase_add_test(tc, test_filter_reponames);
    suite_add_tcase(s, tc);

    tc = tcase_create("Filelists");
    tcase_add_unchecked_fixture(tc, setup_yum, teardown);
    tcase_add_test(tc, test_filter_files);
    suite_add_tcase(s, tc);

    return s;
}
