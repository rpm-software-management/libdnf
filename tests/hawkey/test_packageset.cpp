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


#include "libdnf/hy-package-private.hpp"
#include "libdnf/hy-packageset-private.hpp"
#include "libdnf/dnf-sack-private.hpp"
#include "libdnf/sack/packageset.hpp"

#include "fixtures.h"
#include "test_suites.h"

static DnfPackageSet *pset;

static void
packageset_fixture(void)
{
    fixture_all();

    DnfSack *sack = test_globals.sack;
    g_autoptr(DnfPackage) pkg0 = dnf_package_new(sack, 0);
    g_autoptr(DnfPackage) pkg9 = dnf_package_new(sack, 9);
    int max = dnf_sack_last_solvable(sack);
    g_autoptr(DnfPackage) pkg_max = dnf_package_new(sack, max);

    // init the global var
    pset = dnf_packageset_new(sack);

    dnf_packageset_add(pset, pkg0);
    dnf_packageset_add(pset, pkg9);
    dnf_packageset_add(pset, pkg_max);

}

static void
packageset_teardown(void)
{
    delete pset;
    teardown();
}

START_TEST(test_clone)
{
    DnfSack *sack = test_globals.sack;
    auto pset2 = std::unique_ptr <DnfPackageSet> (dnf_packageset_clone(pset));

    DnfPackage *pkg8 = dnf_package_new(sack, 8);
    DnfPackage *pkg9 = dnf_package_new(sack, 9);

    fail_if(dnf_packageset_has(pset2.get(), pkg8));
    fail_unless(dnf_packageset_has(pset2.get(), pkg9));

    g_object_unref(pkg8);
    g_object_unref(pkg9);
}
END_TEST

START_TEST(test_has)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg0 = dnf_package_new(sack, 0);
    DnfPackage *pkg9 = dnf_package_new(sack, 9);
    DnfPackage *pkg_max = dnf_package_new(sack, dnf_sack_last_solvable(sack));

    DnfPackage *pkg7 = dnf_package_new(sack, 7);
    DnfPackage *pkg8 = dnf_package_new(sack, 8);
    DnfPackage *pkg15 = dnf_package_new(sack, 15);

    fail_unless(dnf_packageset_has(pset, pkg0));
    fail_unless(dnf_packageset_has(pset, pkg9));
    fail_unless(dnf_packageset_has(pset, pkg_max));
    fail_if(dnf_packageset_has(pset, pkg7));
    fail_if(dnf_packageset_has(pset, pkg8));
    fail_if(dnf_packageset_has(pset, pkg15));

    g_object_unref(pkg0);
    g_object_unref(pkg9);
    g_object_unref(pkg_max);
    g_object_unref(pkg7);
    g_object_unref(pkg8);
    g_object_unref(pkg15);

}
END_TEST

START_TEST(test_get_clone)
{
    DnfSack *sack = test_globals.sack;
    int max = dnf_sack_last_solvable(sack);

    fail_unless(dnf_packageset_count(pset) == 3);
    DnfPackage *pkg0 = dnf_package_new(pset->getSack(), (*pset)[0]);
    DnfPackage *pkg9 = dnf_package_new(pset->getSack(), (*pset)[1]);
    DnfPackage *pkg_max = dnf_package_new(pset->getSack(), (*pset)[2]);
    fail_unless(dnf_package_get_id(pkg0) == 0);
    fail_unless(dnf_package_get_id(pkg9) == 9);
    fail_unless(dnf_package_get_id(pkg_max) == max);
    //fail_unless(dnf_package_new(pset->getSack(), (*pset)[3]) == NULL);

    g_object_unref(pkg0);
    g_object_unref(pkg9);
    g_object_unref(pkg_max);

    DnfPackage *pkg8 = dnf_package_new(sack, 8);
    DnfPackage *pkg11 = dnf_package_new(sack, 11);
    dnf_packageset_add(pset, pkg8);
    dnf_packageset_add(pset, pkg11);
    g_object_unref(pkg8);
    g_object_unref(pkg11);
    pkg8 = dnf_package_new(pset->getSack(), (*pset)[1]);
    pkg9 = dnf_package_new(pset->getSack(), (*pset)[2]);
    pkg11 = dnf_package_new(pset->getSack(), (*pset)[3]);
    fail_unless(dnf_package_get_id(pkg8) == 8);
    fail_unless(dnf_package_get_id(pkg9) == 9);
    fail_unless(dnf_package_get_id(pkg11) == 11);

    g_object_unref(pkg8);
    g_object_unref(pkg9);
    g_object_unref(pkg11);
}
END_TEST

START_TEST(test_get_pkgid)
{
    DnfSack *sack = test_globals.sack;
    int max = dnf_sack_last_solvable(sack);

    // add some more packages
    DnfPackage *pkg;
    pkg = dnf_package_new(sack, 7);
    dnf_packageset_add(pset, pkg);
    g_object_unref(pkg);
    pkg = dnf_package_new(sack, 8);
    dnf_packageset_add(pset, pkg);
    g_object_unref(pkg);
    pkg = dnf_package_new(sack, 15);
    dnf_packageset_add(pset, pkg);
    g_object_unref(pkg);

    Id id = -1;
    id = pset->next(id);
    fail_unless(id == 0);
    id = pset->next(id);
    fail_unless(id == 7);
    id = pset->next(id);
    fail_unless(id == 8);
    id = pset->next(id);
    fail_unless(id == 9);
    id = pset->next(id);
    fail_unless(id == 15);
    id = (*pset)[5];
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
