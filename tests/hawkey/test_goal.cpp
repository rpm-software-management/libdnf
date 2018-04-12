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
#include <glib.h>
#include <stdarg.h>


#include "libdnf/dnf-types.h"
#include "libdnf/hy-goal-private.hpp"
#include "libdnf/hy-iutil.h"
#include "libdnf/hy-package-private.hpp"
#include "libdnf/hy-packageset.h"
#include "libdnf/dnf-sack-private.hpp"
#include "libdnf/hy-repo.h"
#include "libdnf/hy-query.h"
#include "libdnf/dnf-sack-private.hpp"
#include "libdnf/dnf-goal.h"
#include "libdnf/hy-selector.h"
#include "libdnf/hy-util-private.hpp"
#include "libdnf/sack/packageset.hpp"
#include "fixtures.h"
#include "testsys.h"
#include "test_suites.h"

static DnfPackage *
get_latest_pkg(DnfSack *sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
    hy_query_filter_latest_per_arch(q, 1);
    GPtrArray *plist = hy_query_run(q);
    fail_unless(plist->len == 1,
                "get_latest_pkg() failed finding '%s'.", name);
    auto pkg = static_cast<DnfPackage *>(g_object_ref(g_ptr_array_index(plist, 0)));
    hy_query_free(q);
    g_ptr_array_unref(plist);
    return pkg;
}

static DnfPackage *
get_available_pkg(DnfSack *sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
    GPtrArray *plist = hy_query_run(q);
    fail_unless(plist->len == 1);
    auto pkg = static_cast<DnfPackage *>(g_object_ref(g_ptr_array_index(plist, 0)));
    hy_query_free(q);
    g_ptr_array_unref(plist);
    return pkg;
}

/* make Sack think we are unable to determine the running kernel */
static Id
mock_running_kernel_no(DnfSack *sack)
{
    return -1;
}

/* make Sack think k-1-1 is the running kernel */
static Id
mock_running_kernel(DnfSack *sack)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "k");
    hy_query_filter(q, HY_PKG_EVR, HY_EQ, "1-1");
    GPtrArray *plist = hy_query_run(q);
    fail_unless(plist->len == 1);
    auto pkg = static_cast<DnfPackage *>(g_object_ref(g_ptr_array_index(plist, 0)));
    hy_query_free(q);
    g_ptr_array_unref(plist);
    Id id = dnf_package_get_id(pkg);
    g_object_unref(pkg);
    return id;
}

static int
size_and_free(GPtrArray *plist)
{
    int c = plist->len;
    g_ptr_array_unref(plist);
    return c;
}

static void
userinstalled(DnfSack *sack, HyGoal goal, const char *name)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
    GPtrArray *plist = hy_query_run(q);

    for(guint i = 0; i < plist->len; i++) {
        auto pkg = static_cast<DnfPackage *>(g_ptr_array_index (plist, i));
        hy_goal_userinstalled(goal, pkg);
    }

    g_ptr_array_unref(plist);
    hy_query_free(q);
}

/* assert on installed-upgraded-erased-obsoleted numbers */
static void
assert_iueo(HyGoal goal, int i, int u, int e, int o)
{
    ck_assert_int_eq(size_and_free(hy_goal_list_installs(goal, NULL)), i);
    ck_assert_int_eq(size_and_free(hy_goal_list_upgrades(goal, NULL)), u);
    ck_assert_int_eq(size_and_free(hy_goal_list_erasures(goal, NULL)), e);
    ck_assert_int_eq(size_and_free(hy_goal_list_obsoleted(goal, NULL)), o);
}

