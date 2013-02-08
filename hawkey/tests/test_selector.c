// hawkey
#include "src/selector.h"
#include "fixtures.h"
#include "testsys.h"
#include "test_suites.h"

START_TEST(test_sltr_matching)
{
    HySelector sltr = hy_selector_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "penny"));
    HyPackageList plist = hy_selector_matches(sltr);
    fail_unless(hy_packagelist_count(plist) == 2);

    hy_packagelist_free(plist);
    hy_selector_free(sltr);
}
END_TEST

START_TEST(test_sltr_version)
{
    HySelector sltr = hy_selector_create(test_globals.sack);

    fail_if(hy_selector_set(sltr, HY_PKG_NAME, HY_EQ, "jay"));
    fail_if(hy_selector_set(sltr, HY_PKG_VERSION, HY_EQ, "5.0"));
    HyPackageList plist = hy_selector_matches(sltr);
    fail_unless(hy_packagelist_count(plist) == 2);

    hy_packagelist_free(plist);
    hy_selector_free(sltr);
}
END_TEST

Suite *
selector_suite(void)
{
    Suite *s = suite_create("Selector");
    TCase *tc = tcase_create("Core");

    tcase_add_unchecked_fixture(tc, fixture_all, teardown);
    tcase_add_test(tc, test_sltr_matching);
    tcase_add_test(tc, test_sltr_version);

    suite_add_tcase(s, tc);

    return s;
}
