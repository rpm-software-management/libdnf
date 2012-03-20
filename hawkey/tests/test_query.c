#include <check.h>

// libsolv
#include <solv/testcase.h>

// hawkey
#include "src/query.h"
#include "src/sack_internal.h"
#include "testsys.h"
#include "test_query.h"

static int
count_results(HyQuery q)
{
    HyPackageList plist = hy_query_run(q);
    int ret = hy_packagelist_count(plist);
    hy_packagelist_free(plist);
    return ret;
}

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

START_TEST(test_query_repo)
{
    HyQuery q;

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, KN_PKG_REPO, FT_EQ, SYSTEM_REPO_NAME);

    fail_unless(count_results(q) == TEST_EXPECT_SYSTEM_NSOLVABLES - 2);

    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, KN_PKG_REPO, FT_NEQ, SYSTEM_REPO_NAME);
    fail_if(count_results(q));

    hy_query_free(q);
}
END_TEST

START_TEST(test_query_name)
{
    HyQuery q;

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, KN_PKG_NAME, FT_SUBSTR, "penny");
    fail_unless(count_results(q) == 2);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, KN_PKG_NAME, FT_EQ, "lane");
    fail_if(count_results(q));
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_anded)
{
    HyQuery q;

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, KN_PKG_NAME, FT_SUBSTR, "penny");
    hy_query_filter(q, KN_PKG_SUMMARY, FT_SUBSTR, "ears");
    fail_unless(count_results(q) == 1);
    hy_query_free(q);
}
END_TEST

START_TEST(test_updates_sanity)
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

START_TEST(test_updates)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter_updates(q, 1);
    fail_unless(count_results(q) == TEST_EXPECT_UPDATES_NSOLVABLES);
    hy_query_free(q);
}
END_TEST

START_TEST(test_filter_latest)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, KN_PKG_NAME, FT_EQ, "fool");
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
    hy_query_filter(q, KN_PKG_NAME, FT_EQ, "flying");
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

Suite *
query_suite(void)
{
    Suite *s = suite_create("HyQuery");
    TCase *tc;

    tc = tcase_create("Core");
    tcase_add_unchecked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_query_sanity);
    tcase_add_test(tc, test_query_repo);
    tcase_add_test(tc, test_query_name);
    tcase_add_test(tc, test_query_anded);
    suite_add_tcase(s, tc);

    tc = tcase_create("Updates");
    tcase_add_unchecked_fixture(tc, setup_with_updates, teardown);
    tcase_add_test(tc, test_updates_sanity);
    tcase_add_test(tc, test_updates);
    tcase_add_test(tc, test_filter_latest);
    suite_add_tcase(s, tc);

    tc = tcase_create("Full");
    tcase_add_unchecked_fixture(tc, setup_all, teardown);
    tcase_add_test(tc, test_filter_latest2);
    suite_add_tcase(s, tc);

    return s;
}
