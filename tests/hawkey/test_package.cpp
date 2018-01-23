/*
 * Copyright (C) 2012-2014 Red Hat, Inc.
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


#include <solv/util.h>


#include "libdnf/dnf-advisory.h"
#include "libdnf/hy-package.h"
#include "libdnf/hy-package-private.hpp"
#include "libdnf/hy-query.h"
#include "libdnf/dnf-reldep.h"
#include "libdnf/dnf-reldep-list.h"
#include "libdnf/dnf-sack-private.hpp"
#include "libdnf/hy-util.h"
#include "fixtures.h"
#include "test_suites.h"
#include "testsys.h"

START_TEST(test_package_summary)
{
    DnfPackage *pkg = by_name(test_globals.sack, "penny-lib");
    fail_if(strcmp(dnf_package_get_summary(pkg), "in my ears"));
    g_object_unref(pkg);
}
END_TEST

START_TEST(test_identical)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg1 = by_name(sack, "penny-lib");
    DnfPackage *pkg2 = by_name(sack, "flying");
    DnfPackage *pkg3 = by_name(sack, "penny-lib");

    fail_unless(dnf_package_get_identical(pkg1, pkg3));
    fail_if(dnf_package_get_identical(pkg2, pkg3));

    g_object_unref(pkg1);
    g_object_unref(pkg2);
    g_object_unref(pkg3);
}
END_TEST

START_TEST(test_versions)
{
    DnfSack *sack = test_globals.sack;
    unsigned epoch;
    const char *version, *release;
    DnfPackage *pkg;

    pkg = by_name(sack, "baby");
    ck_assert_str_eq(dnf_package_get_evr(pkg), "6:5.0-11");
    epoch = dnf_package_get_epoch(pkg);
    fail_unless(epoch == 6);
    version = dnf_package_get_version(pkg);
    ck_assert_str_eq(version, "5.0");
    release = dnf_package_get_release(pkg);
    ck_assert_str_eq(release, "11");
    g_object_unref(pkg);

    pkg = by_name(sack, "jay");
    // epoch missing if it's 0:
    ck_assert_str_eq(dnf_package_get_evr(pkg), "5.0-0");
    epoch = dnf_package_get_epoch(pkg);
    fail_unless(epoch == 0);
    version = dnf_package_get_version(pkg);
    ck_assert_str_eq(version, "5.0");
    release = dnf_package_get_release(pkg);
    ck_assert_str_eq(release, "0");
    g_object_unref(pkg);
}
END_TEST

START_TEST(test_no_sourcerpm)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = by_name(sack, "baby");
    const char *src = dnf_package_get_sourcerpm(pkg);

    fail_unless(src == NULL);
    g_object_unref(pkg);
}
END_TEST

START_TEST(test_get_requires)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = by_name(sack, "flying");
    DnfReldepList *reldeplist = dnf_package_get_requires(pkg);

    fail_unless(dnf_reldep_list_count (reldeplist) == 1);
    DnfReldep *reldep = dnf_reldep_list_index (reldeplist, 0);

    const char *depstr = dnf_reldep_to_string (reldep);
    ck_assert_str_eq(depstr, "P-lib >= 3");

    delete[] depstr;
    delete reldep;
    delete reldeplist;
    g_object_unref(pkg);
}
END_TEST

START_TEST(test_get_more_requires)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = by_name(sack, "walrus");
    DnfReldepList *reldeplist = dnf_package_get_requires(pkg);

    fail_unless(dnf_reldep_list_count (reldeplist) == 2);
    g_object_unref(reldeplist);
    g_object_unref(pkg);
}
END_TEST

START_TEST(test_chksum_fail)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = by_name(sack, "walrus");
    int type;

    const unsigned char *chksum = dnf_package_get_chksum(pkg, &type);
    fail_unless(chksum == NULL);
    chksum = dnf_package_get_hdr_chksum(pkg, &type);
    fail_unless(chksum == NULL);
    g_object_unref(pkg);
}
END_TEST

START_TEST(test_checksums)
{
    DnfPackage *pkg = by_name(test_globals.sack, "mystery-devel");
    int i;
    HyChecksum *csum = dnf_package_get_chksum(pkg, &i);
    fail_unless(i == G_CHECKSUM_SHA256);
    // Check the first and last bytes. Those need to match against information
    // in primary.xml.gz.
    fail_unless(csum[0] == 0x2e);
    fail_unless(csum[31] == 0xf5);

    g_object_unref(pkg);
}
END_TEST

START_TEST(test_get_files)
{
    DnfSack *sack = test_globals.sack;

    DnfPackage *pkg = by_name(sack, "tour");
    gchar **files = dnf_package_get_files(pkg);
    int i = 0;
    char **iter;

    for (iter = files; iter && *iter; iter++)
        i++;
    g_assert_cmpint(i, ==, 6);
    g_strfreev(files);
    g_object_unref(pkg);
}
END_TEST

START_TEST(test_get_advisories)
{
    GPtrArray *advisories;
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = by_name(sack, "tour");

    advisories = dnf_package_get_advisories(pkg, HY_GT);
    fail_unless(advisories != NULL);
    ck_assert_int_eq(advisories->len, 1);
    auto advisory = static_cast<DnfAdvisory *>(g_ptr_array_index(advisories, 0));
    ck_assert_str_eq(dnf_advisory_get_id(advisory), "FEDORA-2008-9969");
    g_ptr_array_unref(advisories);
    g_object_unref(pkg);
}
END_TEST

START_TEST(test_get_advisories_none)
{
    GPtrArray *advisories;
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = by_name(sack, "mystery-devel");

    advisories = dnf_package_get_advisories(pkg, HY_GT|HY_EQ);
    fail_unless(advisories != NULL);
    ck_assert_int_eq(advisories->len, 1);
    g_ptr_array_unref(advisories);

    advisories = dnf_package_get_advisories(pkg, HY_LT|HY_EQ);
    fail_unless(advisories != NULL);
    ck_assert_int_eq(advisories->len, 1);
    g_ptr_array_unref(advisories);

    g_object_unref(pkg);
}
END_TEST

START_TEST(test_lookup_num)
{
    DnfPackage *pkg = by_name(test_globals.sack, "tour");
    guint64 buildtime = dnf_package_get_buildtime(pkg);
    fail_unless(buildtime > 1330473600); // after 2012-02-29
    fail_unless(buildtime < 1456704000); // before 2016-02-29

    g_object_unref(pkg);
}
END_TEST

START_TEST(test_installed)
{
    DnfPackage *pkg1 = by_name_repo(test_globals.sack, "penny-lib", "main");
    DnfPackage *pkg2 = by_name_repo(test_globals.sack,
                                  "penny-lib", HY_SYSTEM_REPO_NAME);
    int installed1 = dnf_package_installed(pkg1);
    int installed2 = dnf_package_installed(pkg2);
    fail_unless(installed1 == 0);
    fail_unless(installed2 == 1);

    g_object_unref(pkg1);
    g_object_unref(pkg2);
}
END_TEST

START_TEST(test_two_sacks)
{
    /* This clumsily mimics create_ut_sack() and setup_with() to
     * create a second DnfSack. */
    char *tmpdir = solv_dupjoin(test_globals.tmpdir, "/tmp", NULL);
    DnfSack *sack1 = dnf_sack_new();
    dnf_sack_set_arch(sack1, TEST_FIXED_ARCH, NULL);
    dnf_sack_set_cachedir(sack1, tmpdir);
    fail_unless(dnf_sack_setup(sack1, DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR, NULL));
    Pool *pool1 = dnf_sack_get_pool(sack1);
    const char *path = pool_tmpjoin(pool1, test_globals.repo_dir,
                                    "change.repo", NULL);
    fail_if(load_repo(pool1, "change", path, 0));
    DnfPackage *pkg1 = by_name(sack1, "penny-lib");
    fail_if(pkg1 == NULL);

    DnfSack *sack2 = test_globals.sack;
    Pool *pool2 = dnf_sack_get_pool(sack2);
    DnfPackage *pkg2 = by_name(sack2, "penny-lib");
    fail_if(pkg2 == NULL);

    /* "penny-lib" is in both pools but at different offsets */
    Solvable *s1 = pool_id2solvable(pool1, dnf_package_get_id(pkg1));
    Solvable *s2 = pool_id2solvable(pool2, dnf_package_get_id(pkg2));
    fail_if(s1->name == s2->name);

    fail_if(dnf_package_cmp(pkg1, pkg2) != 0);

    g_object_unref(pkg1);
    g_object_unref(pkg2);

    g_object_unref(sack1);
    g_free(tmpdir);
}
END_TEST

