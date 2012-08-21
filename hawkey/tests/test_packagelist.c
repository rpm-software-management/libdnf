// hawkey
#include "src/package_internal.h"
#include "src/packagelist.h"
#include "test_packagelist.h"

static HyPackage
mock_package(Id id)
{
    return package_create(NULL, id);
}

static HyPackageList fixture_plist;

static void
create_fixture(void)
{
    fixture_plist = hy_packagelist_create();
    hy_packagelist_push(fixture_plist, mock_package(10));
    hy_packagelist_push(fixture_plist, mock_package(11));
    hy_packagelist_push(fixture_plist, mock_package(12));
}

static void
free_fixture(void)
{
    hy_packagelist_free(fixture_plist);
}

START_TEST(test_iter_macro)
{
    int i, max = 0, count = 0;
    HyPackage pkg;
    FOR_PACKAGELIST(pkg, fixture_plist, i) {
	count += 1;
	max = i;
    }
    fail_unless(max == 3);
    fail_unless(count == hy_packagelist_count(fixture_plist));
}
END_TEST

START_TEST(test_has)
{
    HyPackage pkg1 = mock_package(10);
    HyPackage pkg2 = mock_package(1);

    fail_unless(hy_packagelist_has(fixture_plist, pkg1));
    fail_if(hy_packagelist_has(fixture_plist, pkg2));

    hy_package_free(pkg1);
    hy_package_free(pkg2);
}
END_TEST

Suite *
packagelist_suite(void)
{
    Suite *s = suite_create("PackageList");

    TCase *tc = tcase_create("Core");
    tcase_add_unchecked_fixture(tc, create_fixture, free_fixture);
    tcase_add_test(tc, test_iter_macro);
    tcase_add_test(tc, test_has);
    suite_add_tcase(s, tc);

    return s;
}
