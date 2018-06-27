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



#include "libdnf/sack/advisory.hpp"
#include "libdnf/sack/advisorypkg.hpp"
#include "libdnf/dnf-advisory.h"
#include "libdnf/dnf-advisorypkg.h"
#include "libdnf/hy-package.h"
#include "libdnf/repo/RpmPackage.hpp"
#include "fixtures.h"
#include "test_suites.h"
#include "testsys.h"

static std::shared_ptr<libdnf::Advisory> advisory;
static libdnf::RpmPackage *package;

static void
advisorypkg_fixture(void)
{
    fixture_yum();

    package = by_name(test_globals.sack, "tour");
    auto advisories = package->getAdvisories(HY_GT);
    advisory = advisories.at(0);
}

static void
advisorypkg_teardown(void)
{
    delete package;
    teardown();
}

START_TEST(test_name)
{

    ck_assert_str_eq(advisory->getPackages().at(0).getNameString(), "tour");
}
END_TEST

START_TEST(test_evr)
{
    ck_assert_str_eq(advisory->getPackages().at(0).getEVRString(), "4-7");
}
END_TEST

START_TEST(test_arch)
{
    ck_assert_str_eq(advisory->getPackages().at(0).getArchString(), "noarch");
}
END_TEST

START_TEST(test_filename)
{
    ck_assert_str_eq(advisory->getPackages().at(0).getFileName(), "tour.noarch.rpm");
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
