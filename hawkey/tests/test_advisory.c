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
#include "src/advisory.h"
#include "src/advisorypkg.h"
#include "src/advisoryref.h"
#include "src/package.h"
#include "fixtures.h"
#include "test_suites.h"
#include "testsys.h"

static HyAdvisory advisory;

static void
advisory_fixture(void)
{
    fixture_yum();

    HyPackage pkg;
    HyAdvisoryList advisories;

    pkg = by_name(test_globals.sack, "tour");
    advisories = hy_package_get_advisories(pkg, HY_GT);

    advisory = hy_advisorylist_get_clone(advisories, 0);

    hy_advisorylist_free(advisories);
    hy_package_free(pkg);
}

static void
advisory_teardown(void)
{
    hy_advisory_free(advisory);
    teardown();
}

START_TEST(test_title)
{
    ck_assert_str_eq(hy_advisory_get_title(advisory), "lvm2-2.02.39-7.fc10");
}
END_TEST

START_TEST(test_id)
{
    ck_assert_str_eq(hy_advisory_get_id(advisory), "FEDORA-2008-9969");
}
END_TEST

START_TEST(test_type)
{
    ck_assert_int_eq(hy_advisory_get_type(advisory), HY_ADVISORY_BUGFIX);
}
END_TEST

START_TEST(test_description)
{
    ck_assert_str_eq(
	    hy_advisory_get_description(advisory),
	    "An example update to the tour package.");
}
END_TEST

START_TEST(test_rights)
{
    fail_if(hy_advisory_get_rights(advisory));
}
END_TEST

START_TEST(test_updated)
{
    ck_assert_int_eq(hy_advisory_get_updated(advisory), 1228822286);
}
END_TEST

START_TEST(test_packages)
{
    HyAdvisoryPkgList pkglist = hy_advisory_get_packages(advisory);

    ck_assert_int_eq(hy_advisorypkglist_count(pkglist), 1);
    HyAdvisoryPkg package = hy_advisorypkglist_get_clone(pkglist, 0);
    ck_assert_str_eq(
	    hy_advisorypkg_get_string(package, HY_ADVISORYPKG_FILENAME),
	    "tour.noarch.rpm");
    hy_advisorypkg_free(package);

    hy_advisorypkglist_free(pkglist);
}
END_TEST

START_TEST(test_refs)
{
    HyAdvisoryRef reference;
    HyAdvisoryRefList reflist = hy_advisory_get_references(advisory);

    ck_assert_int_eq(hy_advisoryreflist_count(reflist), 2);
    reference = hy_advisoryreflist_get_clone(reflist, 0);
    ck_assert_str_eq(
	    hy_advisoryref_get_url(reference),
	    "https://bugzilla.redhat.com/show_bug.cgi?id=472090");
    hy_advisoryref_free(reference);
    reference = hy_advisoryreflist_get_clone(reflist, 1);
    ck_assert_str_eq(
	    hy_advisoryref_get_url(reference),
	    "https://bugzilla.gnome.com/show_bug.cgi?id=472091");
    hy_advisoryref_free(reference);

    hy_advisoryreflist_free(reflist);
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
