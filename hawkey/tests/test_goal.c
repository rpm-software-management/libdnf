/*
 * Copyright (C) 2012-2013 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <check.h>
#include <stdarg.h>

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
#include "test_suites.h"

static HyPackage
get_latest_pkg(HySack sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
    hy_query_filter_latest_per_arch(q, 1);
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 1,
		"get_latest_pkg() failed finding '%s'.", name);
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

/* make Sack think we are unable to determine the running kernel */
static Id
mock_running_kernel_no(HySack sack)
{
    return -1;
}

/* make Sack think k-1-1 is the running kernel */
static Id
mock_running_kernel(HySack sack)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "k");
    hy_query_filter(q, HY_PKG_EVR, HY_EQ, "1-1");
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 1);
    HyPackage pkg = hy_packagelist_get_clone(plist, 0);
    hy_query_free(q);
    hy_packagelist_free(plist);
    Id id = package_id(pkg);
    hy_package_free(pkg);
    return id;
}

static int
size_and_free(HyPackageList plist)
{
    int c = hy_packagelist_count(plist);
    hy_packagelist_free(plist);
    return c;
}

static void
userinstalled(HySack sack, HyGoal goal, const char *name)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
    HyPackageList plist = hy_query_run(q);
    HyPackage pkg;
    int i;

    FOR_PACKAGELIST(pkg, plist, i)
	hy_goal_userinstalled(goal, pkg);

    hy_packagelist_free(plist);
    hy_query_free(q);
}

