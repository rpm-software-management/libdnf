// hawkey
#include "src/package_internal.h"
#include "src/packagelist.h"
#include "test_packagelist.h"

static HyPackage
mock_package(Id id)
{
    return package_create(NULL, id);
}

START_TEST(test_iter_macro)
{
    HyPackageList plist = hy_packagelist_create();
    hy_packagelist_push(plist, mock_package(10));
    hy_packagelist_push(plist, mock_package(11));
    hy_packagelist_push(plist, mock_package(12));

    int i, max, count = 0;
    HyPackage pkg;
    FOR_PACKAGELIST(pkg, plist, i) {
	count += 1;
	max = i;
    }
    fail_unless(max == 3);
    fail_unless(count == hy_packagelist_count(plist));
    hy_packagelist_free(plist);
}
END_TEST

Suite *
packagelist_suite(void)
{
    Suite *s = suite_create("PackageList");
    TCase *tc = tcase_create("Core");

    tcase_add_test(tc, test_iter_macro);
    suite_add_tcase(s, tc);

    return s;
}
