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

// hawkey
#include "src/repo_internal.h"
#include "test_suites.h"

START_TEST(test_strings)
{
    HyRepo hrepo = hy_repo_create("happy2");
    ck_assert_str_eq(hy_repo_get_string(hrepo, HY_REPO_NAME), "happy2");
    hy_repo_set_string(hrepo, HY_REPO_PRESTO_FN, "tunedtoA");
    ck_assert_str_eq(hy_repo_get_string(hrepo, HY_REPO_PRESTO_FN), "tunedtoA");
    hy_repo_set_string(hrepo, HY_REPO_PRESTO_FN, "naturalE");
    ck_assert_str_eq(hy_repo_get_string(hrepo, HY_REPO_PRESTO_FN), "naturalE");
    hy_repo_free(hrepo);
}
END_TEST

Suite *
repo_suite(void)
{
    Suite *s = suite_create("Repo");
    TCase *tc = tcase_create("Core");
    tcase_add_test(tc, test_strings);
    suite_add_tcase(s, tc);

    return s;
}
