/*
 * Copyright (C) 2012-2015 Red Hat, Inc.
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>


#include <solv/testcase.h>

#include <glib/gstdio.h>

#include <libdnf/repo/Repo-private.hpp>
#include "libdnf/dnf-types.h"
#include "libdnf/hy-package-private.hpp"
#include "libdnf/hy-repo-private.hpp"
#include "libdnf/dnf-sack-private.hpp"
#include "libdnf/hy-util.h"
#include "libdnf/hy-iutil-private.hpp"
#include "fixtures.h"
#include "testsys.h"
#include "test_suites.h"

START_TEST(test_environment)
{
    /* check the tmpdir was created */
    fail_if(access(test_globals.tmpdir, W_OK));
}
END_TEST

START_TEST(test_sack_create)
{
    g_autoptr(DnfSack) sack = dnf_sack_new();
    dnf_sack_set_cachedir(sack, test_globals.tmpdir);
    fail_unless(dnf_sack_setup(sack, DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR, NULL));
    fail_if(sack == NULL, NULL);
    fail_if(dnf_sack_get_pool(sack) == NULL, NULL);
}
END_TEST

START_TEST(test_give_cache_fn)
{
    DnfSack *sack = dnf_sack_new();
    dnf_sack_set_cachedir(sack, test_globals.tmpdir);
    fail_unless(dnf_sack_setup(sack, DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR, NULL));

    char *path = dnf_sack_give_cache_fn(sack, "rain", NULL);
    fail_if(strstr(path, "rain.solv") == NULL);
    g_free(path);

    path = dnf_sack_give_cache_fn(sack, "rain", HY_EXT_FILENAMES);
    fail_if(strstr(path, "rain-filenames.solvx") == NULL);
    g_free(path);
    g_object_unref(sack);
}
END_TEST

START_TEST(test_list_arches)
{
    DnfSack *sack = dnf_sack_new();
    dnf_sack_set_cachedir(sack, test_globals.tmpdir);
    dnf_sack_set_arch(sack, TEST_FIXED_ARCH, NULL);
    fail_unless(dnf_sack_setup(sack, DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR, NULL));
    const char ** arches = dnf_sack_list_arches(sack);

    /* noarch, x86_64, athlon, i686, i586, i486, i386 */
    fail_unless(g_strv_length((gchar**)arches) >= 6 && g_strv_length((gchar**)arches) <= 7);

    if (g_strv_length((gchar**)arches) == 7) {
        // Fedora, Mageia
        ck_assert_str_eq(arches[0], "noarch");
        ck_assert_str_eq(arches[1], "x86_64");
        ck_assert_str_eq(arches[2], "athlon");
        ck_assert_str_eq(arches[3], "i686");
        ck_assert_str_eq(arches[4], "i586");
        ck_assert_str_eq(arches[5], "i486");
        ck_assert_str_eq(arches[6], "i386");
    } else {
        // openSUSE, Debian - "athlon" is not available
        ck_assert_str_eq(arches[0], "noarch");
        ck_assert_str_eq(arches[1], "x86_64");
        ck_assert_str_eq(arches[2], "i686");
        ck_assert_str_eq(arches[3], "i586");
        ck_assert_str_eq(arches[4], "i486");
        ck_assert_str_eq(arches[5], "i386");
    }

    g_free(arches);
    g_object_unref(sack);
}
END_TEST

START_TEST(test_load_repo_err)
{
    g_autoptr(GError) error = NULL;
    DnfSack *sack = dnf_sack_new();
    dnf_sack_set_cachedir(sack, test_globals.tmpdir);
    fail_unless(dnf_sack_setup(sack, DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR, &error));
    g_assert(sack != NULL);
    HyRepo repo = hy_repo_create("crabalocker");
    g_assert(repo != NULL);
    hy_repo_set_string(repo, HY_REPO_MD_FN, "/non/existing");
    fail_unless(!dnf_sack_load_repo(sack, repo, 0, &error));
    fail_unless(g_error_matches (error, DNF_ERROR, DNF_ERROR_FILE_INVALID));
    hy_repo_free(repo);
    g_object_unref(sack);
}
END_TEST

