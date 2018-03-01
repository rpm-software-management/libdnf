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


#include "libdnf/hy-util-private.hpp"
#include "libdnf/repo/RpmPackage.hpp"
#include "test_suites.h"

static DnfPackage *
mock_package(Id id)
{
    return dnf_package_new(nullptr, id);
}

static std::vector<libdnf::RpmPackage *> fixture_plist;

static void
create_fixture(void)
{
    fixture_plist.emplace_back(mock_package(10));
    fixture_plist.emplace_back(mock_package(11));
    fixture_plist.emplace_back(mock_package(12));
}

static void
free_fixture(void)
{
    for (auto package : fixture_plist)
        delete package;
}

START_TEST(test_has)
{
    DnfPackage *pkg1 = mock_package(10);
    DnfPackage *pkg2 = mock_package(1);

    bool hasPkg1 = false;
    bool hasPkg2 = false;

    for (auto package : fixture_plist) {
        if (*pkg1 == *package)
            hasPkg1 = true;
        else if (*pkg1 == *package)
            hasPkg2 = true;
    }

    fail_unless(hasPkg1);
    fail_if(hasPkg2);

    delete pkg1;
    delete pkg2;
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
