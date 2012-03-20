#include <check.h>

// libsolv
#include <solv/testcase.h>

// hawkey
#include "src/goal.h"
#include "src/query.h"
#include "testsys.h"
#include "test_goal.h"

Package
get_latest_pkg(Sack sack, const char *name)
{
    Query q = query_create(sack);
    query_filter(q, KN_PKG_NAME, FT_EQ, name);
    query_filter(q, KN_PKG_REPO, FT_LT|FT_GT, SYSTEM_REPO_NAME);
    query_filter_latest(q, 1);
    PackageList plist = query_run(q);
    fail_unless(packagelist_count(plist) == 1);
    Package pkg = package_create(sack_pool(sack),
				 package_id(packagelist_get(plist, 0)));
    query_free(q);
    packagelist_free(plist);
    return pkg;
}

int
size_and_free(PackageList plist)
{
    int c = packagelist_count(plist);
    packagelist_free(plist);
    return c;
}

START_TEST(test_goal_sanity)
{
    Goal goal = goal_create(test_globals.sack);
    fail_if(goal == NULL);
    fail_unless(sack_pool(test_globals.sack)->nsolvables ==
		TEST_EXPECT_SYSTEM_NSOLVABLES +
		TEST_EXPECT_MAIN_NSOLVABLES +
		TEST_EXPECT_UPDATES_NSOLVABLES);
    goal_free(goal);
}
END_TEST

START_TEST(test_goal_update_impossible)
{
    Package pkg = get_latest_pkg(test_globals.sack, "walrus");
    fail_if(pkg == NULL);

    Goal goal = goal_create(test_globals.sack);
    // can not try an update, walrus is not installed:
    fail_unless(goal_update(goal, pkg));
    package_free(pkg);
    goal_free(goal);
}
END_TEST

START_TEST(test_goal_install)
{
    Package pkg = get_latest_pkg(test_globals.sack, "walrus");
    Goal goal = goal_create(test_globals.sack);
    fail_if(goal_install(goal, pkg));
    package_free(pkg);
    fail_if(goal_go(goal));
    fail_unless(size_and_free(goal_list_erasures(goal)) == 0);
    fail_unless(size_and_free(goal_list_upgrades(goal)) == 0);
    fail_unless(size_and_free(goal_list_installs(goal)) == 1);
    goal_free(goal);
}
END_TEST

START_TEST(test_goal_upgrade)
{
    Package pkg = get_latest_pkg(test_globals.sack, "fool");
    Goal goal = goal_create(test_globals.sack);
    fail_if(goal_update(goal, pkg));
    package_free(pkg);
    fail_if(goal_go(goal));
    fail_unless(size_and_free(goal_list_erasures(goal)) == 0);
    fail_unless(size_and_free(goal_list_upgrades(goal)) == 1);
    fail_unless(size_and_free(goal_list_installs(goal)) == 0);
    goal_free(goal);
}
END_TEST

Suite *
goal_suite(void)
{
    Suite *s = suite_create("Goal");
    TCase *tc;

    tc = tcase_create("Core");
    tcase_add_unchecked_fixture(tc, setup_all, teardown);
    tcase_add_test(tc, test_goal_sanity);
    tcase_add_test(tc, test_goal_update_impossible);
    tcase_add_test(tc, test_goal_install);
    tcase_add_test(tc, test_goal_upgrade);
    suite_add_tcase(s, tc);

    return s;
}