/* assert on installed-upgraded-erased-obsoleted numbers */
static void
assert_iueo(HyGoal goal, int i, int u, int e, int o)
{
    ck_assert_int_eq(size_and_free(hy_goal_list_installs(goal)), i);
    ck_assert_int_eq(size_and_free(hy_goal_list_upgrades(goal)), u);
    ck_assert_int_eq(size_and_free(hy_goal_list_erasures(goal)), e);
    ck_assert_int_eq(size_and_free(hy_goal_list_obsoleted(goal)), o);
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

START_TEST(test_goal_actions)
{
    HyPackage pkg = get_latest_pkg(test_globals.sack, "walrus");
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_has_actions(goal, HY_INSTALL));
    fail_if(hy_goal_install(goal, pkg));
    fail_unless(hy_goal_has_actions(goal, HY_INSTALL));
    hy_package_free(pkg);
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
    assert_iueo(goal, 2, 0, 0, 0);
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
    assert_iueo(goal, 1, 0, 0, 0);
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
    assert_iueo(goal, 1, 0, 0, 0);

    HyPackageList plist = hy_goal_list_installs(goal);
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
    assert_iueo(goal, 1, 1, 0, 1);

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
    assert_iueo(goal, 0, 0, 0, 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_install_weak_deps)
{
    HySelector sltr = hy_selector_create(test_globals.sack);
    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "B");
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_install_selector(goal, sltr));
    HyGoal goal2 = hy_goal_clone(goal);
    fail_if(hy_goal_run(goal));
    // recommended package C is installed too
    assert_iueo(goal, 2, 0, 0, 0);

    fail_if(hy_goal_run_flags(goal2, HY_IGNORE_WEAK_DEPS));
    assert_iueo(goal2, 1, 0, 0, 0);
    hy_goal_free(goal);
    hy_goal_free(goal2);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_goal_selector_glob)
{
    HySelector sltr = hy_selector_create(test_globals.sack);
    HyGoal goal = hy_goal_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_GLOB, "*emolin*"));
    fail_if(hy_goal_install_selector(goal, sltr));
    fail_if(hy_goal_run(goal));
    assert_iueo(goal, 1, 0, 0, 0);

    hy_goal_free(goal);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_goal_selector_provides_glob)
{
    HySelector sltr = hy_selector_create(test_globals.sack);
    HyGoal goal = hy_goal_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_PROVIDES, HY_GLOB, "P*"));
    fail_if(hy_goal_erase_selector(goal, sltr));
    fail_if(hy_goal_run(goal));
    assert_iueo(goal, 0, 0, 1, 0);

    hy_goal_free(goal);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_goal_selector_upgrade)
{
    HySelector sltr = hy_selector_create(test_globals.sack);
    HyGoal goal = hy_goal_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "dog"));
    fail_if(hy_selector_set(sltr, HY_PKG_EVR, HY_EQ, "1-2"));
    fail_if(hy_goal_upgrade_to_selector(goal, sltr));
    fail_if(hy_goal_run(goal));
    HyPackageList plist = hy_goal_list_upgrades(goal);
    fail_unless(hy_packagelist_count(plist) == 1);
    assert_nevra_eq(hy_packagelist_get(plist, 0), "dog-1-2.x86_64");
    hy_packagelist_free(plist);
    hy_goal_free(goal);
    hy_selector_free(sltr);

    sltr = hy_selector_create(test_globals.sack);
    goal = hy_goal_create(test_globals.sack);
    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "pilchard"));
    fail_if(hy_goal_upgrade_to_selector(goal, sltr));
    fail_if(hy_goal_run(goal));
    assert_iueo(goal, 0, 2, 0, 0);
    hy_goal_free(goal);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_goal_selector_upgrade_provides)
{
    HySack sack = test_globals.sack;
    HySelector sltr = hy_selector_create(sack);
    HyGoal goal = hy_goal_create(sack);

    fail_if(hy_selector_set(sltr, HY_PKG_PROVIDES, HY_EQ, "fool"));
    fail_if(hy_goal_upgrade_selector(goal, sltr));
    hy_selector_free(sltr);

    fail_if(hy_goal_run(goal));
    assert_iueo(goal, 0, 1, 0, 1);
    hy_goal_free(goal);

    sltr = hy_selector_create(sack);
    goal = hy_goal_create(sack);
    fail_if(hy_selector_set(sltr, HY_PKG_PROVIDES, HY_EQ, "fool > 1-3"));
    fail_if(hy_goal_upgrade_selector(goal, sltr));
    hy_selector_free(sltr);

    fail_if(hy_goal_run(goal));
    assert_iueo(goal, 0, 1, 0, 1);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_install_selector_file)
{
    HySack sack = test_globals.sack;
    HySelector sltr = hy_selector_create(sack);
    HyGoal goal = hy_goal_create(sack);
    fail_if(hy_selector_set(sltr, HY_PKG_FILE, HY_EQ|HY_GLOB, "/*/answers"));
    fail_if(hy_goal_erase_selector(goal, sltr));
    fail_if(hy_goal_run(goal));
    assert_iueo(goal, 0, 0, 1, 0);
    HyPackageList plist = hy_goal_list_erasures(goal);
    HyPackage pkg = hy_packagelist_get(plist, 0);
    ck_assert_str_eq("fool", hy_package_get_name(pkg));
    hy_selector_free(sltr);
    hy_packagelist_free(plist);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_install_optional)
{
    HySelector sltr;
    HyGoal goal = hy_goal_create(test_globals.sack);

    // test optional selector installation
    sltr = hy_selector_create(test_globals.sack);
    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "hello");
    fail_if(hy_goal_install_selector_optional(goal, sltr));
    fail_if(hy_goal_run(goal));
    hy_selector_free(sltr);
    assert_iueo(goal, 0, 0, 0, 0);

    // test optional package installation
    HyPackage pkg = get_latest_pkg(test_globals.sack, "hello");
    fail_if(hy_goal_install_optional(goal, pkg));
    fail_if(hy_goal_run(goal));
    assert_iueo(goal, 0, 0, 0, 0);
    hy_package_free(pkg);
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
    assert_iueo(goal, 0, 1, 0, 1);
    hy_goal_free(goal);
}
END_TEST

static void
assert_list_names(HyPackageList plist, ...)
{
    va_list names;
    char *name;
    int count = hy_packagelist_count(plist), i = 0;

    va_start(names, plist);
    while ((name = va_arg(names, char *)) != NULL) {
	if (i >= count)
	    fail("assert_list_names(): list too short");
	HyPackage pkg = hy_packagelist_get(plist, i++);
	ck_assert_str_eq(hy_package_get_name(pkg), name);
    }
    fail_unless(i == count, "assert_list_names(): too many items in the list");
    va_end(names);
}

