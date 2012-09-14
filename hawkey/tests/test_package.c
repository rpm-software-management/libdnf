// hawkey
#include "src/query.h"
#include "src/util.h"
#include "testsys.h"
#include "test_package.h"

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
    ck_assert_str_eq(hy_package_get_evr(pkg), "1:5.0-11");
    epoch = hy_package_get_epoch(pkg);
    fail_unless(epoch == 1);
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

START_TEST(test_checksums)
{
    HyPackage pkg = by_name(test_globals.sack, "mystery");
    int i;
    HyChecksum *csum = hy_package_get_chksum(pkg, &i);
    fail_unless(i == HY_CHKSUM_SHA256);
    // Check the first and last bytes. Those need to match against information
    // in primary.xml.gz.
    fail_unless(csum[0] == 0xb2);
    fail_unless(csum[31] == 0x7a);

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

START_TEST(test_sourcerpm)
{
    HyPackage pkg = by_name(test_globals.sack, "tour");
    char *sourcerpm = hy_package_get_sourcerpm(pkg);

    ck_assert_str_eq(sourcerpm, "tour-4-6.src.rpm");
    hy_free(sourcerpm);
    hy_package_free(pkg);

    pkg = by_name(test_globals.sack, "mystery");
    sourcerpm = hy_package_get_sourcerpm(pkg);
    ck_assert_str_eq(sourcerpm, "mmysteryt-19.67-1.src.rpm");
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
    suite_add_tcase(s, tc);

    tc = tcase_create("WithRealRepo");
    tcase_add_unchecked_fixture(tc, fixture_yum, teardown);
    tcase_add_test(tc, test_checksums);
    tcase_add_test(tc, test_lookup_num);
    tcase_add_test(tc, test_sourcerpm);
    tcase_add_test(tc, test_presto);
    suite_add_tcase(s, tc);

    return s;
}
