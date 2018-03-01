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
#include "libdnf/hy-query.h"
#include "libdnf/dnf-reldep.h"
#include "libdnf/dnf-reldep-list.h"
#include "libdnf/dnf-sack-private.hpp"
#include "libdnf/hy-util.h"
#include "fixtures.h"
#include "test_suites.h"
#include "testsys.h"

#include "libdnf/repo/solvable/Dependency.hpp"
#include "libdnf/repo/solvable/DependencyContainer.hpp"
#include "libdnf/sack/advisory.hpp"
#include "libdnf/repo/RpmPackage.hpp"

START_TEST(test_package_summary)
{
    DnfPackage *pkg = by_name(test_globals.sack, "penny-lib");
    fail_if(strcmp(pkg->getSummary(), "in my ears") != 0);
    delete pkg;
}
END_TEST

START_TEST(test_identical)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg1 = by_name(sack, "penny-lib");
    DnfPackage *pkg2 = by_name(sack, "flying");
    DnfPackage *pkg3 = by_name(sack, "penny-lib");

    fail_unless(*pkg1 == *pkg3);
    fail_if(*pkg2 == *pkg3);

    delete pkg1;
    delete pkg2;
    delete pkg3;
}
END_TEST

START_TEST(test_versions)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg;

    pkg = by_name(sack, "baby");
    ck_assert_str_eq(pkg->getEvr(), "6:5.0-11");
    fail_unless(pkg->getEpoch() == 6);
    ck_assert_str_eq(pkg->getVersion(), "5.0");
    ck_assert_str_eq(pkg->getRelease(), "11");
    delete pkg;

    pkg = by_name(sack, "jay");
    // epoch missing if it's 0:
    ck_assert_str_eq(pkg->getEvr(), "5.0-0");
    fail_unless(pkg->getEpoch() == 0);
    ck_assert_str_eq(pkg->getVersion(), "5.0");
    ck_assert_str_eq(pkg->getRelease(), "0");
    delete pkg;
}
END_TEST

START_TEST(test_no_sourcerpm)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = by_name(sack, "baby");
    auto src = pkg->getSourceRpm();

    fail_unless(src == nullptr);
    delete pkg;
}
END_TEST

START_TEST(test_get_requires)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = by_name(sack, "flying");
    auto reldeplist = pkg->getRequires();

    fail_unless(reldeplist->count() == 1);
    auto reldep = reldeplist->get(0);

    ck_assert_str_eq(reldep->toString(), "P-lib >= 3");

    delete pkg;
}
END_TEST

START_TEST(test_get_more_requires)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = by_name(sack, "walrus");
    auto reldeplist = pkg->getRequires();

    fail_unless(reldeplist->count() == 2);
    delete pkg;
}
END_TEST

START_TEST(test_chksum_fail)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = by_name(sack, "walrus");

    auto chksum = pkg->getChecksum();
    fail_unless(std::get<0>(chksum) == nullptr);
    chksum = pkg->getHeaderChecksum();
    fail_unless(std::get<0>(chksum) == nullptr);
    delete pkg;
}
END_TEST

START_TEST(test_checksums)
{
    DnfPackage *pkg = by_name(test_globals.sack, "mystery-devel");
    auto checksum = pkg->getChecksum();
    fail_unless(std::get<1>(checksum) == G_CHECKSUM_SHA256);
    // Check the first and last bytes. Those need to match against information
    // in primary.xml.gz.
    fail_unless(std::get<0>(checksum)[0] == 0x2e);
    fail_unless(std::get<0>(checksum)[31] == 0xf5);

    delete pkg;
}
END_TEST

START_TEST(test_get_files)
{
    DnfSack *sack = test_globals.sack;

    DnfPackage *pkg = by_name(sack, "tour");
    auto files = pkg->getFiles();

    g_assert_cmpint(files.size(), ==, 6);
    delete pkg;
}
END_TEST

START_TEST(test_get_advisories)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *pkg = by_name(sack, "tour");

    auto advisories = pkg->getAdvisories(HY_GT);
    ck_assert_int_eq(advisories.size(), 1);
    auto advisory = advisories.at(0);
    ck_assert_str_eq(advisory->getName(), "FEDORA-2008-9969");
    delete pkg;
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

    delete pkg;
}
END_TEST

START_TEST(test_lookup_num)
{
    DnfPackage *pkg = by_name(test_globals.sack, "tour");
    guint64 buildtime = dnf_package_get_buildtime(pkg);
    fail_unless(buildtime > 1330473600); // after 2012-02-29
    fail_unless(buildtime < 1456704000); // before 2016-02-29

    delete pkg;
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

    delete pkg1;
    delete pkg2;
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

    delete pkg1;
    delete pkg2;

    g_object_unref(sack1);
    g_free(tmpdir);
}
END_TEST

START_TEST(test_packager)
{
    DnfPackage *pkg = by_name(test_globals.sack, "tour");
    ck_assert_str_eq(pkg->getPackager(), "roll up <roll@up.net>");
    delete pkg;
}
END_TEST

START_TEST(test_sourcerpm)
{
    DnfPackage *pkg = by_name(test_globals.sack, "tour");

    ck_assert_str_eq(pkg->getSourceRpm(), "tour-4-6.src.rpm");
    delete pkg;

    pkg = by_name(test_globals.sack, "mystery-devel");
    ck_assert_str_eq(pkg->getSourceRpm(), "mystery-19.67-1.src.rpm");
    delete pkg;
}
END_TEST

#define TOUR_45_46_DRPM_CHKSUM "\xc3\xc3\xd5\x72\xa4\x6b"\
    "\x1a\x66\x90\x6d\x42\xca\x17\x63\xef\x36\x20\xf7\x02"\
    "\x58\xaa\xac\x4c\x14\xbf\x46\x3e\xd5\x37\x16\xd4\x44"

START_TEST(test_presto)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *tour = by_name(sack, "tour");
    fail_if(tour == nullptr);

    auto delta = tour->getDelta("4-5");
    const char *location = dnf_packagedelta_get_location(delta.get());
    ck_assert_str_eq(location, "drpms/tour-4-5_4-6.noarch.drpm");
    const char *baseurl = dnf_packagedelta_get_baseurl(delta.get());
    fail_unless(baseurl == nullptr);
    guint64 size = dnf_packagedelta_get_downloadsize(delta.get());
    ck_assert_int_eq(size, 3132);
    int type;
    HyChecksum *csum = dnf_packagedelta_get_chksum(delta.get(), &type);
    fail_unless(type == G_CHECKSUM_SHA256);
    ck_assert(!memcmp(csum, TOUR_45_46_DRPM_CHKSUM, 32));
    delete tour;
}
END_TEST

START_TEST(test_get_files_cmdline)
{
    DnfSack *sack = test_globals.sack;

    DnfPackage *pkg = by_name(sack, "tour");

    auto files = pkg->getFiles();
    g_assert_cmpint (6, ==, files.size());
    delete pkg;
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