START_TEST(test_packager)
{
    DnfPackage *pkg = by_name(test_globals.sack, "tour");
    ck_assert_str_eq(dnf_package_get_packager(pkg), "roll up <roll@up.net>");
    g_object_unref(pkg);
}
END_TEST

START_TEST(test_sourcerpm)
{
    DnfPackage *pkg = by_name(test_globals.sack, "tour");
    const char *sourcerpm = dnf_package_get_sourcerpm(pkg);

    ck_assert_str_eq(sourcerpm, "tour-4-6.src.rpm");
    g_object_unref(pkg);

    pkg = by_name(test_globals.sack, "mystery-devel");
    sourcerpm = dnf_package_get_sourcerpm(pkg);
    ck_assert_str_eq(sourcerpm, "mystery-19.67-1.src.rpm");
    g_object_unref(pkg);
}
END_TEST

#define TOUR_45_46_DRPM_CHKSUM "\xc3\xc3\xd5\x72\xa4\x6b"\
    "\x1a\x66\x90\x6d\x42\xca\x17\x63\xef\x36\x20\xf7\x02"\
    "\x58\xaa\xac\x4c\x14\xbf\x46\x3e\xd5\x37\x16\xd4\x44"

START_TEST(test_presto)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *tour = by_name(sack, "tour");
    fail_if(tour == NULL);

    DnfPackageDelta *delta = dnf_package_get_delta_from_evr(tour, "4-5");
    const char *location = dnf_packagedelta_get_location(delta);
    ck_assert_str_eq(location, "drpms/tour-4-5_4-6.noarch.drpm");
    const char *baseurl = dnf_packagedelta_get_baseurl(delta);
    fail_unless(baseurl == NULL);
    guint64 size = dnf_packagedelta_get_downloadsize(delta);
    ck_assert_int_eq(size, 3132);
    int type;
    HyChecksum *csum = dnf_packagedelta_get_chksum(delta, &type);
    fail_unless(type == G_CHECKSUM_SHA256);
    ck_assert(!memcmp(csum, TOUR_45_46_DRPM_CHKSUM, 32));
    g_object_unref(delta);
    g_object_unref(tour);
}
END_TEST

