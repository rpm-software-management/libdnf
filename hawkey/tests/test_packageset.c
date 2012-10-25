// hawkey
#include "src/package_internal.h"
#include "src/packageset.h"
#include "src/sack_internal.h"
#include "fixtures.h"
#include "test_packageset.h"

START_TEST(test_has)
{
    HySack sack = test_globals.sack;
    HyPackageSet pset = hy_packageset_create(sack);
    HyPackage pkg0 = package_create(sack_pool(sack), 0);
    HyPackage pkg9 = package_create(sack_pool(sack), 9);
    int max = sack_last_solvable(sack);
    HyPackage pkg_max = package_create(sack_pool(sack), max);

    hy_packageset_add(pset, pkg0);
    hy_packageset_add(pset, pkg9);
    hy_packageset_add(pset, pkg_max);

    pkg0 = package_create(sack_pool(sack), 0);
    pkg9 = package_create(sack_pool(sack), 9);
    pkg_max = package_create(sack_pool(sack), sack_last_solvable(sack));

    HyPackage pkg7 = package_create(sack_pool(sack), 7);
    HyPackage pkg8 = package_create(sack_pool(sack), 8);
    HyPackage pkg15 = package_create(sack_pool(sack), 15);

    fail_unless(hy_packageset_has(pset, pkg0));
    fail_unless(hy_packageset_has(pset, pkg9));
    fail_unless(hy_packageset_has(pset, pkg_max));
    fail_if(hy_packageset_has(pset, pkg7));
    fail_if(hy_packageset_has(pset, pkg8));
    fail_if(hy_packageset_has(pset, pkg15));

    hy_package_free(pkg0);
    hy_package_free(pkg9);
    hy_package_free(pkg_max);
    hy_package_free(pkg7);
    hy_package_free(pkg8);
    hy_package_free(pkg15);

    hy_packageset_free(pset);
}
END_TEST

START_TEST(test_get)
{
    HySack sack = test_globals.sack;
    HyPackageSet pset = hy_packageset_create(sack);
    HyPackage pkg0 = package_create(sack_pool(sack), 0);
    HyPackage pkg9 = package_create(sack_pool(sack), 9);
    int max = sack_last_solvable(sack);
    HyPackage pkg_max = package_create(sack_pool(sack), max);

    hy_packageset_add(pset, pkg0);
    hy_packageset_add(pset, pkg9);
    hy_packageset_add(pset, pkg_max);

    fail_unless(hy_packageset_count(pset) == 3);

    pkg0 = hy_packageset_get_clone(pset, 0);
    pkg9 = hy_packageset_get_clone(pset, 1);
    pkg_max = hy_packageset_get_clone(pset, 2);
    fail_unless(package_id(pkg0) == 0);
    fail_unless(package_id(pkg9) == 9);
    fail_unless(package_id(pkg_max) == max);
    fail_unless(hy_packageset_get_clone(pset, 3) == NULL);

    hy_package_free(pkg0);
    hy_package_free(pkg9);
    hy_package_free(pkg_max);
    hy_packageset_free(pset);
}
END_TEST

Suite *
packageset_suite(void)
{
    Suite *s = suite_create("PackageSet");

    TCase *tc = tcase_create("Core");
    tcase_add_unchecked_fixture(tc, fixture_all, teardown);
    tcase_add_test(tc, test_has);
    tcase_add_test(tc, test_get);
    suite_add_tcase(s, tc);

    return s;
}