START_TEST(test_goal_sanity)
{
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(goal == NULL);
    fail_unless(dnf_sack_count(test_globals.sack) ==
                TEST_EXPECT_SYSTEM_NSOLVABLES +
                TEST_EXPECT_MAIN_NSOLVABLES +
                TEST_EXPECT_UPDATES_NSOLVABLES);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_actions)
{
    DnfPackage *pkg = get_latest_pkg(test_globals.sack, "walrus");
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_has_actions(goal, DNF_INSTALL));
    fail_if(hy_goal_install(goal, pkg));
    fail_unless(hy_goal_has_actions(goal, DNF_INSTALL));
    g_object_unref(pkg);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_list_err)
{
    g_autoptr(GError) error = NULL;
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_unless(hy_goal_list_installs(goal, &error) == NULL);
    fail_unless(error->code == DNF_ERROR_INTERNAL_ERROR);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_install)
{
    DnfPackage *pkg = get_latest_pkg(test_globals.sack, "walrus");
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_install(goal, pkg));
    g_object_unref(pkg);
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
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
    fail_if(!hy_goal_install_selector(goal, sltr, NULL));
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
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
    fail_if(!hy_goal_install_selector(goal, sltr, NULL));
    hy_selector_free(sltr);

    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 1, 0, 0, 0);

    GPtrArray *plist = hy_goal_list_installs(goal, NULL);
    const char *nvra = dnf_package_get_nevra(
        static_cast<DnfPackage *>(g_ptr_array_index(plist, 0)));
    ck_assert_str_eq(nvra, "semolina-2-0.i686");
    g_ptr_array_unref(plist);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_install_selector_err)
{
    int rc;
    g_autoptr(GError) error = NULL;
    // Test that using the hy_goal_*_selector() methods returns an error for
    // selectors invalid in this context.

    HySelector sltr;
    HyGoal goal = hy_goal_create(test_globals.sack);

    sltr = hy_selector_create(test_globals.sack);
    hy_selector_set(sltr, HY_PKG_ARCH, HY_EQ, "i586");
    fail_unless(!hy_goal_install_selector(goal, sltr, &error));
    fail_unless(error->code == DNF_ERROR_BAD_SELECTOR);
    hy_selector_free(sltr);

    g_clear_error(&error);
    sltr = hy_selector_create(test_globals.sack);
    rc = hy_selector_set(sltr, HY_PKG_NAME, HY_GT, "semolina");
    fail_unless(rc == DNF_ERROR_BAD_SELECTOR);
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
    fail_if(!hy_goal_install_selector(goal, sltr, NULL));
    hy_selector_free(sltr);

    sltr = hy_selector_create(test_globals.sack);
    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "fool");
    fail_if(hy_goal_upgrade_selector(goal, sltr));
    hy_selector_free(sltr);

    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 1, 1, 0, 1);

    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_install_selector_nomatch)
{
    HySelector sltr = hy_selector_create(test_globals.sack);
    HyGoal goal = hy_goal_create(test_globals.sack);

    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "crabalocker");
    fail_if(!hy_goal_install_selector(goal, sltr, NULL));
    hy_selector_free(sltr);

    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 0, 0, 0, 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_install_weak_deps)
{
    HySelector sltr = hy_selector_create(test_globals.sack);
    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "B");
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(!hy_goal_install_selector(goal, sltr, NULL));
    HyGoal goal2 = hy_goal_clone(goal);
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    // recommended package C is installed too
    assert_iueo(goal, 2, 0, 0, 0);

    fail_if(hy_goal_run_flags(goal2, DNF_IGNORE_WEAK_DEPS));
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
    fail_if(!hy_goal_install_selector(goal, sltr, NULL));
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
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
    fail_if(hy_goal_erase_selector_flags(goal, sltr, 0));
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 0, 0, 1, 0);

    hy_goal_free(goal);
    hy_selector_free(sltr);
}
END_TEST


START_TEST(test_goal_selector_upgrade_provides)
{
    DnfSack *sack = test_globals.sack;
    HySelector sltr = hy_selector_create(sack);
    HyGoal goal = hy_goal_create(sack);

    fail_if(hy_selector_set(sltr, HY_PKG_PROVIDES, HY_EQ, "fool"));
    fail_if(hy_goal_upgrade_selector(goal, sltr));
    hy_selector_free(sltr);

    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 0, 1, 0, 1);
    hy_goal_free(goal);

    sltr = hy_selector_create(sack);
    goal = hy_goal_create(sack);
    fail_if(hy_selector_set(sltr, HY_PKG_PROVIDES, HY_EQ, "fool > 1-3"));
    fail_if(hy_goal_upgrade_selector(goal, sltr));
    hy_selector_free(sltr);

    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 0, 1, 0, 1);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_install_selector_file)
{
    DnfSack *sack = test_globals.sack;
    HySelector sltr = hy_selector_create(sack);
    HyGoal goal = hy_goal_create(sack);
    fail_if(hy_selector_set(sltr, HY_PKG_FILE, HY_EQ|HY_GLOB, "/*/answers"));
    fail_if(hy_goal_erase_selector_flags(goal, sltr, 0));
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 0, 0, 1, 0);
    GPtrArray *plist = hy_goal_list_erasures(goal, NULL);
    auto pkg = static_cast<DnfPackage *>(g_ptr_array_index(plist, 0));
    ck_assert_str_eq("fool", dnf_package_get_name(pkg));
    hy_selector_free(sltr);
    g_ptr_array_unref(plist);
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
    fail_if(!hy_goal_install_selector_optional(goal, sltr, NULL));
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    hy_selector_free(sltr);
    assert_iueo(goal, 0, 0, 0, 0);

    // test optional package installation
    DnfPackage *pkg = get_latest_pkg(test_globals.sack, "hello");
    fail_if(hy_goal_install_optional(goal, pkg));
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 0, 0, 0, 0);
    g_object_unref(pkg);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_upgrade)
{
    DnfPackage *pkg = get_latest_pkg(test_globals.sack, "fool");
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_upgrade_to(goal, pkg));
    g_object_unref(pkg);
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 0, 1, 0, 1);
    hy_goal_free(goal);
}
END_TEST