START_TEST(test_goal_upgrade_all)
{
    HyGoal goal = hy_goal_create(test_globals.sack);
    hy_goal_upgrade_all(goal);
    fail_if(hy_goal_run(goal));

    HyPackageList plist = hy_goal_list_erasures(goal);
    fail_unless(size_and_free(plist) == 0);

    plist = hy_goal_list_obsoleted(goal);
    assert_list_names(plist, "penny", NULL);
    hy_packagelist_free(plist);

    plist = hy_goal_list_upgrades(goal);
    assert_list_names(plist, "dog", "flying", "fool", "pilchard", "pilchard",
		      NULL);

    // see all obsoletes of fool:
    HyPackage pkg = hy_packagelist_get(plist, 2);
    HyPackageList plist_obs = hy_goal_list_obsoleted_by_package(goal, pkg);
    assert_list_names(plist_obs, "fool", "penny", NULL);
    hy_packagelist_free(plist_obs);
    hy_packagelist_free(plist);

    fail_unless(size_and_free(hy_goal_list_installs(goal)) == 0);
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
    assert_iueo(goal, 0, 0, 0, 0);

    HyPackageList plist = hy_goal_list_downgrades(goal);
    fail_unless(hy_packagelist_count(plist) == 1);

    HyPackage pkg = hy_packagelist_get(plist, 0);
    ck_assert_str_eq(hy_package_get_evr(pkg),
		     "6:4.9-3");
    HyPackageList obsoleted = hy_goal_list_obsoleted_by_package(goal, pkg);
    fail_unless(hy_packagelist_count(obsoleted) == 1);
    HyPackage old_pkg = hy_packagelist_get(obsoleted, 0);
    ck_assert_str_eq(hy_package_get_evr(old_pkg),
		     "6:5.0-11");
    hy_packagelist_free(obsoleted);
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

START_TEST(test_goal_get_reason_selector)
{

    HySelector sltr = hy_selector_create(test_globals.sack);
    HyGoal goal = hy_goal_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "walrus"));
    fail_if(hy_goal_install_selector(goal, sltr));
    hy_selector_free(sltr);

    fail_if(hy_goal_run(goal));

    HyPackageList plist = hy_goal_list_installs(goal);
    fail_unless(hy_packagelist_count(plist) == 2);
    HyPackage pkg = hy_packagelist_get(plist, 0);
    fail_unless(hy_goal_get_reason(goal, pkg) == HY_REASON_USER);

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

