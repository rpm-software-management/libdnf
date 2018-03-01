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
#include "libdnf/sack/advisoryref.hpp"
#include "libdnf/dnf-advisory.h"
#include "libdnf/dnf-advisoryref.h"
#include "libdnf/hy-package.h"
#include "libdnf/repo/RpmPackage.hpp"
#include "fixtures.h"
#include "test_suites.h"
#include "testsys.h"

static std::shared_ptr<libdnf::Advisory> advisory;
static libdnf::RpmPackage *package;

static void
advisoryref_fixture(void)
{
    fixture_yum();

    package = by_name(test_globals.sack, "tour");
    auto advisories = package->getAdvisories(HY_GT);
    advisory = advisories.at(0);
}

static void
advisoryref_teardown(void)
{
    delete package;
    teardown();
}

START_TEST(test_type)
{
    ck_assert_int_eq(dnf_advisoryref_get_kind(&advisory->getReferences().at(0)), DNF_REFERENCE_KIND_BUGZILLA);
}
END_TEST

START_TEST(test_id)
{
    ck_assert_str_eq(dnf_advisoryref_get_id(&advisory->getReferences().at(0)), "472090");
}
END_TEST

START_TEST(test_title)
{
    ck_assert_str_eq(dnf_advisoryref_get_title(&advisory->getReferences().at(0)), "/etc/init.d/clvmd points to /usr/sbin for LVM tools");
}
END_TEST

START_TEST(test_url)
{
    ck_assert_str_eq(dnf_advisoryref_get_url(&advisory->getReferences().at(0)), "https://bugzilla.redhat.com/show_bug.cgi?id=472090");
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