static void
assert_list_names(GPtrArray *plist, ...)
{
    va_list names;
    char *name;
    int count = plist->len, i = 0;

    va_start(names, plist);
    while ((name = va_arg(names, char *)) != NULL) {
        if (i >= count)
            fail("assert_list_names(): list too short");
        auto pkg = static_cast<DnfPackage *>(g_ptr_array_index(plist, i++));
        ck_assert_str_eq(dnf_package_get_name(pkg), name);
    }
    fail_unless(i == count, "assert_list_names(): too many items in the list");
    va_end(names);
}

START_TEST(test_goal_upgrade_all)
{
    HyGoal goal = hy_goal_create(test_globals.sack);
    hy_goal_upgrade_all(goal);
    fail_if(hy_goal_run_flags(goal, DNF_NONE));

    GPtrArray *plist = hy_goal_list_erasures(goal, NULL);
    fail_unless(size_and_free(plist) == 0);

    plist = hy_goal_list_obsoleted(goal, NULL);
    assert_list_names(plist, "penny", NULL);
    g_ptr_array_unref(plist);

    plist = hy_goal_list_upgrades(goal, NULL);
    assert_list_names(plist, "dog", "flying", "fool", "pilchard", "pilchard",
                      NULL);

    // see all obsoletes of fool:
    auto pkg = static_cast<DnfPackage *>(g_ptr_array_index(plist, 2));
    GPtrArray *plist_obs = hy_goal_list_obsoleted_by_package(goal, pkg);
    assert_list_names(plist_obs, "fool", "penny", NULL);
    g_ptr_array_unref(plist_obs);
    g_ptr_array_unref(plist);

    fail_unless(size_and_free(hy_goal_list_installs(goal, NULL)) == 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_downgrade)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *to_be_pkg = get_available_pkg(sack, "baby");
    HyGoal goal = hy_goal_create(sack);

    hy_goal_install(goal, to_be_pkg);
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 0, 0, 0, 0);

    GPtrArray *plist = hy_goal_list_downgrades(goal, NULL);
    fail_unless(plist->len == 1);

    auto pkg = static_cast<DnfPackage *>(g_ptr_array_index(plist, 0));
    ck_assert_str_eq(dnf_package_get_evr(pkg),
                     "6:4.9-3");
    GPtrArray *obsoleted = hy_goal_list_obsoleted_by_package(goal, pkg);
    fail_unless(obsoleted->len == 1);
    auto old_pkg = static_cast<DnfPackage *>(g_ptr_array_index(obsoleted, 0));
    ck_assert_str_eq(dnf_package_get_evr(old_pkg),
                     "6:5.0-11");
    g_ptr_array_unref(obsoleted);
    g_ptr_array_unref(plist);

    hy_goal_free(goal);
    g_object_unref(to_be_pkg);
}
END_TEST

