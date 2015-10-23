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


// hawkey
#include "libhif/hif-advisory.h"
#include "libhif/hif-advisorypkg.h"
#include "libhif/hif-advisoryref.h"
#include "libhif/hy-package.h"
#include "fixtures.h"
#include "test_suites.h"
#include "testsys.h"

static HifAdvisory *advisory;

static void
advisory_fixture(void)
{
    fixture_yum();

    HyPackage pkg;
    GPtrArray *advisories;

    pkg = by_name(test_globals.sack, "tour");
    advisories = hy_package_get_advisories(pkg, HY_GT);

    advisory = g_object_ref(g_ptr_array_index(advisories, 0));

    g_ptr_array_unref(advisories);
    hy_package_free(pkg);
}

static void
advisory_teardown(void)
{
    g_object_unref(advisory);
    teardown();
}

START_TEST(test_title)
{
    ck_assert_str_eq(hif_advisory_get_title(advisory), "lvm2-2.02.39-7.fc10");
}
END_TEST

START_TEST(test_id)
{
    ck_assert_str_eq(hif_advisory_get_id(advisory), "FEDORA-2008-9969");
}
END_TEST

START_TEST(test_type)
{
    ck_assert_int_eq(hif_advisory_get_kind(advisory), HIF_ADVISORY_KIND_BUGFIX);
}
END_TEST

START_TEST(test_description)
{
    ck_assert_str_eq(
            hif_advisory_get_description(advisory),
            "An example update to the tour package.");
}
END_TEST

START_TEST(test_rights)
{
    fail_if(hif_advisory_get_rights(advisory));
}
END_TEST

START_TEST(test_updated)
{
    ck_assert_int_eq(hif_advisory_get_updated(advisory), 1228822286);
}
END_TEST

START_TEST(test_packages)
{
    GPtrArray *pkglist = hif_advisory_get_packages(advisory);

    ck_assert_int_eq(pkglist->len, 1);
    HifAdvisoryPkg *package = g_ptr_array_index(pkglist, 0);
    ck_assert_str_eq(
            hif_advisorypkg_get_filename(package),
            "tour.noarch.rpm");

    g_ptr_array_unref(pkglist);
}
END_TEST

START_TEST(test_refs)
{
    HifAdvisoryRef *reference;
    GPtrArray *reflist = hif_advisory_get_references(advisory);

    ck_assert_int_eq(reflist->len, 2);
    reference = g_ptr_array_index(reflist, 0);
    ck_assert_str_eq(
            hif_advisoryref_get_url(reference),
            "https://bugzilla.redhat.com/show_bug.cgi?id=472090");
    reference = g_ptr_array_index(reflist, 1);
    ck_assert_str_eq(
            hif_advisoryref_get_url(reference),
            "https://bugzilla.gnome.com/show_bug.cgi?id=472091");

    g_ptr_array_unref(reflist);
}
END_TEST

Suite *
advisory_suite(void)
{
    Suite *s = suite_create("Advisory");
    TCase *tc;

    tc = tcase_create("WithRealRepo");
    tcase_add_unchecked_fixture(tc, advisory_fixture, advisory_teardown);
    tcase_add_test(tc, test_title);
    tcase_add_test(tc, test_id);
    tcase_add_test(tc, test_type);
    tcase_add_test(tc, test_description);
    tcase_add_test(tc, test_rights);
    tcase_add_test(tc, test_updated);
    tcase_add_test(tc, test_packages);
    tcase_add_test(tc, test_refs);
    suite_add_tcase(s, tc);

    return s;
}
