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
#include "src/package.h"
#include "fixtures.h"
#include "test_suites.h"
#include "testsys.h"

static HyAdvisoryPkg advisorypkg;

static void
advisorypkg_fixture(void)
{
    fixture_yum();

    HyPackage pkg;
    HyAdvisoryList advisories;
    HyAdvisory advisory;
    HyAdvisoryPkgList pkglist;

    pkg = by_name(test_globals.sack, "tour");
    advisories = hy_package_get_advisories(pkg, HY_GT);
    advisory = hy_advisorylist_get_clone(advisories, 0);
    pkglist = hy_advisory_get_packages(advisory);
    advisorypkg = hy_advisorypkglist_get_clone(pkglist, 0);

    hy_advisorypkglist_free(pkglist);
    hy_advisory_free(advisory);
    hy_advisorylist_free(advisories);
    hy_package_free(pkg);
}

static void
advisorypkg_teardown(void)
{
    hy_advisorypkg_free(advisorypkg);
    teardown();
}

START_TEST(test_name)
{
    ck_assert_str_eq(
	hy_advisorypkg_get_string(advisorypkg, HY_ADVISORYPKG_NAME),
	"tour");
}
END_TEST

START_TEST(test_evr)
{
    ck_assert_str_eq(
	hy_advisorypkg_get_string(advisorypkg, HY_ADVISORYPKG_EVR),
	"4-7");
}
END_TEST

START_TEST(test_arch)
{
    ck_assert_str_eq(
	hy_advisorypkg_get_string(advisorypkg, HY_ADVISORYPKG_ARCH),
	"noarch");
}
END_TEST

START_TEST(test_filename)
{
    ck_assert_str_eq(
	hy_advisorypkg_get_string(advisorypkg, HY_ADVISORYPKG_FILENAME),
	"tour.noarch.rpm");
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
