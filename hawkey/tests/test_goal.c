#include <check.h>

// libsolv
#include <solv/testcase.h>

// hawkey
#include "src/errno.h"
#include "src/goal.h"
#include "src/iutil.h"
#include "src/package_internal.h"
#include "src/packageset.h"
#include "src/repo.h"
#include "src/query.h"
#include "src/sack_internal.h"
#include "src/selector.h"
#include "src/util.h"
#include "fixtures.h"
#include "testsys.h"
#include "test_goal.h"

static HyPackage
get_latest_pkg(HySack sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
    hy_query_filter_latest(q, 1);
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 1);
    HyPackage pkg = hy_packagelist_get_clone(plist, 0);
    hy_query_free(q);
    hy_packagelist_free(plist);
    return pkg;
}

static HyPackage
get_installed_pkg(HySack sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 1);
    HyPackage pkg = hy_packagelist_get_clone(plist, 0);
    hy_query_free(q);
    hy_packagelist_free(plist);
    return pkg;
}

static HyPackage
get_available_pkg(HySack sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 1);
    HyPackage pkg = hy_packagelist_get_clone(plist, 0);
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
    fail_unless(hy_sack_count(test_globals.sack) ==
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
    fail_unless(hy_goal_upgrade_to_flags(goal, pkg, HY_CHECK_INSTALLED));
    hy_package_free(pkg);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_list_err)
{
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_unless(hy_goal_list_installs(goal) == NULL);
    fail_unless(hy_get_errno() == HY_E_OP);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_install)
{
    HyPackage pkg = get_latest_pkg(test_globals.sack, "walrus");
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_install(goal, pkg));
    hy_package_free(pkg);
    fail_if(hy_goal_run(goal));
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 2);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_install_multilib)
{
    // Tests installation of multilib package. The package is selected via
    // install query, allowing the depsolver maximum influence on the selection.

    HySelector sltr = hy_selector_create(test_globals.sack);
    HyGoal goal = hy_goal_create(test_globals.sack);

    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "semolina");
    fail_if(hy_goal_install_selector(goal, sltr));
    fail_if(hy_goal_run(goal));
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 1);
    hy_selector_free(sltr);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_install_selector)
{
    HySelector sltr;
    HyGoal goal = hy_goal_create(test_globals.sack);

    // test arch forcing
    sltr = hy_selector_create(test_globals.sack);
    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "semolina");
    hy_selector_set(sltr, HY_PKG_ARCH, HY_EQ, "i686");
    fail_if(hy_goal_install_selector(goal, sltr));
    hy_selector_free(sltr);

    fail_if(hy_goal_run(goal));
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 0);

    HyPackageList plist = hy_goal_list_installs(goal);
    fail_unless(hy_packagelist_count(plist), 1);
    char *nvra = hy_package_get_nevra(hy_packagelist_get(plist, 0));
    ck_assert_str_eq(nvra, "semolina-2-0.i686");
    hy_free(nvra);
    hy_packagelist_free(plist);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_install_selector_err)
{
    // Test that using the hy_goal_*_selector() methods returns an error for
    // selectors invalid in this context.

    HySelector sltr;
    HyGoal goal = hy_goal_create(test_globals.sack);

    sltr = hy_selector_create(test_globals.sack);
    hy_selector_set(sltr, HY_PKG_ARCH, HY_EQ, "i586");
    fail_unless(hy_goal_install_selector(goal, sltr));
    fail_unless(hy_get_errno() == HY_E_SELECTOR);
    hy_selector_free(sltr);

    sltr = hy_selector_create(test_globals.sack);
    hy_selector_set(sltr, HY_PKG_NAME, HY_GT, "semolina");
    fail_unless(hy_goal_erase_selector(goal, sltr));
    fail_unless(hy_get_errno() == HY_E_SELECTOR);
    hy_selector_free(sltr);

    sltr = hy_selector_create(test_globals.sack);
    fail_unless(hy_selector_set(sltr, HY_REPO_NAME, HY_EQ, HY_SYSTEM_REPO_NAME));
    hy_selector_free(sltr);

    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_install_selector_two)
{
    // check that we can add and resolve two selector installs to the Goal
    HySelector sltr;
    HyGoal goal = hy_goal_create(test_globals.sack);

    sltr = hy_selector_create(test_globals.sack);
    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "semolina");
    fail_if(hy_goal_install_selector(goal, sltr));
    hy_selector_free(sltr);

    sltr = hy_selector_create(test_globals.sack);
    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "fool");
    fail_if(hy_goal_upgrade_selector(goal, sltr));
    hy_selector_free(sltr);

    fail_if(hy_goal_run(goal));
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 1);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 1);
    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 1);

    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_install_selector_nomatch)
{
    HySelector sltr = hy_selector_create(test_globals.sack);
    HyGoal goal = hy_goal_create(test_globals.sack);

    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "crabalocker");
    fail_if(hy_goal_install_selector(goal, sltr));
    hy_selector_free(sltr);

    fail_if(hy_goal_run(goal));
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_selector_glob)
{
    HySelector sltr = hy_selector_create(test_globals.sack);
    HyGoal goal = hy_goal_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_GLOB, "*emolin*"));
    fail_if(hy_goal_install_selector(goal, sltr));
    fail_if(hy_goal_run(goal));
    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 1);

    hy_goal_free(goal);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_goal_selector_upgrade)
{
    HySelector sltr = hy_selector_create(test_globals.sack);
    HyGoal goal = hy_goal_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "pilchard"));
    fail_if(hy_selector_set(sltr, HY_PKG_EVR, HY_EQ, "1.2.4-2"));
    fail_if(hy_goal_upgrade_to_selector(goal, sltr));
    fail_if(hy_goal_run(goal));
    HyPackageList plist = hy_goal_list_upgrades(goal);
    fail_unless(hy_packagelist_count(plist) == 1);
    assert_nevra_eq(hy_packagelist_get(plist, 0), "pilchard-1.2.4-2.x86_64");
    hy_packagelist_free(plist);
    hy_goal_free(goal);
    hy_selector_free(sltr);

    sltr = hy_selector_create(test_globals.sack);
    goal = hy_goal_create(test_globals.sack);
    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "pilchard"));
    fail_if(hy_goal_upgrade_to_selector(goal, sltr));
    fail_if(hy_goal_run(goal));
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 2);
    hy_goal_free(goal);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_goal_selector_upgrade_provides)
{
    HySelector sltr = hy_selector_create(test_globals.sack);
    HyGoal goal = hy_goal_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_PROVIDES, HY_EQ, "fool"));
    fail_if(hy_goal_upgrade_selector(goal, sltr));
    hy_selector_free(sltr);

    fail_if(hy_goal_run(goal));
    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 1);
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 1);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_upgrade)
{
    HyPackage pkg = get_latest_pkg(test_globals.sack, "fool");
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_upgrade_to_flags(goal, pkg, HY_CHECK_INSTALLED));
    hy_package_free(pkg);
    fail_if(hy_goal_run(goal));
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
    fail_if(hy_goal_run(goal));

    HyPackageList plist = hy_goal_list_erasures(goal);
    fail_unless(hy_packagelist_count(plist) == 1);
    HyPackage pkg = hy_packagelist_get(plist, 0);
    fail_if(strcmp(hy_package_get_name(pkg), "penny"));
    hy_packagelist_free(plist);

    plist = hy_goal_list_upgrades(goal);
    fail_unless(hy_packagelist_count(plist) == 4);
    pkg = hy_packagelist_get(plist, 0);
    ck_assert_str_eq(hy_package_get_name(pkg), "fool");
    pkg = hy_packagelist_get(plist, 1);
    ck_assert_str_eq(hy_package_get_name(pkg), "flying");
    pkg = hy_packagelist_get(plist, 2);
    ck_assert_str_eq(hy_package_get_name(pkg), "pilchard");
    pkg = hy_packagelist_get(plist, 3);
    ck_assert_str_eq(hy_package_get_name(pkg), "pilchard");

    hy_packagelist_free(plist);

    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_upgrade_all_excludes)
{
    HySack sack = test_globals.sack;
    HyQuery q = hy_query_create_flags(sack, HY_IGNORE_EXCLUDES);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "pilchard");

    HyPackageSet pset = hy_query_run_set(q);
    hy_sack_add_excludes(sack, pset);
    hy_packageset_free(pset);
    hy_query_free(q);

    HyGoal goal = hy_goal_create(sack);
    hy_goal_upgrade_all(goal);
    hy_goal_run(goal);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 2);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_downgrade)
{
    HySack sack = test_globals.sack;
    HyPackage to_be_pkg = get_available_pkg(sack, "baby");
    HyGoal goal = hy_goal_create(sack);

    hy_goal_downgrade_to(goal, to_be_pkg);
    fail_if(hy_goal_run(goal));

    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 0);

    HyPackageList plist = hy_goal_list_downgrades(goal);
    fail_unless(hy_packagelist_count(plist) == 1);

    HyPackage pkg = hy_packagelist_get(plist, 0);
    ck_assert_str_eq(hy_package_get_evr(pkg),
		     "6:4.9-3");
    HyPackage old_pkg = hy_goal_package_obsoletes(goal, pkg);
    ck_assert_str_eq(hy_package_get_evr(old_pkg),
		     "6:5.0-11");
    hy_package_free(old_pkg);
    hy_packagelist_free(plist);

    hy_goal_free(goal);
    hy_package_free(to_be_pkg);
}
END_TEST