START_TEST(test_goal_no_reinstall)
{
    HySack sack = test_globals.sack;
    HyPackage pkg = get_latest_pkg(sack, "penny");
    HyGoal goal = hy_goal_create(sack);
    fail_if(hy_goal_install(goal, pkg));
    hy_package_free(pkg);
    fail_if(hy_goal_run(goal));
    assert_iueo(goal, 0, 0, 0, 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_erase_simple)
{
    HySack sack = test_globals.sack;
    HyPackage pkg = by_name_repo(sack, "penny", HY_SYSTEM_REPO_NAME);
    HyGoal goal = hy_goal_create(sack);
    fail_if(hy_goal_erase(goal, pkg));
    hy_package_free(pkg);
    fail_if(hy_goal_run(goal));
    assert_iueo(goal, 0, 0, 1, 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_erase_with_deps)
{
    HySack sack = test_globals.sack;
    HyPackage pkg = by_name_repo(sack, "penny-lib", HY_SYSTEM_REPO_NAME);

    // by default can not remove penny-lib, flying depends on it:
    HyGoal goal = hy_goal_create(sack);
    hy_goal_erase(goal, pkg);
    fail_unless(hy_goal_run(goal));
    hy_goal_free(goal);

    goal = hy_goal_create(sack);
    hy_goal_erase(goal, pkg);
    fail_if(hy_goal_run_flags(goal, HY_ALLOW_UNINSTALL));
    assert_iueo(goal, 0, 0, 2, 0);
    hy_goal_free(goal);

    hy_package_free(pkg);
}
END_TEST

START_TEST(test_goal_erase_clean_deps)
{
    HySack sack = test_globals.sack;
    HyPackage pkg = by_name_repo(sack, "flying", HY_SYSTEM_REPO_NAME);

    // by default, leave dependencies alone:
    HyGoal goal = hy_goal_create(sack);
    hy_goal_erase(goal, pkg);
    hy_goal_run(goal);
    assert_iueo(goal, 0, 0, 1, 0);
    hy_goal_free(goal);

    // allow deleting dependencies:
    goal = hy_goal_create(sack);
    hy_goal_erase_flags(goal, pkg, HY_CLEAN_DEPS);
    fail_unless(hy_goal_run(goal) == 0);
    assert_iueo(goal, 0, 0, 2, 0);
    hy_goal_free(goal);

    // test userinstalled specification:
    HyPackage penny_pkg = by_name_repo(sack, "penny-lib", HY_SYSTEM_REPO_NAME);
    goal = hy_goal_create(sack);
    hy_goal_erase_flags(goal, pkg, HY_CLEAN_DEPS);
    hy_goal_userinstalled(goal, penny_pkg);
    // having the same solvable twice in a goal shouldn't break anything:
    hy_goal_userinstalled(goal, pkg);
    fail_unless(hy_goal_run(goal) == 0);
    assert_iueo(goal, 0, 0, 1, 0);
    hy_goal_free(goal);
    hy_package_free(penny_pkg);

    hy_package_free(pkg);
}
END_TEST

START_TEST(test_goal_forcebest)
{
    HySack sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);
    HySelector sltr = hy_selector_create(sack);

    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "flying");
    hy_goal_upgrade_selector(goal, sltr);
    fail_unless(hy_goal_run_flags(goal, HY_FORCE_BEST));
    fail_unless(hy_goal_count_problems(goal) == 1);

    hy_selector_free(sltr);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_verify)
{
    HySack sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);

    fail_unless(hy_goal_run_flags(goal, HY_VERIFY));
    fail_unless(hy_goal_list_installs(goal) == NULL);
    fail_unless(hy_get_errno() == HY_E_NO_SOLUTION);
    fail_unless(hy_goal_count_problems(goal) == 2);

    char *expected;
    char *problem;
    problem = hy_goal_describe_problem(goal, 0);
    expected = "nothing provides missing-dep needed by missing-1-0.x86_64";
    fail_if(strncmp(problem, expected, strlen(expected)));
    hy_free(problem);
    problem = hy_goal_describe_problem(goal, 1);
    expected = "package conflict-1-0.x86_64 conflicts with ok provided by ok-1-0.x86_64";
    fail_if(strncmp(problem, expected, strlen(expected)));
    hy_free(problem);

    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_installonly)
{
    const char *installonly[] = {"fool", NULL};

    HySack sack = test_globals.sack;
    hy_sack_set_installonly(sack, installonly);
    hy_sack_set_installonly_limit(sack, 2);
    HyPackage pkg = get_latest_pkg(sack, "fool");
    HyGoal goal = hy_goal_create(sack);
    fail_if(hy_goal_upgrade_to_flags(goal, pkg, HY_CHECK_INSTALLED));
    hy_package_free(pkg);
    fail_if(hy_goal_run(goal));
    assert_iueo(goal, 1, 0, 1, 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_installonly_upgrade_all)
{
    const char *installonly[] = {"fool", NULL};
    HySack sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);

    hy_sack_set_installonly(sack, installonly);
    hy_sack_set_installonly_limit(sack, 2);

    hy_goal_upgrade_all(goal);
    fail_if(hy_goal_run(goal));

    HyPackageList plist = hy_goal_list_erasures(goal);
    assert_list_names(plist, "penny", NULL);
    hy_packagelist_free(plist);
    plist = hy_goal_list_installs(goal);
    assert_list_names(plist, "fool", NULL);
    hy_packagelist_free(plist);
    assert_iueo(goal, 1, 4, 1, 0);

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
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 3);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_upgrade_disabled_repo)
{
    HySack sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);

    hy_goal_upgrade_all(goal);
    hy_goal_run(goal);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal)) == 5);
    hy_goal_free(goal);

    hy_sack_repo_enabled(sack, "updates", 0);
    goal = hy_goal_create(sack);
    hy_goal_upgrade_all(goal);
    hy_goal_run(goal);
    assert_iueo(goal, 0, 1, 0, 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_describe_problem_excludes)
{
    HySack sack = test_globals.sack;

    HyQuery q = hy_query_create_flags(sack, HY_IGNORE_EXCLUDES);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "semolina");
    HyPackageSet pset = hy_query_run_set(q);
    hy_sack_add_excludes(sack, pset);
    hy_packageset_free(pset);
    hy_query_free(q);

    HyGoal goal = hy_goal_create(sack);
    HySelector sltr = hy_selector_create(sack);

    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "semolina");
    hy_goal_install_selector(goal, sltr);
    hy_selector_free(sltr);

    fail_unless(hy_goal_run(goal));
    fail_unless(hy_goal_count_problems(goal) > 0);

    char *problem = hy_goal_describe_problem(goal, 0);
    ck_assert_str_eq(problem, "package semolina does not exist");
    hy_free(problem);

    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_distupgrade_all)
{
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_distupgrade_all(goal));
    fail_if(hy_goal_run(goal));

    assert_iueo(goal, 0, 1, 0, 0);
    HyPackageList plist = hy_goal_list_upgrades(goal);
    assert_nevra_eq(hy_packagelist_get(plist, 0), "flying-3-0.noarch");
    hy_packagelist_free(plist);

    plist = hy_goal_list_downgrades(goal);
    fail_unless(hy_packagelist_count(plist) == 1);
    assert_nevra_eq(hy_packagelist_get(plist, 0), "baby-6:4.9-3.x86_64");
    hy_packagelist_free(plist);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_distupgrade_all_excludes)
{
    HyQuery q = hy_query_create_flags(test_globals.sack, HY_IGNORE_EXCLUDES);
    hy_query_filter_provides(q, HY_GT|HY_EQ, "flying", "0");
    HyPackageSet pset = hy_query_run_set(q);
    hy_sack_add_excludes(test_globals.sack, pset);
    hy_packageset_free(pset);
    hy_query_free(q);

    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_distupgrade_all(goal));
    fail_if(hy_goal_run(goal));

    assert_iueo(goal, 0, 0, 0, 0);

    HyPackageList plist = hy_goal_list_downgrades(goal);
    fail_unless(hy_packagelist_count(plist) == 1);
    assert_nevra_eq(hy_packagelist_get(plist, 0), "baby-6:4.9-3.x86_64");
    hy_packagelist_free(plist);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_distupgrade_all_keep_arch)
{
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_distupgrade_all(goal));
    fail_if(hy_goal_run(goal));

    assert_iueo(goal, 0, 5, 0, 1);
    HyPackageList plist = hy_goal_list_upgrades(goal);
    // gun pkg is not upgraded to latest version of different arch
    assert_nevra_eq(hy_packagelist_get(plist, 0), "dog-1-2.x86_64");
    assert_nevra_eq(hy_packagelist_get(plist, 1), "pilchard-1.2.4-1.i686");
    assert_nevra_eq(hy_packagelist_get(plist, 2), "pilchard-1.2.4-1.x86_64");
    assert_nevra_eq(hy_packagelist_get(plist, 3), "flying-3.1-0.x86_64");
    assert_nevra_eq(hy_packagelist_get(plist, 4), "fool-1-5.noarch");
    hy_packagelist_free(plist);

    plist = hy_goal_list_obsoleted(goal);
    fail_unless(hy_packagelist_count(plist) == 1);
    assert_nevra_eq(hy_packagelist_get(plist, 0), "penny-4-1.noarch");
    hy_packagelist_free(plist);

    plist = hy_goal_list_downgrades(goal);
    fail_unless(hy_packagelist_count(plist) == 1);
    assert_nevra_eq(hy_packagelist_get(plist, 0), "baby-6:4.9-3.x86_64");
    hy_packagelist_free(plist);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_distupgrade_selector_upgrade)
{
    HyGoal goal = hy_goal_create(test_globals.sack);
    HySelector sltr = hy_selector_create(test_globals.sack);
    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "flying");
    fail_if(hy_goal_distupgrade_selector(goal, sltr));
    fail_if(hy_goal_run(goal));

    assert_iueo(goal, 0, 1, 0, 0);
    HyPackageList plist = hy_goal_list_upgrades(goal);
    assert_nevra_eq(hy_packagelist_get(plist, 0), "flying-3-0.noarch");

    hy_packagelist_free(plist);
    hy_goal_free(goal);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_goal_distupgrade_selector_downgrade)
{
    HyGoal goal = hy_goal_create(test_globals.sack);
    HySelector sltr = hy_selector_create(test_globals.sack);
    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "baby");
    fail_if(hy_goal_distupgrade_selector(goal, sltr));
    fail_if(hy_goal_run(goal));

    assert_iueo(goal, 0, 0, 0, 0);
    HyPackageList plist = hy_goal_list_downgrades(goal);
    fail_unless(hy_packagelist_count(plist) == 1);
    assert_nevra_eq(hy_packagelist_get(plist, 0), "baby-6:4.9-3.x86_64");

    hy_packagelist_free(plist);
    hy_goal_free(goal);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_goal_distupgrade_selector_nothing)
{
    HyGoal goal = hy_goal_create(test_globals.sack);
    HySelector sltr = hy_selector_create(test_globals.sack);
    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "P-lib");
    fail_if(hy_goal_distupgrade_selector(goal, sltr));
    fail_if(hy_goal_run(goal));

    assert_iueo(goal, 0, 0, 0, 0);
    HyPackageList plist = hy_goal_list_downgrades(goal);
    fail_unless(hy_packagelist_count(plist) == 0);
    hy_packagelist_free(plist);
    hy_goal_free(goal);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_goal_rerun)
{
    HySack sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);
    HyPackage pkg = get_latest_pkg(sack, "walrus");

    hy_goal_install(goal, pkg);
    fail_if(hy_goal_run(goal));
    assert_iueo(goal, 2, 0, 0, 0);
    hy_package_free(pkg);

    // add an erase:
    pkg = by_name_repo(sack, "dog", HY_SYSTEM_REPO_NAME);
    hy_goal_erase(goal, pkg);
    fail_if(hy_goal_run(goal));
    assert_iueo(goal, 2, 0, 1, 0);
    hy_package_free(pkg);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_unneeded)
{
    HySack sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);

    userinstalled(sack, goal, "baby");
    userinstalled(sack, goal, "dog");
    userinstalled(sack, goal, "fool");
    userinstalled(sack, goal, "gun");
    userinstalled(sack, goal, "jay");
    userinstalled(sack, goal, "penny");
    userinstalled(sack, goal, "pilchard");
    hy_goal_run(goal);

    HyPackageList plist = hy_goal_list_unneeded(goal);
    ck_assert_int_eq(hy_packagelist_count(plist), 4);
    HyPackage pkg = hy_packagelist_get(plist, 0);
    assert_nevra_eq(pkg, "flying-2-9.noarch");
    pkg = hy_packagelist_get(plist, 1);
    assert_nevra_eq(pkg, "penny-lib-4-1.x86_64");
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