START_TEST(test_repo_written)
{
    DnfSack *sack = dnf_sack_new();
    dnf_sack_set_cachedir(sack, test_globals.tmpdir);
    fail_unless(dnf_sack_setup(sack, DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR, NULL));
    char *filename = dnf_sack_give_cache_fn(sack, "test_sack_written", NULL);

    fail_unless(access(filename, R_OK|W_OK));
    setup_yum_sack(sack, "test_sack_written");

    HyRepo repo = hrepo_by_name(sack, "test_sack_written");
    auto repoImpl = libdnf::repoGetImpl(repo);
    fail_if(repo == NULL);
    fail_unless(repoImpl->state_main == _HY_WRITTEN);
    fail_unless(repoImpl->state_filelists == _HY_WRITTEN);
    fail_unless(repoImpl->state_presto == _HY_WRITTEN);
    fail_if(access(filename, R_OK|W_OK));

    g_free(filename);
    g_object_unref(sack);
}
END_TEST

START_TEST(test_add_cmdline_package)
{
    g_autoptr(DnfSack) sack = dnf_sack_new();
    dnf_sack_set_cachedir(sack, test_globals.tmpdir);

    g_autofree gchar *path_mystery = g_build_filename (TESTDATADIR, "/hawkey/yum/mystery-devel-19.67-1.noarch.rpm", NULL);
    g_autoptr(DnfPackage) pkg_mystery = dnf_sack_add_cmdline_package (sack, path_mystery);
    const gchar *location_mystery = dnf_package_get_location(pkg_mystery);
    ck_assert_str_eq(path_mystery, location_mystery);

    g_autofree gchar *path_tour = g_build_filename (TESTDATADIR, "/hawkey/yum/tour-4-6.noarch.rpm", NULL);
    g_autoptr(DnfPackage) pkg_tour = dnf_sack_add_cmdline_package (sack, path_tour);
    const gchar *location_tour = dnf_package_get_location(pkg_tour);
    ck_assert_str_eq(path_tour, location_tour);

    g_autofree gchar *path_null_rpm = g_build_filename (test_globals.tmpdir, "null.rpm", NULL);
    FILE *fp = g_fopen (path_null_rpm, "w");
    fail_unless (fp != NULL);
    fclose (fp);
    fail_unless (dnf_sack_add_cmdline_package (sack, path_null_rpm) == NULL);
}
END_TEST

START_TEST(test_repo_load)
{
    fail_unless(dnf_sack_count(test_globals.sack) ==
                TEST_EXPECT_SYSTEM_NSOLVABLES);
}
END_TEST

static void
check_filelist(Pool *pool)
{
    Dataiterator di;
    int count;
    Id last_found_solvable = 0;
    dataiterator_init(&di, pool, 0, 0, SOLVABLE_FILELIST, "/usr/bin/ste",
                      SEARCH_STRING | SEARCH_FILES | SEARCH_COMPLETE_FILELIST);
    for (count = 0; dataiterator_step(&di); ++count)
        last_found_solvable = di.solvid;
    fail_unless(count == 1);
    fail_if(last_found_solvable == 0);
    dataiterator_free(&di);

    dataiterator_init(&di, pool, 0, last_found_solvable, SOLVABLE_FILELIST, "/",
                      SEARCH_STRINGSTART | SEARCH_FILES);
    for (count = 0; dataiterator_step(&di); ++count)
        fail_if(strncmp(di.kv.str, "/usr/bin/", strlen("/usr/bin/")));
    fail_unless(count == 3);
    dataiterator_free(&di);

    dataiterator_init(&di, pool, 0, 0, SOLVABLE_FILELIST,
                      "/usr/lib/python2.7/site-packages/tour/today.pyc",
                      SEARCH_STRING | SEARCH_FILES | SEARCH_COMPLETE_FILELIST);
    for (count = 0; dataiterator_step(&di); ++count) ;
    fail_unless(count == 1);
    dataiterator_free(&di);
}

START_TEST(test_filelist)
{
    DnfSack *sack = test_globals.sack;
    HyRepo repo = hrepo_by_name(sack, YUM_REPO_NAME);
    char *fn_solv = dnf_sack_give_cache_fn(sack, YUM_REPO_NAME, HY_EXT_FILENAMES);

    fail_unless(libdnf::repoGetImpl(repo)->state_filelists == _HY_WRITTEN);
    fail_if(access(fn_solv, R_OK));
    g_free(fn_solv);

    check_filelist(dnf_sack_get_pool(test_globals.sack));
}
END_TEST

