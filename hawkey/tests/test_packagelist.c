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

// hawkey
#include "src/package_internal.h"
#include "src/packagelist.h"
#include "test_suites.h"

static HyPackage
mock_package(Id id)
{
    return package_create(NULL, id);
}

static HyPackageList fixture_plist;

static void
create_fixture(void)
{
    fixture_plist = hy_packagelist_create();
    hy_packagelist_push(fixture_plist, mock_package(10));
    hy_packagelist_push(fixture_plist, mock_package(11));
    hy_packagelist_push(fixture_plist, mock_package(12));
}

static void
free_fixture(void)
{
    hy_packagelist_free(fixture_plist);
}

START_TEST(test_iter_macro)
{
    int i, max = 0, count = 0;
    HyPackage pkg;
    FOR_PACKAGELIST(pkg, fixture_plist, i) {
	count += 1;
	max = i;
    }
    fail_unless(max == 2);
    fail_unless(count == hy_packagelist_count(fixture_plist));
}
END_TEST

START_TEST(test_has)
{
    HyPackage pkg1 = mock_package(10);
    HyPackage pkg2 = mock_package(1);

    fail_unless(hy_packagelist_has(fixture_plist, pkg1));
    fail_if(hy_packagelist_has(fixture_plist, pkg2));

    hy_package_free(pkg1);
    hy_package_free(pkg2);
}
END_TEST

Suite *
packagelist_suite(void)
{
    Suite *s = suite_create("PackageList");

    TCase *tc = tcase_create("Core");
    tcase_add_unchecked_fixture(tc, create_fixture, free_fixture);
    tcase_add_test(tc, test_iter_macro);
    tcase_add_test(tc, test_has);
    suite_add_tcase(s, tc);

    return s;
}