START_TEST(test_goal_get_reason)
{
    HyPackage pkg = get_latest_pkg(test_globals.sack, "walrus");
    HyGoal goal = hy_goal_create(test_globals.sack);
    hy_goal_install(goal, pkg);
    hy_package_free(pkg);
    hy_goal_run(goal);

    HyPackageList plist = hy_goal_list_installs(goal);
    int i;
    int set = 0;
    FOR_PACKAGELIST(pkg, plist, i) {
	if (!strcmp(hy_package_get_name(pkg), "walrus")) {
	    set |= 1 << 0;
	    fail_unless(hy_goal_get_reason(goal, pkg) == HY_REASON_USER);
	}
	if (!strcmp(hy_package_get_name(pkg), "semolina")) {
	    set |= 1 << 1;
	    fail_unless(hy_goal_get_reason(goal, pkg) == HY_REASON_DEP);
	}
    }
    fail_unless(set == 3);

    hy_packagelist_free(plist);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_describe_problem)
{
    HySack sack = test_globals.sack;
    HyPackage pkg = get_latest_pkg(sack, "hello");
    HyGoal goal = hy_goal_create(sack);

    hy_goal_install(goal, pkg);
    fail_unless(hy_goal_run(goal));
    fail_unless(hy_goal_list_installs(goal) == NULL);
    fail_unless(hy_get_errno() == HY_E_NO_SOLUTION);
    fail_unless(hy_goal_count_problems(goal) > 0);

    char *problem = hy_goal_describe_problem(goal, 0);
    const char *expected = "nothing provides goodbye";
    fail_if(strncmp(problem, expected, strlen(expected)));
    hy_free(problem);

    hy_package_free(pkg);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_log_decisions)
{
    HySack sack = test_globals.sack;
    HyPackage pkg = get_latest_pkg(sack, "hello");
    HyGoal goal = hy_goal_create(sack);

    hy_goal_install(goal, pkg);
    HY_LOG_INFO("--- decisions below --->");
    const int origsize = logfile_size(sack);
    hy_goal_run(goal);
    hy_goal_log_decisions(goal);
    const int newsize = logfile_size(sack);
    // check something substantial was added to the logfile:
    fail_unless(newsize - origsize > 3000);

    hy_package_free(pkg);
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
    fail_if(hy_goal_upgrade_to_flags(goal, pkg, HY_CHECK_INSTALLED));
    hy_package_free(pkg);
    fail_if(hy_goal_run(goal));
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 1);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 1);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_no_reinstall)
{
    HySack sack = test_globals.sack;
    HyPackage pkg = get_latest_pkg(sack, "penny");
    HyGoal goal = hy_goal_create(sack);
    fail_if(hy_goal_install(goal, pkg));
    hy_package_free(pkg);
    fail_if(hy_goal_run(goal));
    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_erase_simple)
{
    HySack sack = test_globals.sack;
    HyPackage pkg = get_installed_pkg(sack, "penny");
    HyGoal goal = hy_goal_create(sack);
    fail_if(hy_goal_erase(goal, pkg));
    hy_package_free(pkg);
    fail_if(hy_goal_run(goal));
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 1);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_erase_with_deps)
{
    HySack sack = test_globals.sack;
    HyPackage pkg = get_installed_pkg(sack, "penny-lib");

    // by default can not remove penny-lib, flying depends on it:
    HyGoal goal = hy_goal_create(sack);
    hy_goal_erase(goal, pkg);
    fail_unless(hy_goal_run(goal));
    hy_goal_free(goal);

    goal = hy_goal_create(sack);
    hy_goal_erase(goal, pkg);
    fail_if(hy_goal_run_flags(goal, HY_ALLOW_UNINSTALL));
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 2);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 0);
    hy_goal_free(goal);

    hy_package_free(pkg);
}
END_TEST