START_TEST(test_goal_installonly_limit)
{
    const char *installonly[] = {"k", NULL};
    HySack sack = test_globals.sack;
    hy_sack_set_installonly(sack, installonly);
    hy_sack_set_installonly_limit(sack, 3);
    sack->running_kernel_fn = mock_running_kernel_no;

    HyGoal goal = hy_goal_create(sack);
    hy_goal_upgrade_all(goal);
    fail_if(hy_goal_run_flags(goal, 0));

    assert_iueo(goal, 1, 1, 3, 0); // k-m is just upgraded
    HyPackageList erasures = hy_goal_list_erasures(goal);
    assert_nevra_eq(hy_packagelist_get(erasures, 0), "k-1-0.x86_64");
    assert_nevra_eq(hy_packagelist_get(erasures, 1), "k-freak-1-0-1-0.x86_64");
    assert_nevra_eq(hy_packagelist_get(erasures, 2), "k-1-1.x86_64");
    hy_packagelist_free(erasures);

    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_installonly_limit_disabled)
{
    // test that setting limit to 0 does not cause all intallonlies to be
    // uninstalled
    const char *installonly[] = {"k", NULL};
    HySack sack = test_globals.sack;
    hy_sack_set_installonly(sack, installonly);
    hy_sack_set_installonly_limit(sack, 0);
    sack->running_kernel_fn = mock_running_kernel_no;

    HyGoal goal = hy_goal_create(sack);
    hy_goal_upgrade_all(goal);
    fail_if(hy_goal_run_flags(goal, 0));

    assert_iueo(goal, 1, 1, 0, 0);
    hy_goal_free(goal);
}
END_TEST