START_TEST(test_goal_get_reason)
{
    DnfPackage *pkg = get_latest_pkg(test_globals.sack, "walrus");
    HyGoal goal = hy_goal_create(test_globals.sack);
    hy_goal_install(goal, pkg);
    g_object_unref(pkg);
    hy_goal_run_flags(goal, DNF_NONE);

    GPtrArray *plist = hy_goal_list_installs(goal, NULL);
    guint i;
    int set = 0;
    for(i = 0; i < plist->len; i++) {
        pkg = static_cast<DnfPackage *>(g_ptr_array_index (plist, i));
        if (!strcmp(dnf_package_get_name(pkg), "walrus")) {
            set |= 1 << 0;
            fail_unless(hy_goal_get_reason(goal, pkg) == HY_REASON_USER);
        }
        if (!strcmp(dnf_package_get_name(pkg), "semolina")) {
            set |= 1 << 1;
            fail_unless(hy_goal_get_reason(goal, pkg) == HY_REASON_DEP);
        }
    }
    fail_unless(set == 3);

    g_ptr_array_unref(plist);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_get_reason_selector)
{

    HySelector sltr = hy_selector_create(test_globals.sack);
    HyGoal goal = hy_goal_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "walrus"));
    fail_if(!hy_goal_install_selector(goal, sltr, NULL));
    hy_selector_free(sltr);

    fail_if(hy_goal_run_flags(goal, DNF_NONE));

    GPtrArray *plist = hy_goal_list_installs(goal, NULL);
    fail_unless(plist->len == 2);
    auto pkg = static_cast<DnfPackage *>(g_ptr_array_index(plist, 0));
    fail_unless(hy_goal_get_reason(goal, pkg) == HY_REASON_USER);

    g_ptr_array_unref(plist);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_describe_problem_rules)
{
    g_autoptr(GError) error = NULL;
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = get_latest_pkg(sack, "hello");
    HyGoal goal = hy_goal_create(sack);

    hy_goal_install(goal, pkg);
    fail_unless(hy_goal_run_flags(goal, DNF_NONE));
    fail_unless(hy_goal_list_installs(goal, &error) == NULL);
    fail_unless(error->code == DNF_ERROR_NO_SOLUTION);
    fail_unless(hy_goal_count_problems(goal) > 0);

    g_auto(GStrv) problems = hy_goal_describe_problem_rules(goal, 0);
    const char *expected[] = {
                "conflicting requests",
                "nothing provides goodbye needed by hello-1-1.noarch"
                };
    for (gint p = 0; p < hy_goal_count_problems(goal); ++p) {
        fail_if(strncmp(problems[p], expected[p], strlen(expected[p])));
    }

    g_object_unref(pkg);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_no_reinstall)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = get_latest_pkg(sack, "penny");
    HyGoal goal = hy_goal_create(sack);
    fail_if(hy_goal_install(goal, pkg));
    g_object_unref(pkg);
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 0, 0, 0, 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_erase_simple)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = by_name_repo(sack, "penny", HY_SYSTEM_REPO_NAME);
    HyGoal goal = hy_goal_create(sack);
    fail_if(hy_goal_erase(goal, pkg));
    g_object_unref(pkg);
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 0, 0, 1, 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_erase_with_deps)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = by_name_repo(sack, "penny-lib", HY_SYSTEM_REPO_NAME);

    // by default can not remove penny-lib, flying depends on it:
    HyGoal goal = hy_goal_create(sack);
    hy_goal_erase(goal, pkg);
    fail_unless(hy_goal_run_flags(goal, DNF_NONE));
    hy_goal_free(goal);

    goal = hy_goal_create(sack);
    hy_goal_erase(goal, pkg);
    fail_if(hy_goal_run_flags(goal, DNF_ALLOW_UNINSTALL));
    assert_iueo(goal, 0, 0, 2, 0);
    hy_goal_free(goal);
    g_object_unref(pkg);
}
END_TEST

START_TEST(test_goal_protected)
{
    DnfSack *sack = test_globals.sack;
    DnfPackageSet *protected_pkgs = dnf_packageset_new(sack);
    DnfPackage *pkg = by_name_repo(sack, "penny-lib", HY_SYSTEM_REPO_NAME);
    DnfPackage *pp = by_name_repo(sack, "flying", HY_SYSTEM_REPO_NAME);
    const char *expected;

    // when protected_packages set is empty it should remove both packages
    HyGoal goal = hy_goal_create(sack);
    DnfPackageSet *empty = dnf_packageset_new(sack);
    dnf_goal_set_protected(goal, empty);
    dnf_packageset_free(empty);
    hy_goal_erase(goal, pkg);
    fail_if(hy_goal_run_flags(goal, DNF_ALLOW_UNINSTALL));
    assert_iueo(goal, 0, 0, 2, 0);
    hy_goal_free(goal);

    // fails to uninstall penny-lib because flying is protected
    goal = hy_goal_create(sack);
    dnf_packageset_add(protected_pkgs, pp);
    dnf_goal_set_protected(goal, protected_pkgs);
    hy_goal_erase(goal, pkg);
    fail_unless(hy_goal_run_flags(goal, DNF_ALLOW_UNINSTALL));
    hy_goal_free(goal);

    // removal of protected package explicitly should trigger error
    goal = hy_goal_create(sack);
    dnf_goal_set_protected(goal, protected_pkgs);
    hy_goal_erase(goal, pp);
    fail_unless(hy_goal_run_flags(goal, DNF_NONE));
    fail_unless(hy_goal_count_problems(goal) == 1);
    auto problem = hy_goal_describe_problem_rules(goal, 0);
    expected = "The operation would result in removing "
        "the following protected packages: flying";
    fail_if(g_strcmp0(*problem, expected));
    hy_goal_free(goal);

    dnf_packageset_free(protected_pkgs);
    g_object_unref(pkg);
    g_object_unref(pp);
}
END_TEST

