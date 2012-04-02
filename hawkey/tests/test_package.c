//hawkey
#include "src/query.h"
#include "testsys.h"
#include "test_package.h"

static HyPackageList
default_test_package(HySack sack)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "penny-lib");
    HyPackageList plist = hy_query_run(q);
    hy_query_free(q);
    return plist;
}

START_TEST(test_package_summary)
{
    HySack sack = test_globals.sack;
    HyPackageList plist = default_test_package(sack);
    fail_unless(hy_packagelist_count(plist) == 1);
    HyPackage pkg = hy_packagelist_get(plist, 0);
    fail_if(strcmp(hy_package_get_summary(pkg), "in my ears"));
    hy_packagelist_free(plist);
}
END_TEST

Suite *
package_suite(void)
{
    Suite *s = suite_create("Package");
    TCase *tc;

    tc = tcase_create("Core");
    tcase_add_unchecked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_package_summary);
    suite_add_tcase(s, tc);
    return s;
}