START_TEST(test_get_files_cmdline)
{
    DnfSack *sack = test_globals.sack;

    DnfPackage *pkg = by_name(sack, "tour");
    gchar **files;

    files = dnf_package_get_files(pkg);
    g_assert_cmpint (6, ==, g_strv_length(files));
    g_strfreev(files);
    g_object_unref(pkg);
}
END_TEST

Suite *
package_suite(void)
{
    Suite *s = suite_create("Package");
    TCase *tc;

    tc = tcase_create("Core");
    tcase_add_unchecked_fixture(tc, fixture_system_only, teardown);
    tcase_add_test(tc, test_package_summary);
    tcase_add_test(tc, test_identical);
    tcase_add_test(tc, test_versions);
    tcase_add_test(tc, test_no_sourcerpm);
    suite_add_tcase(s, tc);

    tc = tcase_create("Requires");
    tcase_add_unchecked_fixture(tc, fixture_with_main, teardown);
    tcase_add_test(tc, test_get_requires);
    tcase_add_test(tc, test_get_more_requires);
    tcase_add_test(tc, test_chksum_fail);
    tcase_add_test(tc, test_installed);
    tcase_add_test(tc, test_two_sacks);
    suite_add_tcase(s, tc);

    tc = tcase_create("WithRealRepo");
    tcase_add_unchecked_fixture(tc, fixture_yum, teardown);
    tcase_add_test(tc, test_checksums);
    tcase_add_test(tc, test_get_files);
    tcase_add_test(tc, test_get_advisories);
    tcase_add_test(tc, test_get_advisories_none);
    tcase_add_test(tc, test_lookup_num);
    tcase_add_test(tc, test_packager);
    tcase_add_test(tc, test_sourcerpm);
    tcase_add_test(tc, test_presto);
    suite_add_tcase(s, tc);

    tc = tcase_create("WithCmdlinePackage");
    tcase_add_unchecked_fixture(tc, fixture_cmdline_only, teardown);
    tcase_add_test(tc, test_get_files_cmdline);
    suite_add_tcase(s, tc);

    return s;
}