START_TEST(test_goal_erase_clean_deps)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = by_name_repo(sack, "flying", HY_SYSTEM_REPO_NAME);

    // by default, leave dependencies alone:
    HyGoal goal = hy_goal_create(sack);
    hy_goal_erase(goal, pkg);
    hy_goal_run_flags(goal, DNF_NONE);
    assert_iueo(goal, 0, 0, 1, 0);
    hy_goal_free(goal);

    // allow deleting dependencies:
    goal = hy_goal_create(sack);
    hy_goal_erase_flags(goal, pkg, HY_CLEAN_DEPS);
    fail_unless(hy_goal_run_flags(goal, DNF_NONE) == 0);
    assert_iueo(goal, 0, 0, 2, 0);
    hy_goal_free(goal);

    // test userinstalled specification:
    DnfPackage *penny_pkg = by_name_repo(sack, "penny-lib", HY_SYSTEM_REPO_NAME);
    goal = hy_goal_create(sack);
    hy_goal_erase_flags(goal, pkg, HY_CLEAN_DEPS);
    hy_goal_userinstalled(goal, penny_pkg);
    // having the same solvable twice in a goal shouldn't break anything:
    hy_goal_userinstalled(goal, pkg);
    fail_unless(hy_goal_run_flags(goal, DNF_NONE) == 0);
    assert_iueo(goal, 0, 0, 1, 0);
    hy_goal_free(goal);
    g_object_unref(penny_pkg);

    g_object_unref(pkg);
}
END_TEST