START_TEST(test_goal_erase_clean_deps)
{
    HySack sack = test_globals.sack;
    HyPackage pkg = get_installed_pkg(sack, "flying");

    // by default, leave dependencies alone:
    HyGoal goal = hy_goal_create(sack);
    hy_goal_erase(goal, pkg);
    hy_goal_run(goal);
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 1);
    hy_goal_free(goal);

    // allow deleting dependencies:
    goal = hy_goal_create(sack);
    hy_goal_erase_flags(goal, pkg, HY_CLEAN_DEPS);
    fail_unless(hy_goal_run(goal) == 0);
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 2);
    hy_goal_free(goal);

    // test userinstalled specification:
    HyPackage penny_pkg = get_installed_pkg(sack, "penny-lib");
    goal = hy_goal_create(sack);
    hy_goal_erase_flags(goal, pkg, HY_CLEAN_DEPS);
    hy_goal_userinstalled(goal, penny_pkg);
    // having the same solvable twice in a goal shouldn't break anything:
    hy_goal_userinstalled(goal, pkg);
    fail_unless(hy_goal_run(goal) == 0);
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 1);
    hy_goal_free(goal);
    hy_package_free(penny_pkg);

    hy_package_free(pkg);
}
END_TEST

START_TEST(test_goal_distupgrade_all)
{
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_distupgrade_all(goal));
    fail_if(hy_goal_run(goal));

    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 0);
    fail_unless(size_and_free(hy_goal_list_erasures(goal)) == 0);

    HyPackageList plist = hy_goal_list_upgrades(goal);
    fail_unless(hy_packagelist_count(plist) == 1);
    assert_nevra_eq(hy_packagelist_get(plist, 0), "flying-3-0.noarch");
    hy_packagelist_free(plist);

    plist = hy_goal_list_downgrades(goal);
    fail_unless(hy_packagelist_count(plist) == 1);
    assert_nevra_eq(hy_packagelist_get(plist, 0), "baby-6:4.9-3.x86_64");
    hy_packagelist_free(plist);
    hy_goal_free(goal);
}
END_TEST

