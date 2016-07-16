/*
 * Copyright (C) 2014 Red Hat, Inc.
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



#include "libdnf/dnf-advisory.h"
#include "libdnf/dnf-advisorypkg.h"
#include "libdnf/hy-package.h"
#include "fixtures.h"
#include "test_suites.h"
#include "testsys.h"

static DnfAdvisoryPkg *advisorypkg;

static void
advisorypkg_fixture(void)
{
    fixture_yum();

    DnfPackage *pkg;
    GPtrArray *advisories;
    DnfAdvisory *advisory;
    GPtrArray *pkglist;

    pkg = by_name(test_globals.sack, "tour");
    advisories = dnf_package_get_advisories(pkg, HY_GT);
    advisory = g_ptr_array_index(advisories, 0);
    pkglist = dnf_advisory_get_packages(advisory);
    advisorypkg = g_object_ref(g_ptr_array_index(pkglist, 0));

    g_ptr_array_unref(pkglist);
    g_ptr_array_unref(advisories);
    g_object_unref(pkg);
}

static void
advisorypkg_teardown(void)
{
    g_object_unref(advisorypkg);
    teardown();
}

START_TEST(test_name)
{
    ck_assert_str_eq(dnf_advisorypkg_get_name(advisorypkg), "tour");
}
END_TEST

START_TEST(test_evr)
{
    ck_assert_str_eq(dnf_advisorypkg_get_evr(advisorypkg), "4-7");
}
END_TEST

START_TEST(test_arch)
{
    ck_assert_str_eq(dnf_advisorypkg_get_arch(advisorypkg), "noarch");
}
END_TEST

START_TEST(test_filename)
{
    ck_assert_str_eq(dnf_advisorypkg_get_filename(advisorypkg), "tour.noarch.rpm");
}
END_TEST

Suite *
advisorypkg_suite(void)
{
    Suite *s = suite_create("AdvisoryPkg");
    TCase *tc;

    tc = tcase_create("WithRealRepo");
    tcase_add_unchecked_fixture(tc, advisorypkg_fixture, advisorypkg_teardown);
    tcase_add_test(tc, test_name);
    tcase_add_test(tc, test_evr);
    tcase_add_test(tc, test_arch);
    tcase_add_test(tc, test_filename);
    suite_add_tcase(s, tc);

    return s;
}