START_TEST(test_goal_forcebest)
{
    DnfSack *sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);
    HySelector sltr = hy_selector_create(sack);

    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "flying");
    hy_goal_upgrade_selector(goal, sltr);
    fail_unless(hy_goal_run_flags(goal, DNF_FORCE_BEST));
    fail_unless(hy_goal_count_problems(goal) == 1);

    hy_selector_free(sltr);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_installonly)
{
    const char *installonly[] = {"fool", NULL};

    DnfSack *sack = test_globals.sack;
    dnf_sack_set_installonly(sack, installonly);
    dnf_sack_set_installonly_limit(sack, 2);
    DnfPackage *pkg = get_latest_pkg(sack, "fool");
    HyGoal goal = hy_goal_create(sack);
    fail_if(hy_goal_upgrade_to(goal, pkg));
    g_object_unref(pkg);
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 1, 0, 1, 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_installonly_upgrade_all)
{
    const char *installonly[] = {"fool", NULL};
    DnfSack *sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);

    dnf_sack_set_installonly(sack, installonly);
    dnf_sack_set_installonly_limit(sack, 2);

    hy_goal_upgrade_all(goal);
    fail_if(hy_goal_run_flags(goal, DNF_NONE));

    GPtrArray *plist = hy_goal_list_erasures(goal, NULL);
    assert_list_names(plist, "penny", NULL);
    g_ptr_array_unref(plist);
    plist = hy_goal_list_installs(goal, NULL);
    assert_list_names(plist, "fool", NULL);
    g_ptr_array_unref(plist);
    assert_iueo(goal, 1, 4, 1, 0);

    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_upgrade_all_excludes)
{
    DnfSack *sack = test_globals.sack;
    HyQuery q = hy_query_create_flags(sack, HY_IGNORE_EXCLUDES);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "pilchard");

    DnfPackageSet *pset = hy_query_run_set(q);
    dnf_sack_add_excludes(sack, pset);
    dnf_packageset_free(pset);
    hy_query_free(q);

    HyGoal goal = hy_goal_create(sack);
    hy_goal_upgrade_all(goal);
    hy_goal_run_flags(goal, DNF_NONE);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal, NULL)) == 3);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_upgrade_disabled_repo)
{
    DnfSack *sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);

    hy_goal_upgrade_all(goal);
    hy_goal_run_flags(goal, DNF_NONE);
    fail_unless(size_and_free(hy_goal_list_upgrades(goal, NULL)) == 5);
    hy_goal_free(goal);

    dnf_sack_repo_enabled(sack, "updates", 0);
    goal = hy_goal_create(sack);
    hy_goal_upgrade_all(goal);
    hy_goal_run_flags(goal, DNF_NONE);
    assert_iueo(goal, 0, 1, 0, 0);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_describe_problem_excludes)
{
    DnfSack *sack = test_globals.sack;

    HyQuery q = hy_query_create_flags(sack, HY_IGNORE_EXCLUDES);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "semolina");
    DnfPackageSet *pset = hy_query_run_set(q);
    dnf_sack_add_excludes(sack, pset);
    dnf_packageset_free(pset);
    hy_query_free(q);

    HyGoal goal = hy_goal_create(sack);
    HySelector sltr = hy_selector_create(sack);

    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "semolina");
    hy_goal_install_selector(goal, sltr, NULL);
    hy_selector_free(sltr);

    fail_unless(hy_goal_run_flags(goal, DNF_NONE));
    fail_unless(hy_goal_count_problems(goal) > 0);

    auto problems = hy_goal_describe_problem_rules(goal, 0);
    const char *expected[] = {
                "conflicting requests",
                "package semolina does not exist"
                };
    for (gint p = 0; p < hy_goal_count_problems(goal); ++p) {
        fail_if(strncmp(problems[p], expected[p], strlen(expected[p])));
    }

    g_free(problems);

    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_distupgrade_all)
{
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_distupgrade_all(goal));
    fail_if(hy_goal_run_flags(goal, DNF_NONE));

    assert_iueo(goal, 0, 1, 0, 0);
    GPtrArray *plist = hy_goal_list_upgrades(goal, NULL);
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(plist, 0)), "flying-3-0.noarch");
    g_ptr_array_unref(plist);

    plist = hy_goal_list_downgrades(goal, NULL);
    fail_unless(plist->len == 1);
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(plist, 0)), "baby-6:4.9-3.x86_64");
    g_ptr_array_unref(plist);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_distupgrade_all_excludes)
{
    HyQuery q = hy_query_create_flags(test_globals.sack, HY_IGNORE_EXCLUDES);
    hy_query_filter_provides(q, HY_GT|HY_EQ, "flying", "0");
    DnfPackageSet *pset = hy_query_run_set(q);
    dnf_sack_add_excludes(test_globals.sack, pset);
    dnf_packageset_free(pset);
    hy_query_free(q);

    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_distupgrade_all(goal));
    fail_if(hy_goal_run_flags(goal, DNF_NONE));

    assert_iueo(goal, 0, 0, 0, 0);

    GPtrArray *plist = hy_goal_list_downgrades(goal, NULL);
    fail_unless(plist->len == 1);
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(plist, 0)), "baby-6:4.9-3.x86_64");
    g_ptr_array_unref(plist);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_distupgrade_all_keep_arch)
{
    HyGoal goal = hy_goal_create(test_globals.sack);
    fail_if(hy_goal_distupgrade_all(goal));
    fail_if(hy_goal_run_flags(goal, DNF_NONE));

    assert_iueo(goal, 0, 5, 0, 1);
    GPtrArray *plist = hy_goal_list_upgrades(goal, NULL);
    // gun pkg is not upgraded to latest version of different arch
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(plist, 0)), "dog-1-2.x86_64");
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(plist, 1)),
                    "pilchard-1.2.4-1.i686");
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(plist, 2)),
                    "pilchard-1.2.4-1.x86_64");
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(plist, 3)), "flying-3.1-0.x86_64");
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(plist, 4)), "fool-1-5.noarch");
    g_ptr_array_unref(plist);

    plist = hy_goal_list_obsoleted(goal, NULL);
    fail_unless(plist->len == 1);
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(plist, 0)), "penny-4-1.noarch");
    g_ptr_array_unref(plist);

    plist = hy_goal_list_downgrades(goal, NULL);
    fail_unless(plist->len == 1);
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(plist, 0)), "baby-6:4.9-3.x86_64");
    g_ptr_array_unref(plist);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_distupgrade_selector_upgrade)
{
    HyGoal goal = hy_goal_create(test_globals.sack);
    HySelector sltr = hy_selector_create(test_globals.sack);
    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "flying");
    fail_if(hy_goal_distupgrade_selector(goal, sltr));
    fail_if(hy_goal_run_flags(goal, DNF_NONE));

    assert_iueo(goal, 0, 1, 0, 0);
    GPtrArray *plist = hy_goal_list_upgrades(goal, NULL);
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(plist, 0)), "flying-3-0.noarch");

    g_ptr_array_unref(plist);
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
    fail_if(hy_goal_run_flags(goal, DNF_NONE));

    assert_iueo(goal, 0, 0, 0, 0);
    GPtrArray *plist = hy_goal_list_downgrades(goal, NULL);
    fail_unless(plist->len == 1);
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(plist, 0)), "baby-6:4.9-3.x86_64");

    g_ptr_array_unref(plist);
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
    fail_if(hy_goal_run_flags(goal, DNF_NONE));

    assert_iueo(goal, 0, 0, 0, 0);
    GPtrArray *plist = hy_goal_list_downgrades(goal, NULL);
    fail_unless(plist->len == 0);
    g_ptr_array_unref(plist);
    hy_goal_free(goal);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_goal_rerun)
{
    DnfSack *sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);
    DnfPackage *pkg = get_latest_pkg(sack, "walrus");

    hy_goal_install(goal, pkg);
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 2, 0, 0, 0);
    g_object_unref(pkg);

    // add an erase:
    pkg = by_name_repo(sack, "dog", HY_SYSTEM_REPO_NAME);
    hy_goal_erase(goal, pkg);
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 2, 0, 1, 0);
    g_object_unref(pkg);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_unneeded)
{
    DnfSack *sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);

    userinstalled(sack, goal, "baby");
    userinstalled(sack, goal, "dog");
    userinstalled(sack, goal, "fool");
    userinstalled(sack, goal, "gun");
    userinstalled(sack, goal, "jay");
    userinstalled(sack, goal, "penny");
    userinstalled(sack, goal, "pilchard");
    hy_goal_run_flags(goal, DNF_NONE);

    GPtrArray *plist = hy_goal_list_unneeded(goal, NULL);
    ck_assert_int_eq(plist->len, 4);
    auto pkg = static_cast<DnfPackage *>(g_ptr_array_index(plist, 0));
    assert_nevra_eq(pkg, "flying-2-9.noarch");
    pkg = static_cast<DnfPackage *>(g_ptr_array_index(plist, 1));
    assert_nevra_eq(pkg, "penny-lib-4-1.x86_64");
    g_ptr_array_unref(plist);

    hy_goal_free(goal);
}
END_TEST