struct Solutions {
    int solutions;
    HyPackageList installs;
};

static struct Solutions *
solutions_create(void)
{
    struct Solutions *solutions = solv_calloc(1, sizeof(struct Solutions));
    solutions->installs = hy_packagelist_create();
    return solutions;
}

static void
solutions_free(struct Solutions *solutions)
{
    hy_packagelist_free(solutions->installs);
    solv_free(solutions);
}

static int
solution_cb(HyGoal goal, void *data)
{
    struct Solutions *solutions = (struct Solutions*)data;
    solutions->solutions++;

    HyPackageList new_installs = hy_goal_list_installs(goal);
    HyPackage pkg;
    int i;

    FOR_PACKAGELIST(pkg, new_installs, i)
	if (!hy_packagelist_has(solutions->installs, pkg))
	    hy_packagelist_push(solutions->installs, hy_package_link(pkg));
    hy_packagelist_free(new_installs);

    return 0;
}

START_TEST(test_goal_run_all)
{
    HySack sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);
    HyPackage pkg = get_available_pkg(sack, "A");

    fail_if(hy_goal_install(goal, pkg));

    struct Solutions *solutions = solutions_create();
    fail_if(hy_goal_run_all(goal, solution_cb, solutions));
    fail_unless(solutions->solutions == 2);
    fail_unless(hy_packagelist_count(solutions->installs) == 3);
    solutions_free(solutions);

    hy_goal_free(goal);
    hy_package_free(pkg);
}
END_TEST

