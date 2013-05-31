// hawkey
#include "src/package.h"
#include "src/query.h"
#include "src/reldep.h"
#include "src/util.h"
#include "fixtures.h"
#include "test_suites.h"
#include "testsys.h"

START_TEST(test_refcounting)
{
    HyPackage pkg = by_name(test_globals.sack, "penny-lib");
    fail_unless(hy_package_link(pkg) != NULL);
    hy_package_free(pkg);
    hy_package_free(pkg);
}
END_TEST

START_TEST(test_package_summary)
{
    HyPackage pkg = by_name(test_globals.sack, "penny-lib");
    fail_if(strcmp(hy_package_get_summary(pkg), "in my ears"));
    hy_package_free(pkg);
}
END_TEST

START_TEST(test_identical)
{
    HySack sack = test_globals.sack;
    HyPackage pkg1 = by_name(sack, "penny-lib");
    HyPackage pkg2 = by_name(sack, "flying");
    HyPackage pkg3 = by_name(sack, "penny-lib");

    fail_unless(hy_package_identical(pkg1, pkg3));
    fail_if(hy_package_identical(pkg2, pkg3));

    hy_package_free(pkg1);
    hy_package_free(pkg2);
    hy_package_free(pkg3);
}
END_TEST

START_TEST(test_versions)
{
    HySack sack = test_globals.sack;
    unsigned epoch;
    char *version, *release;
    HyPackage pkg;

    pkg = by_name(sack, "baby");
    ck_assert_str_eq(hy_package_get_evr(pkg), "6:5.0-11");
    epoch = hy_package_get_epoch(pkg);
    fail_unless(epoch == 6);
    version = hy_package_get_version(pkg);
    ck_assert_str_eq(version, "5.0");
    hy_free(version);
    release = hy_package_get_release(pkg);
    ck_assert_str_eq(release, "11");
    hy_free(release);
    hy_package_free(pkg);

    pkg = by_name(sack, "jay");
    // epoch missing if it's 0:
    ck_assert_str_eq(hy_package_get_evr(pkg), "6.0-0");
    epoch = hy_package_get_epoch(pkg);
    fail_unless(epoch == 0);
    version = hy_package_get_version(pkg);
    ck_assert_str_eq(version, "6.0");
    hy_free(version);
    release = hy_package_get_release(pkg);
    ck_assert_str_eq(release, "0");
    hy_free(release);
    hy_package_free(pkg);
}
END_TEST

START_TEST(test_no_sourcerpm)
{
    HySack sack = test_globals.sack;
    HyPackage pkg = by_name(sack, "baby");
    char *src = hy_package_get_sourcerpm(pkg);

    fail_unless(src == NULL);
    hy_package_free(pkg);
}
END_TEST

START_TEST(test_get_requires)
{
    HySack sack = test_globals.sack;
    HyPackage pkg = by_name(sack, "flying");
    HyReldepList reldeplist = hy_package_get_requires(pkg);

    fail_unless(hy_reldeplist_count(reldeplist) == 1);
    HyReldep reldep = hy_reldeplist_get_clone(reldeplist, 0);

    char *depstr = hy_reldep_str(reldep);
    ck_assert_str_eq(depstr, "P-lib >= 3");
    hy_free(depstr);

    hy_reldep_free(reldep);
    hy_reldeplist_free(reldeplist);
    hy_package_free(pkg);
}
END_TEST

START_TEST(test_get_more_requires)
{
    HySack sack = test_globals.sack;
    HyPackage pkg = by_name(sack, "walrus");
    HyReldepList reldeplist = hy_package_get_requires(pkg);

    fail_unless(hy_reldeplist_count(reldeplist) == 2);
    hy_reldeplist_free(reldeplist);
    hy_package_free(pkg);
}
END_TEST

