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

// hawkey
#include "src/selector.h"
#include "fixtures.h"
#include "testsys.h"
#include "test_suites.h"

START_TEST(test_sltr_matching)
{
    HySelector sltr = hy_selector_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "penny"));
    HyPackageList plist = hy_selector_matches(sltr);
    fail_unless(hy_packagelist_count(plist) == 2);

    hy_packagelist_free(plist);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_sltr_reponame)
{
    HySelector sltr = hy_selector_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "penny"));
    fail_if(hy_selector_set(sltr, HY_PKG_REPONAME, HY_EQ, "main"));
    HyPackageList plist = hy_selector_matches(sltr);
    fail_unless(hy_packagelist_count(plist) == 1);

    hy_packagelist_free(plist);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_sltr_reponame_nonexistent)
{
    HySelector sltr = hy_selector_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_GLOB, "*"));
    fail_if(hy_selector_set(sltr, HY_PKG_REPONAME, HY_EQ, "non-existent"));
    HyPackageList plist = hy_selector_matches(sltr);
    fail_unless(hy_packagelist_count(plist) == 0);

    hy_packagelist_free(plist);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_sltr_version)
{
    HySelector sltr = hy_selector_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "jay"));
    fail_if(hy_selector_set(sltr, HY_PKG_VERSION, HY_EQ, "5.0"));
    HyPackageList plist = hy_selector_matches(sltr);
    fail_unless(hy_packagelist_count(plist) == 2);

    hy_packagelist_free(plist);
    hy_selector_free(sltr);
}
END_TEST

Suite *
selector_suite(void)
{
    Suite *s = suite_create("Selector");
    TCase *tc = tcase_create("Core");

    tcase_add_unchecked_fixture(tc, fixture_all, teardown);
    tcase_add_test(tc, test_sltr_matching);
    tcase_add_test(tc, test_sltr_reponame);
    tcase_add_test(tc, test_sltr_reponame_nonexistent);
    tcase_add_test(tc, test_sltr_version);

    suite_add_tcase(s, tc);

    return s;
}