START_TEST(test_goal_update_vendor)
{
    HySack sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);
    HySelector sltr = hy_selector_create(sack);

    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "fool");
    fail_if(hy_goal_upgrade_selector(goal, sltr));
    hy_selector_free(sltr);

    /* hy_goal_upgrade_all(goal); */
    fail_if(hy_goal_run(goal));

    HyPackageList plist = hy_goal_list_installs(goal);
    fail_unless(hy_packagelist_count(plist) == 1);
    hy_packagelist_free(plist);

    plist = hy_goal_list_erasures(goal);
    fail_unless(hy_packagelist_count(plist) == 1);
    hy_packagelist_free(plist);
    hy_goal_free(goal);
}
END_TEST

Suite *
goal_suite(void)
{
    Suite *s = suite_create("Goal");
    TCase *tc;

    tc = tcase_create("Core");
    tcase_add_unchecked_fixture(tc, fixture_all, teardown);
    tcase_add_test(tc, test_goal_sanity);
    tcase_add_test(tc, test_goal_update_impossible);
    tcase_add_test(tc, test_goal_list_err);
    tcase_add_test(tc, test_goal_install);
    tcase_add_test(tc, test_goal_install_multilib);
    tcase_add_test(tc, test_goal_install_selector);
    tcase_add_test(tc, test_goal_install_selector_err);
    tcase_add_test(tc, test_goal_install_selector_two);
    tcase_add_test(tc, test_goal_install_selector_nomatch);
    tcase_add_test(tc, test_goal_selector_glob);
    tcase_add_test(tc, test_goal_selector_upgrade);
    tcase_add_test(tc, test_goal_selector_upgrade_provides);
    tcase_add_test(tc, test_goal_upgrade);
    tcase_add_test(tc, test_goal_upgrade_all);
    tcase_add_test(tc, test_goal_upgrade_all_excludes);
    tcase_add_test(tc, test_goal_downgrade);
    tcase_add_test(tc, test_goal_get_reason);
    tcase_add_test(tc, test_goal_describe_problem);
    tcase_add_test(tc, test_goal_log_decisions);
    tcase_add_test(tc, test_goal_installonly);
    tcase_add_test(tc, test_goal_no_reinstall);
    tcase_add_test(tc, test_goal_erase_simple);
    tcase_add_test(tc, test_goal_erase_with_deps);
    tcase_add_test(tc, test_goal_erase_clean_deps);
    suite_add_tcase(s, tc);

    tc = tcase_create("Main");
    tcase_add_unchecked_fixture(tc, fixture_with_main, teardown);
    tcase_add_test(tc, test_goal_distupgrade_all);
    suite_add_tcase(s, tc);

    tc = tcase_create("Greedy");
    tcase_add_unchecked_fixture(tc, fixture_greedy_only, teardown);
    tcase_add_test(tc, test_goal_run_all);
    suite_add_tcase(s, tc);

    tc = tcase_create("Vendor");
    tcase_add_unchecked_fixture(tc, fixture_with_vendor, teardown);
    tcase_add_test(tc, test_goal_update_vendor);
    suite_add_tcase(s, tc);

    return s;
}
