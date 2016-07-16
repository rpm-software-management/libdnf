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


#include "libdnf/hy-package-private.h"
#include "libdnf/hy-util.h"
#include "test_suites.h"

static DnfPackage *
mock_package(Id id)
{
    return dnf_package_new(NULL, id);
}

static GPtrArray *fixture_plist;

static void
create_fixture(void)
{
    fixture_plist = hy_packagelist_create();
    g_ptr_array_add(fixture_plist, mock_package(10));
    g_ptr_array_add(fixture_plist, mock_package(11));
    g_ptr_array_add(fixture_plist, mock_package(12));
}

static void
free_fixture(void)
{
    g_ptr_array_unref(fixture_plist);
}

START_TEST(test_has)
{
    DnfPackage *pkg1 = mock_package(10);
    DnfPackage *pkg2 = mock_package(1);

    fail_unless(hy_packagelist_has(fixture_plist, pkg1));
    fail_if(hy_packagelist_has(fixture_plist, pkg2));

    g_object_unref(pkg1);
    g_object_unref(pkg2);
}
END_TEST

Suite *
packagelist_suite(void)
{
    Suite *s = suite_create("PackageList");

    TCase *tc = tcase_create("Core");
    tcase_add_unchecked_fixture(tc, create_fixture, free_fixture);
    tcase_add_test(tc, test_has);
    suite_add_tcase(s, tc);

    return s;
}
