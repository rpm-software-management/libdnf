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
#include "src/packageset_internal.h"
#include "src/sack_internal.h"
#include "fixtures.h"
#include "test_suites.h"

static HyPackageSet pset;

static void
packageset_fixture(void)
{
    fixture_all();

    HySack sack = test_globals.sack;
    HyPackage pkg0 = package_create(sack, 0);
    HyPackage pkg9 = package_create(sack, 9);
    int max = sack_last_solvable(sack);
    HyPackage pkg_max = package_create(sack, max);

    // init the global var
    pset = hy_packageset_create(sack);

    hy_packageset_add(pset, pkg0);
    hy_packageset_add(pset, pkg9);
    hy_packageset_add(pset, pkg_max);

}

static void
packageset_teardown(void)
{
    hy_packageset_free(pset);
    teardown();
}

START_TEST(test_clone)
{
    HySack sack = test_globals.sack;
    HyPackageSet pset2 = hy_packageset_clone(pset);

    HyPackage pkg8 = package_create(sack, 8);
    HyPackage pkg9 = package_create(sack, 9);

    fail_if(hy_packageset_has(pset2, pkg8));
    fail_unless(hy_packageset_has(pset2, pkg9));

    hy_package_free(pkg8);
    hy_package_free(pkg9);
    hy_packageset_free(pset2);
}
END_TEST

START_TEST(test_has)
{
    HySack sack = test_globals.sack;
    HyPackage pkg0 = package_create(sack, 0);
    HyPackage pkg9 = package_create(sack, 9);
    HyPackage pkg_max = package_create(sack, sack_last_solvable(sack));

    HyPackage pkg7 = package_create(sack, 7);
    HyPackage pkg8 = package_create(sack, 8);
    HyPackage pkg15 = package_create(sack, 15);

    fail_unless(hy_packageset_has(pset, pkg0));
    fail_unless(hy_packageset_has(pset, pkg9));
    fail_unless(hy_packageset_has(pset, pkg_max));
    fail_if(hy_packageset_has(pset, pkg7));
    fail_if(hy_packageset_has(pset, pkg8));
    fail_if(hy_packageset_has(pset, pkg15));

    hy_package_free(pkg0);
    hy_package_free(pkg9);
    hy_package_free(pkg_max);
    hy_package_free(pkg7);
    hy_package_free(pkg8);
    hy_package_free(pkg15);

}
END_TEST

START_TEST(test_get_clone)
{
    HySack sack = test_globals.sack;
    int max = sack_last_solvable(sack);

    fail_unless(hy_packageset_count(pset) == 3);
    HyPackage pkg0 = hy_packageset_get_clone(pset, 0);
    HyPackage pkg9 = hy_packageset_get_clone(pset, 1);
    HyPackage pkg_max = hy_packageset_get_clone(pset, 2);
    fail_unless(package_id(pkg0) == 0);
    fail_unless(package_id(pkg9) == 9);
    fail_unless(package_id(pkg_max) == max);
    fail_unless(hy_packageset_get_clone(pset, 3) == NULL);

    hy_package_free(pkg0);
    hy_package_free(pkg9);
    hy_package_free(pkg_max);

    HyPackage pkg8 = package_create(sack, 8);
    HyPackage pkg11 = package_create(sack, 11);
    hy_packageset_add(pset, pkg8);
    hy_packageset_add(pset, pkg11);
    pkg8 = hy_packageset_get_clone(pset, 1);
    pkg9 = hy_packageset_get_clone(pset, 2);
    pkg11 = hy_packageset_get_clone(pset, 3);
    fail_unless(package_id(pkg8) == 8);
    fail_unless(package_id(pkg9) == 9);
    fail_unless(package_id(pkg11) == 11);

    hy_package_free(pkg8);
    hy_package_free(pkg9);
    hy_package_free(pkg11);
}
END_TEST

START_TEST(test_get_pkgid)
{
    HySack sack = test_globals.sack;
    int max = sack_last_solvable(sack);

    // add some more packages
    HyPackage pkg;
    pkg = package_create(sack, 7);
    hy_packageset_add(pset, pkg);
    pkg = package_create(sack, 8);
    hy_packageset_add(pset, pkg);
    pkg = package_create(sack, 15);
    hy_packageset_add(pset, pkg);

    Id id = -1;
    id = packageset_get_pkgid(pset, 0, id);
    fail_unless(id == 0);
    id = packageset_get_pkgid(pset, 1, id);
    fail_unless(id == 7);
    id = packageset_get_pkgid(pset, 2, id);
    fail_unless(id == 8);
    id = packageset_get_pkgid(pset, 3, id);
    fail_unless(id == 9);
    id = packageset_get_pkgid(pset, 4, id);
    fail_unless(id == 15);
    id = packageset_get_pkgid(pset, 5, id);
    fail_unless(id == max);
}
END_TEST

Suite *
packageset_suite(void)
{
    Suite *s = suite_create("PackageSet");

    TCase *tc = tcase_create("Core");
    tcase_add_checked_fixture(tc, packageset_fixture, packageset_teardown);
    tcase_add_test(tc, test_clone);
    tcase_add_test(tc, test_has);
    tcase_add_test(tc, test_get_clone);
    tcase_add_test(tc, test_get_pkgid);
    suite_add_tcase(s, tc);

    return s;
}