START_TEST(test_chksum_fail)
{
    HySack sack = test_globals.sack;
    HyPackage pkg = by_name(sack, "walrus");
    int type;

    const unsigned char *chksum = hy_package_get_chksum(pkg, &type);
    fail_unless(chksum == NULL);
    chksum = hy_package_get_hdr_chksum(pkg, &type);
    fail_unless(chksum == NULL);
    hy_package_free(pkg);
}
END_TEST

START_TEST(test_checksums)
{
    HyPackage pkg = by_name(test_globals.sack, "mystery-devel");
    int i;
    HyChecksum *csum = hy_package_get_chksum(pkg, &i);
    fail_unless(i == HY_CHKSUM_SHA256);
    // Check the first and last bytes. Those need to match against information
    // in primary.xml.gz.
    fail_unless(csum[0] == 0x2e);
    fail_unless(csum[31] == 0xf5);

    hy_package_free(pkg);
}
END_TEST

START_TEST(test_lookup_num)
{
    HyPackage pkg = by_name(test_globals.sack, "tour");
    unsigned long long buildtime = hy_package_get_buildtime(pkg);
    fail_unless(buildtime > 1330473600); // after 2012-02-29
    fail_unless(buildtime < 1456704000); // before 2016-02-29

    hy_package_free(pkg);
}
END_TEST

START_TEST(test_installed)
{
    HyPackage pkg1 = by_name_repo(test_globals.sack, "penny-lib", "main");
    HyPackage pkg2 = by_name_repo(test_globals.sack,
				  "penny-lib", HY_SYSTEM_REPO_NAME);
    int installed1 = hy_package_installed(pkg1);
    int installed2 = hy_package_installed(pkg2);
    fail_unless(installed1 == 0);
    fail_unless(installed2 == 1);

    hy_package_free(pkg1);
    hy_package_free(pkg2);
}

END_TEST
START_TEST(test_packager)
{
    HyPackage pkg = by_name(test_globals.sack, "tour");
    ck_assert_str_eq(hy_package_get_packager(pkg), "roll up <roll@up.net>");
    hy_package_free(pkg);
}
END_TEST

START_TEST(test_sourcerpm)
{
    HyPackage pkg = by_name(test_globals.sack, "tour");
    char *sourcerpm = hy_package_get_sourcerpm(pkg);

    ck_assert_str_eq(sourcerpm, "tour-4-6.src.rpm");
    hy_free(sourcerpm);
    hy_package_free(pkg);

    pkg = by_name(test_globals.sack, "mystery-devel");
    sourcerpm = hy_package_get_sourcerpm(pkg);
    ck_assert_str_eq(sourcerpm, "mystery-19.67-1.src.rpm");
    hy_free(sourcerpm);
    hy_package_free(pkg);
}
END_TEST

START_TEST(test_presto)
{
    HySack sack = test_globals.sack;
    HyPackage tour = by_name(sack, "tour");
    fail_if(tour == NULL);

    HyPackageDelta delta = hy_package_get_delta_from_evr(tour, "4-5");
    const char *location = hy_packagedelta_get_location(delta);
    ck_assert_str_eq(location, "drpms/tour-4-5_4-6.noarch.drpm");
    hy_packagedelta_free(delta);
    hy_package_free(tour);
}
END_TEST

Suite *
package_suite(void)
{
    Suite *s = suite_create("Package");
    TCase *tc;

    tc = tcase_create("Core");
    tcase_add_unchecked_fixture(tc, fixture_system_only, teardown);
    tcase_add_test(tc, test_refcounting);
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
    suite_add_tcase(s, tc);

    tc = tcase_create("WithRealRepo");
    tcase_add_unchecked_fixture(tc, fixture_yum, teardown);
    tcase_add_test(tc, test_checksums);
    tcase_add_test(tc, test_lookup_num);
    tcase_add_test(tc, test_packager);
    tcase_add_test(tc, test_sourcerpm);
    tcase_add_test(tc, test_presto);
    suite_add_tcase(s, tc);

    return s;
}