struct Solutions {
    int solutions;
    GPtrArray *installs;
};

START_TEST(test_goal_installonly_limit)
{
    const char *installonly[] = {"k", NULL};
    DnfSack *sack = test_globals.sack;
    dnf_sack_set_installonly(sack, installonly);
    dnf_sack_set_installonly_limit(sack, 3);
    dnf_sack_set_running_kernel_fn(sack, mock_running_kernel_no);

    HyGoal goal = hy_goal_create(sack);
    hy_goal_upgrade_all(goal);
    fail_if(hy_goal_run_flags(goal, DNF_NONE));

    assert_iueo(goal, 1, 1, 3, 0); // k-m is just upgraded
    GPtrArray *erasures = hy_goal_list_erasures(goal, NULL);
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(erasures, 0)), "k-1-0.x86_64");
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(erasures, 1)),
                    "k-freak-1-0-1-0.x86_64");
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(erasures, 2)), "k-1-1.x86_64");
    g_ptr_array_unref(erasures);

    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_kernel_protected)
{
    DnfSack *sack = test_globals.sack;
    dnf_sack_set_running_kernel_fn(sack, mock_running_kernel);
    Id kernel_id = mock_running_kernel(sack);
    DnfPackage *kernel = dnf_package_new(sack, kernel_id);

    HyGoal goal = hy_goal_create(sack);
    hy_goal_erase(goal, kernel);
    fail_unless(hy_goal_run_flags(goal, DNF_NONE));

    g_object_unref(kernel);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_installonly_limit_disabled)
{
    // test that setting limit to 0 does not cause all intallonlies to be
    // uninstalled
    const char *installonly[] = {"k", NULL};
    DnfSack *sack = test_globals.sack;
    dnf_sack_set_installonly(sack, installonly);
    dnf_sack_set_installonly_limit(sack, 0);
    dnf_sack_set_running_kernel_fn(sack, mock_running_kernel_no);

    HyGoal goal = hy_goal_create(sack);
    hy_goal_upgrade_all(goal);
    fail_if(hy_goal_run_flags(goal, DNF_NONE));

    assert_iueo(goal, 1, 1, 0, 0);
    hy_goal_free(goal);
}
END_TEST