START_TEST(test_filelist_from_cache)
{
    DnfSack *sack = dnf_sack_new();
    dnf_sack_set_cachedir(sack, test_globals.tmpdir);
    fail_unless(dnf_sack_setup(sack, DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR, NULL));
    setup_yum_sack(sack, YUM_REPO_NAME);

    HyRepo repo = hrepo_by_name(sack, YUM_REPO_NAME);
    fail_unless(libdnf::repoGetImpl(repo)->state_filelists == _HY_LOADED_CACHE);
    check_filelist(dnf_sack_get_pool(sack));
    g_object_unref(sack);
}
END_TEST

static void
check_prestoinfo(Pool *pool)
{
    Dataiterator di;

    dataiterator_init(&di, pool, NULL, SOLVID_META, DELTA_PACKAGE_NAME, "tour",
                      SEARCH_STRING);
    dataiterator_prepend_keyname(&di, REPOSITORY_DELTAINFO);
    fail_unless(dataiterator_step(&di));
    dataiterator_setpos_parent(&di);
    const char *attr;
    attr = pool_lookup_str(pool, SOLVID_POS, DELTA_SEQ_NAME);
    ck_assert_str_eq(attr, "tour");
    attr = pool_lookup_str(pool, SOLVID_POS, DELTA_SEQ_EVR);
    ck_assert_str_eq(attr, "4-5");
    attr = pool_lookup_str(pool, SOLVID_POS, DELTA_LOCATION_DIR);
    ck_assert_str_eq(attr, "drpms");
    dataiterator_free(&di);
    return;
}

START_TEST(test_presto)
{
    DnfSack *sack = test_globals.sack;
    HyRepo repo = hrepo_by_name(sack, YUM_REPO_NAME);
    char *fn_solv = dnf_sack_give_cache_fn(sack, YUM_REPO_NAME, HY_EXT_PRESTO);

    fail_if(access(fn_solv, R_OK));
    fail_unless(libdnf::repoGetImpl(repo)->state_presto == _HY_WRITTEN);
    g_free(fn_solv);
    check_prestoinfo(dnf_sack_get_pool(sack));
}
END_TEST

START_TEST(test_presto_from_cache)
{
    DnfSack *sack = dnf_sack_new();
    dnf_sack_set_cachedir(sack, test_globals.tmpdir);
    dnf_sack_set_arch(sack, TEST_FIXED_ARCH, NULL);
    fail_unless(dnf_sack_setup(sack, DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR, NULL));
    setup_yum_sack(sack, YUM_REPO_NAME);

    HyRepo repo = hrepo_by_name(sack, YUM_REPO_NAME);
    fail_unless(libdnf::repoGetImpl(repo)->state_presto == _HY_LOADED_CACHE);
    check_prestoinfo(dnf_sack_get_pool(sack));
    g_object_unref(sack);
}
END_TEST

Suite *
sack_suite(void)
{
    Suite *s = suite_create("Sack");
    TCase *tc = tcase_create("Core");
    tcase_add_test(tc, test_environment);
    tcase_add_test(tc, test_sack_create);
    tcase_add_test(tc, test_give_cache_fn);
    tcase_add_test(tc, test_list_arches);
    tcase_add_test(tc, test_load_repo_err);
    tcase_add_test(tc, test_repo_written);
    tcase_add_test(tc, test_add_cmdline_package);
    suite_add_tcase(s, tc);

    tc = tcase_create("Repos");
    tcase_add_unchecked_fixture(tc, fixture_system_only, teardown);
    tcase_add_test(tc, test_repo_load);
    suite_add_tcase(s, tc);

    tc = tcase_create("YumRepo");
    tcase_add_unchecked_fixture(tc, fixture_yum, teardown);
    tcase_add_test(tc, test_filelist);
    tcase_add_test(tc, test_filelist_from_cache);
    tcase_add_test(tc, test_presto);
    tcase_add_test(tc, test_presto_from_cache);
    suite_add_tcase(s, tc);

    tc = tcase_create("SackKnows");
    tcase_add_unchecked_fixture(tc, fixture_all, teardown);
    suite_add_tcase(s, tc);

    return s;
}
