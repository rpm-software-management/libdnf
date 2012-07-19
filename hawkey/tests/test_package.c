// hawkey
#include "src/query.h"
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

START_TEST(test_presto)
{
    HySack sack = test_globals.sack;
    HyPackage tour = by_name(sack, "tour");
    fail_if(tour == NULL);
    fail_if(hy_sack_load_presto(sack));

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
    tcase_add_unchecked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_refcounting);
    tcase_add_test(tc, test_package_summary);
    suite_add_tcase(s, tc);

    tc = tcase_create("WithRealRepo");
    tcase_add_unchecked_fixture(tc, setup_yum, teardown);
    tcase_add_test(tc, test_checksums);
    tcase_add_test(tc, test_lookup_num);
    tcase_add_test(tc, test_presto);
    suite_add_tcase(s, tc);

    return s;
}