START_TEST(test_goal_installonly_limit_running_kernel)
{
    const char *installonly[] = {"k", NULL};
    DnfSack *sack = test_globals.sack;
    dnf_sack_set_installonly(sack, installonly);
    dnf_sack_set_installonly_limit(sack, 3);
    dnf_sack_set_running_kernel_fn(sack, mock_running_kernel);

    HyGoal goal = hy_goal_create(sack);
    hy_goal_upgrade_all(goal);
    fail_if(hy_goal_run_flags(goal, DNF_NONE));

    assert_iueo(goal, 1, 1, 3, 0);
    GPtrArray *erasures = hy_goal_list_erasures(goal, NULL);
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(erasures, 0)), "k-1-0.x86_64");
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(erasures, 1)),
                    "k-freak-1-0-1-0.x86_64");
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(erasures, 2)), "k-2-0.x86_64");
    g_ptr_array_unref(erasures);

    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_installonly_limit_with_modules)
{
    // most complex installonly test case, includes the k-m packages
    const char *installonly[] = {"k", "k-m", NULL};
    DnfSack *sack = test_globals.sack;
    dnf_sack_set_installonly(sack, installonly);
    dnf_sack_set_installonly_limit(sack, 3);
    dnf_sack_set_running_kernel_fn(sack, mock_running_kernel);

    HyGoal goal = hy_goal_create(sack);
    hy_goal_upgrade_all(goal);
    fail_if(hy_goal_run_flags(goal, DNF_NONE));

    assert_iueo(goal, 2, 0, 5, 0);
    GPtrArray *erasures = hy_goal_list_erasures(goal, NULL);
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(erasures, 0)), "k-1-0.x86_64");
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(erasures, 1)), "k-m-1-0.x86_64");
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(erasures, 2)),
                    "k-freak-1-0-1-0.x86_64");
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(erasures, 3)), "k-2-0.x86_64");
    assert_nevra_eq(static_cast<DnfPackage *>(g_ptr_array_index(erasures, 4)), "k-m-2-0.x86_64");
    g_ptr_array_unref(erasures);

    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_update_vendor)
{
    DnfSack *sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);
    HySelector sltr = hy_selector_create(sack);

    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "fool");
    fail_if(hy_goal_upgrade_selector(goal, sltr));
    hy_selector_free(sltr);

    /* hy_goal_upgrade_all(goal); */
    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 1, 0, 0, 1);

    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_forcebest_arches)
{
    DnfSack *sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);
    HySelector sltr = hy_selector_create(sack);

    hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "gun");
    fail_if(hy_goal_upgrade_selector(goal, sltr));
    fail_if(hy_goal_run_flags(goal, DNF_FORCE_BEST));
    assert_iueo(goal, 0, 0, 0, 0);

    hy_selector_free(sltr);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_change)
{
    // test that changes are handled like reinstalls

    DnfSack *sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);

    hy_goal_upgrade_all(goal);

    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 0, 1, 0, 0);
    fail_unless(size_and_free(hy_goal_list_reinstalls(goal, NULL)) == 1);
    hy_goal_free(goal);
}
END_TEST

START_TEST(test_goal_clone)
{
    DnfSack *sack = test_globals.sack;
    HyGoal goal = hy_goal_create(sack);

    hy_goal_upgrade_all(goal);
    HyGoal goal2 = hy_goal_clone(goal);

    fail_if(hy_goal_run_flags(goal, DNF_NONE));
    assert_iueo(goal, 0, 1, 0, 0);
    fail_unless(size_and_free(hy_goal_list_reinstalls(goal, NULL)) == 1);
    hy_goal_free(goal);

    fail_if(hy_goal_run_flags(goal2, DNF_NONE));
    assert_iueo(goal2, 0, 1, 0, 0);
    fail_unless(size_and_free(hy_goal_list_reinstalls(goal2, NULL)) == 1);
    hy_goal_free(goal2);
}
END_TEST

START_TEST(test_cmdline_file_provides)
{
    DnfSack *sack = test_globals.sack;
    dnf_sack_set_running_kernel_fn(sack, mock_running_kernel_no);
    HyGoal goal = hy_goal_create(sack);

    hy_goal_upgrade_all(goal);
    ck_assert(!hy_goal_run_flags(goal, DNF_FORCE_BEST));
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
    tcase_add_test(tc, test_goal_selector_upgrade_provides);
    tcase_add_test(tc, test_goal_upgrade);
    tcase_add_test(tc, test_goal_upgrade_all);
    tcase_add_test(tc, test_goal_downgrade);
    tcase_add_test(tc, test_goal_get_reason);
    tcase_add_test(tc, test_goal_get_reason_selector);
    tcase_add_test(tc, test_goal_describe_problem_rules);
    tcase_add_test(tc, test_goal_distupgrade_all_keep_arch);
    tcase_add_test(tc, test_goal_no_reinstall);
    tcase_add_test(tc, test_goal_erase_simple);
    tcase_add_test(tc, test_goal_erase_with_deps);
    tcase_add_test(tc, test_goal_protected);
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
    tcase_add_test(tc, test_goal_install_weak_deps);
    suite_add_tcase(s, tc);

    tc = tcase_create("Installonly");
    tcase_add_unchecked_fixture(tc, fixture_installonly, teardown);
    tcase_add_checked_fixture(tc, fixture_reset, NULL);
    tcase_add_test(tc, test_goal_installonly_limit);
    tcase_add_test(tc, test_goal_installonly_limit_disabled);
    tcase_add_test(tc, test_goal_installonly_limit_running_kernel);
    tcase_add_test(tc, test_goal_installonly_limit_with_modules);
    tcase_add_test(tc, test_goal_kernel_protected);
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
    suite_add_tcase(s, tc);

    return s;
}
