/*
 * Copyright (C) 2012-2014 Red Hat, Inc.
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


#include "libdnf/hy-package-private.h"
#include "libdnf/hy-query.h"
#include "libdnf/hy-selector.h"
#include "libdnf/dnf-types.h"
#include "libdnf/hy-util.h"
#include "fixtures.h"
#include "testsys.h"
#include "test_suites.h"

START_TEST(test_sltr_pkg)
{
    HySelector sltr = hy_selector_create(test_globals.sack);
    DnfPackageSet *pset = dnf_packageset_new(test_globals.sack);

    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "penny");
    GPtrArray *plist1 = hy_query_run(q);
    hy_query_free(q);
    fail_unless(plist1->len == 2);
    DnfPackage *pkg0 = g_object_ref(g_ptr_array_index(plist1, 0));
    DnfPackage *pkg1 = g_object_ref(g_ptr_array_index(plist1, 1));
    g_ptr_array_unref(plist1);
    dnf_packageset_add(pset, pkg0);

    fail_if(hy_selector_pkg_set(sltr, HY_PKG, HY_EQ, pset));
    GPtrArray *plist = hy_selector_matches(sltr);
    fail_unless(plist->len == 1);
    fail_unless(hy_packagelist_has(plist, pkg0));
    fail_if(hy_packagelist_has(plist, pkg1));

    g_object_unref(pkg0);
    g_object_unref(pkg1);
    g_object_unref(pset);
    g_ptr_array_unref(plist);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_sltr_matching)
{
    HySelector sltr = hy_selector_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "penny"));
    GPtrArray *plist = hy_selector_matches(sltr);
    fail_unless(plist->len == 2);

    g_ptr_array_unref(plist);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_sltr_provides)
{
    HySelector sltr = hy_selector_create(test_globals.sack);
    ck_assert_int_eq(hy_selector_set(sltr, HY_PKG_PROVIDES, HY_EQ, "*"),
                     DNF_ERROR_BAD_SELECTOR);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_sltr_provides_glob)
{
    HySelector sltr = hy_selector_create(test_globals.sack);
    fail_if(hy_selector_set(sltr, HY_PKG_PROVIDES, HY_GLOB, "*alru*"));
    GPtrArray *plist = hy_selector_matches(sltr);
    fail_unless(plist->len == 2);

    g_ptr_array_unref(plist);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_sltr_reponame)
{
    HySelector sltr = hy_selector_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "penny"));
    fail_if(hy_selector_set(sltr, HY_PKG_REPONAME, HY_EQ, "main"));
    GPtrArray *plist = hy_selector_matches(sltr);
    fail_unless(plist->len == 1);

    g_ptr_array_unref(plist);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_sltr_reponame_nonexistent)
{
    HySelector sltr = hy_selector_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_GLOB, "*"));
    fail_if(hy_selector_set(sltr, HY_PKG_REPONAME, HY_EQ, "non-existent"));
    GPtrArray *plist = hy_selector_matches(sltr);
    fail_unless(plist->len == 0);

    g_ptr_array_unref(plist);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_sltr_version)
{
    HySelector sltr = hy_selector_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "jay"));
    fail_if(hy_selector_set(sltr, HY_PKG_VERSION, HY_EQ, "5.0"));
    GPtrArray *plist = hy_selector_matches(sltr);
    fail_unless(plist->len == 2);

    g_ptr_array_unref(plist);
    hy_selector_free(sltr);
}
END_TEST

Suite *
selector_suite(void)
{
    Suite *s = suite_create("Selector");
    TCase *tc = tcase_create("Core");

    tcase_add_unchecked_fixture(tc, fixture_all, teardown);
    tcase_add_test(tc, test_sltr_pkg);
    tcase_add_test(tc, test_sltr_matching);
    tcase_add_test(tc, test_sltr_provides);
    tcase_add_test(tc, test_sltr_provides_glob);
    tcase_add_test(tc, test_sltr_reponame);
    tcase_add_test(tc, test_sltr_reponame_nonexistent);
    tcase_add_test(tc, test_sltr_version);

    suite_add_tcase(s, tc);

    return s;
}