START_TEST(test_goal_installonly_limit_running_kernel)
{
    const char *installonly[] = {"k", NULL};
    HySack sack = test_globals.sack;
    hy_sack_set_installonly(sack, installonly);
    hy_sack_set_installonly_limit(sack, 3);
    sack->running_kernel_fn = mock_running_kernel;

    HyGoal goal = hy_goal_create(sack);
    hy_goal_upgrade_all(goal);
    fail_if(hy_goal_run_flags(goal, 0));

    assert_iueo(goal, 1, 1, 3, 0);
    HyPackageList erasures = hy_goal_list_erasures(goal);
    assert_nevra_eq(hy_packagelist_get(erasures, 0), "k-1-0.x86_64");
    assert_nevra_eq(hy_packagelist_get(erasures, 1), "k-freak-1-0-1-0.x86_64");
    assert_nevra_eq(hy_packagelist_get(erasures, 2), "k-2-0.x86_64");
    hy_packagelist_free(erasures);

    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_installonly_limit_with_modules)
{
    // most complex installonly test case, includes the k-m packages
    const char *installonly[] = {"k", "k-m", NULL};
    HySack sack = test_globals.sack;
    hy_sack_set_installonly(sack, installonly);
    hy_sack_set_installonly_limit(sack, 3);
    sack->running_kernel_fn = mock_running_kernel;

    HyGoal goal = hy_goal_create(sack);
    hy_goal_upgrade_all(goal);
    fail_if(hy_goal_run_flags(goal, 0));

    assert_iueo(goal, 2, 0, 5, 0);
    HyPackageList erasures = hy_goal_list_erasures(goal);
    assert_nevra_eq(hy_packagelist_get(erasures, 0), "k-1-0.x86_64");
    assert_nevra_eq(hy_packagelist_get(erasures, 1), "k-m-1-0.x86_64");
    assert_nevra_eq(hy_packagelist_get(erasures, 2), "k-freak-1-0-1-0.x86_64");
    assert_nevra_eq(hy_packagelist_get(erasures, 3), "k-2-0.x86_64");
    assert_nevra_eq(hy_packagelist_get(erasures, 4), "k-m-2-0.x86_64");
    hy_packagelist_free(erasures);

    hy_goal_free(goal);
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
    assert_iueo(goal, 1, 0, 0, 1);

    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_forcebest_arches)
{
    HySack sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);
    HySelector sltr = hy_selector_create(sack);

    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "gun");
    fail_if(hy_goal_upgrade_selector(goal, sltr));
    fail_if(hy_goal_run_flags(goal, HY_FORCE_BEST));
    assert_iueo(goal, 0, 0, 0, 0);

    hy_selector_free(sltr);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_change)
{
    // test that changes are handled like reinstalls

    HySack sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);

    hy_goal_upgrade_all(goal);

    fail_if(hy_goal_run(goal));
    assert_iueo(goal, 0, 1, 0, 0);
    fail_unless(size_and_free(hy_goal_list_reinstalls(goal)) == 1);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_clone)
{
    HySack sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);

    hy_goal_upgrade_all(goal);
    HyGoal goal2 = hy_goal_clone(goal);

    fail_if(hy_goal_run(goal));
    assert_iueo(goal, 0, 1, 0, 0);
    fail_unless(size_and_free(hy_goal_list_reinstalls(goal)) == 1);
    hy_goal_free(goal);

    fail_if(hy_goal_run(goal2));
    assert_iueo(goal2, 0, 1, 0, 0);
    fail_unless(size_and_free(hy_goal_list_reinstalls(goal2)) == 1);
    hy_goal_free(goal2);
}
END_TEST

