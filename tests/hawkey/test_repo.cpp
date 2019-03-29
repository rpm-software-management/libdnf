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

#include <check.h>
#include <string.h>

#include "libdnf/hy-iutil-private.hpp"
#include "libdnf/hy-repo-private.hpp"
#include "libdnf/repo/Repo-private.hpp"
#include "fixtures.h"
#include "testshared.h"
#include "test_suites.h"

START_TEST(test_strings)
{
    HyRepo hrepo = hy_repo_create("happy2");
    auto repoImpl = libdnf::repoGetImpl(hrepo);
    ck_assert_str_eq(hrepo->getId().c_str(), "happy2");
    repoImpl->metadataPaths[MD_TYPE_PRESTODELTA] = "tunedtoA";
    ck_assert_str_eq(repoImpl->getMetadataPath(MD_TYPE_PRESTODELTA).c_str(), "tunedtoA");
    repoImpl->metadataPaths[MD_TYPE_PRESTODELTA] = "naturalE";
    ck_assert_str_eq(repoImpl->getMetadataPath(MD_TYPE_PRESTODELTA).c_str(), "naturalE");
    hy_repo_free(hrepo);
}
END_TEST

START_TEST(test_cost)
{
    DnfSack *sack = test_globals.sack;
    HyRepo repo = hrepo_by_name(sack, YUM_REPO_NAME);
    auto repoImpl = libdnf::repoGetImpl(repo);
    repoImpl->conf->cost().set(libdnf::Option::Priority::RUNTIME, 700);
    if (repoImpl->libsolvRepo)
        repoImpl->libsolvRepo->subpriority = -700;
    fail_unless(repoImpl->libsolvRepo != NULL);
    fail_unless(700 == repoImpl->conf->cost().getValue());
    int subpriority = -700;
    fail_unless(repoImpl->libsolvRepo->subpriority == subpriority);
}
END_TEST

Suite *
repo_suite(void)
{
    Suite *s = suite_create("Repo");
    TCase *tc = tcase_create("Core");
    tcase_add_test(tc, test_strings);
    suite_add_tcase(s, tc);

    tc = tcase_create("Cost");
    tcase_add_unchecked_fixture(tc, fixture_yum, teardown);
    tcase_add_test(tc, test_cost);
    suite_add_tcase(s, tc);

    return s;
}
