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
#include "src/advisoryref.h"
#include "src/package.h"
#include "fixtures.h"
#include "test_suites.h"
#include "testsys.h"

static HyAdvisoryRef reference;

static void
advisoryref_fixture(void)
{
    fixture_yum();

    HyPackage pkg;
    HyAdvisoryList advisories;
    HyAdvisory advisory;
    HyAdvisoryRefList reflist;

    pkg = by_name(test_globals.sack, "tour");
    advisories = hy_package_get_advisories(pkg, HY_GT);
    advisory = hy_advisorylist_get_clone(advisories, 0);
    reflist = hy_advisory_get_references(advisory);
    reference = hy_advisoryreflist_get_clone(reflist, 0);

    hy_advisoryreflist_free(reflist);
    hy_advisory_free(advisory);
    hy_advisorylist_free(advisories);
    hy_package_free(pkg);
}

static void
advisoryref_teardown(void)
{
    hy_advisoryref_free(reference);
    teardown();
}

START_TEST(test_type)
{
    ck_assert_int_eq(hy_advisoryref_get_type(reference), HY_REFERENCE_BUGZILLA);
}
END_TEST

START_TEST(test_id)
{
    ck_assert_str_eq(hy_advisoryref_get_id(reference), "472090");
}
END_TEST

START_TEST(test_title)
{
    ck_assert_str_eq(hy_advisoryref_get_title(reference), "/etc/init.d/clvmd points to /usr/sbin for LVM tools");
}
END_TEST

START_TEST(test_url)
{
    ck_assert_str_eq(hy_advisoryref_get_url(reference), "https://bugzilla.redhat.com/show_bug.cgi?id=472090");
}
END_TEST

Suite *
advisoryref_suite(void)
{
    Suite *s = suite_create("AdvisoryRef");
    TCase *tc;

    tc = tcase_create("WithRealRepo");
    tcase_add_unchecked_fixture(tc, advisoryref_fixture, advisoryref_teardown);
    tcase_add_test(tc, test_type);
    tcase_add_test(tc, test_id);
    tcase_add_test(tc, test_title);
    tcase_add_test(tc, test_url);
    suite_add_tcase(s, tc);

    return s;
}
