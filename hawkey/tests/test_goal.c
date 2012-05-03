#include <check.h>

// libsolv
#include <solv/testcase.h>

// hawkey
#include "src/goal.h"
#include "src/query.h"
#include "src/package_internal.h"
#include "src/sack_internal.h"
#include "testsys.h"
#include "test_goal.h"

HyPackage
get_latest_pkg(HySack sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPO, HY_NEQ, HY_SYSTEM_REPO_NAME);
    hy_query_filter_latest(q, 1);
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 1);
    HyPackage pkg = package_create(sack_pool(sack),
				 package_id(hy_packagelist_get(plist, 0)));
    hy_query_free(q);
    hy_packagelist_free(plist);
    return pkg;
}

static int
size_and_free(HyPackageList plist)
{
    int c = hy_packagelist_count(plist);
    hy_packagelist_free(plist);
    return c;
}

START_TEST(test_goal_sanity)
{
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(goal == NULL);
    fail_unless(sack_pool(test_globals.sack)->nsolvables ==
		TEST_EXPECT_SYSTEM_NSOLVABLES +
		TEST_EXPECT_MAIN_NSOLVABLES +
		TEST_EXPECT_UPDATES_NSOLVABLES);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_update_impossible)
{
    HyPackage pkg = get_latest_pkg(test_globals.sack, "walrus");
    fail_if(pkg == NULL);

    HyGoal goal = hy_goal_create(test_globals.sack);
    // can not try an update, walrus is not installed:
    fail_unless(hy_goal_update(goal, pkg));
    hy_package_free(pkg);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_install)
{
    HyPackage pkg = get_latest_pkg(test_globals.sack, "walrus");
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_install(goal, pkg));
    hy_package_free(pkg);
    fail_if(hy_goal_go(goal));
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 1);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_update)
{
    HyPackage pkg = get_latest_pkg(test_globals.sack, "fool");
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_update(goal, pkg));
    hy_package_free(pkg);
    fail_if(hy_goal_go(goal));
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 1);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 1);
    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 0);
    hy_goal_free(goal);
}
END_TEST


START_TEST(test_goal_upgrade_all)
{
    HyGoal goal = hy_goal_create(test_globals.sack);
    hy_goal_upgrade_all(goal);
    fail_if(hy_goal_go(goal));

    HyPackageList plist = hy_goal_list_erasures(goal);
    fail_unless(hy_packagelist_count(plist) == 1);
    HyPackage pkg = hy_packagelist_get(plist, 0);
    fail_if(strcmp(hy_package_get_name(pkg), "penny"));
    hy_packagelist_free(plist);

    plist = hy_goal_list_upgrades(goal);
    fail_unless(hy_packagelist_count(plist) == 2);
    pkg = hy_packagelist_get(plist, 0);
    fail_if(strcmp(hy_package_get_name(pkg), "fool"));
    pkg = hy_packagelist_get(plist, 1);
    fail_if(strcmp(hy_package_get_name(pkg), "flying"));
    hy_packagelist_free(plist);

    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_installonly)
{
    const char *installonly[] = {"fool", NULL};

    HySack sack = test_globals.sack;
    hy_sack_set_installonly(sack, installonly);
    HyPackage pkg = get_latest_pkg(sack, "fool");
    HyGoal goal = hy_goal_create(sack);
    fail_if(hy_goal_update(goal, pkg));
    hy_package_free(pkg);
    fail_if(hy_goal_go(goal));
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 1);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 1);
    hy_goal_free(goal);
}
END_TEST

Suite *
goal_suite(void)
{
    Suite *s = suite_create("HyGoal");
    TCase *tc;

    tc = tcase_create("Core");
    tcase_add_unchecked_fixture(tc, setup_all, teardown);
    tcase_add_test(tc, test_goal_sanity);
    tcase_add_test(tc, test_goal_update_impossible);
    tcase_add_test(tc, test_goal_install);
    tcase_add_test(tc, test_goal_update);
    tcase_add_test(tc, test_goal_upgrade_all);
    tcase_add_test(tc, test_goal_installonly);
    suite_add_tcase(s, tc);

    return s;
}