START_TEST(test_cmdline_file_provides)
{
    HySack sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);

    hy_goal_upgrade_all(goal);
    ck_assert(!hy_goal_run_flags(goal, HY_FORCE_BEST));
    assert_iueo(goal, 0, 1, 0, 0);
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
    tcase_add_test(tc, test_goal_actions);
    tcase_add_test(tc, test_goal_sanity);
    tcase_add_test(tc, test_goal_update_impossible);
    tcase_add_test(tc, test_goal_list_err);
    tcase_add_test(tc, test_goal_install);
    tcase_add_test(tc, test_goal_install_multilib);
    tcase_add_test(tc, test_goal_install_selector);
    tcase_add_test(tc, test_goal_install_selector_err);
    tcase_add_test(tc, test_goal_install_selector_two);
    tcase_add_test(tc, test_goal_install_selector_nomatch);
    tcase_add_test(tc, test_goal_install_optional);
    tcase_add_test(tc, test_goal_selector_glob);
    tcase_add_test(tc, test_goal_selector_provides_glob);
    tcase_add_test(tc, test_goal_selector_upgrade);
    tcase_add_test(tc, test_goal_selector_upgrade_provides);
    tcase_add_test(tc, test_goal_upgrade);
    tcase_add_test(tc, test_goal_upgrade_all);
    tcase_add_test(tc, test_goal_downgrade);
    tcase_add_test(tc, test_goal_get_reason);
    tcase_add_test(tc, test_goal_get_reason_selector);
    tcase_add_test(tc, test_goal_describe_problem);
    tcase_add_test(tc, test_goal_distupgrade_all_keep_arch);
    tcase_add_test(tc, test_goal_log_decisions);
    tcase_add_test(tc, test_goal_no_reinstall);
    tcase_add_test(tc, test_goal_erase_simple);
    tcase_add_test(tc, test_goal_erase_with_deps);
    tcase_add_test(tc, test_goal_erase_clean_deps);
    tcase_add_test(tc, test_goal_forcebest);
    suite_add_tcase(s, tc);

    tc = tcase_create("ModifiesSackState");
    tcase_add_unchecked_fixture(tc, fixture_all, teardown);
    tcase_add_checked_fixture(tc, fixture_reset, NULL);
    tcase_add_test(tc, test_goal_installonly);
    tcase_add_test(tc, test_goal_installonly_upgrade_all);
    tcase_add_test(tc, test_goal_upgrade_all_excludes);
    tcase_add_test(tc, test_goal_upgrade_disabled_repo);
    tcase_add_test(tc, test_goal_describe_problem_excludes);
    suite_add_tcase(s, tc);

    tc = tcase_create("Main");
    tcase_add_unchecked_fixture(tc, fixture_with_main, teardown);
    tcase_add_test(tc, test_goal_distupgrade_all);
    tcase_add_test(tc, test_goal_distupgrade_selector_upgrade);
    tcase_add_test(tc, test_goal_distupgrade_selector_downgrade);
    tcase_add_test(tc, test_goal_distupgrade_selector_nothing);
    tcase_add_test(tc, test_goal_install_selector_file);
    tcase_add_test(tc, test_goal_rerun);
    tcase_add_test(tc, test_goal_unneeded);
    tcase_add_test(tc, test_goal_distupgrade_all_excludes);
    suite_add_tcase(s, tc);

    tc = tcase_create("Greedy");
    tcase_add_unchecked_fixture(tc, fixture_greedy_only, teardown);
    tcase_add_test(tc, test_goal_run_all);
    tcase_add_test(tc, test_goal_install_weak_deps);
    suite_add_tcase(s, tc);

    tc = tcase_create("Installonly");
    tcase_add_unchecked_fixture(tc, fixture_installonly, teardown);
    tcase_add_checked_fixture(tc, fixture_reset, NULL);
    tcase_add_test(tc, test_goal_installonly_limit);
    tcase_add_test(tc, test_goal_installonly_limit_disabled);
    tcase_add_test(tc, test_goal_installonly_limit_running_kernel);
    tcase_add_test(tc, test_goal_installonly_limit_with_modules);
    suite_add_tcase(s, tc);

    tc = tcase_create("Vendor");
    tcase_add_unchecked_fixture(tc, fixture_with_vendor, teardown);
    tcase_add_test(tc, test_goal_update_vendor);
    suite_add_tcase(s, tc);

    tc = tcase_create("Forcebest");
    tcase_add_unchecked_fixture(tc, fixture_with_forcebest, teardown);
    tcase_add_test(tc, test_goal_forcebest_arches);
    suite_add_tcase(s, tc);

    tc = tcase_create("Change");
    tcase_add_unchecked_fixture(tc, fixture_with_change, teardown);
    tcase_add_test(tc, test_goal_change);
    tcase_add_test(tc, test_goal_clone);
    suite_add_tcase(s, tc);

    tc = tcase_create("Cmdline");
    tcase_add_unchecked_fixture(tc, fixture_with_cmdline, teardown);
    tcase_add_test(tc, test_cmdline_file_provides);
    suite_add_tcase(s, tc);

    tc = tcase_create("Verify");
    tcase_add_unchecked_fixture(tc, fixture_verify, teardown);
    tcase_add_test(tc, test_goal_verify);
    suite_add_tcase(s, tc);

    return s;
}
