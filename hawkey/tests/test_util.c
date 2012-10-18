#include <check.h>
#include <stdio.h>

// hawkey
#include "src/errno.h"
#include "src/util.h"

START_TEST(test_split_nevra)
{
    int ret;
    long int epoch;
    char *name, *version, *release, *arch;

    const char *n = "meanwhile3.34.3-3:3.34-3.fc666.i686";
    fail_if(hy_split_nevra(n, &name, &epoch, &version, &release, &arch));
    ck_assert_str_eq(name, "meanwhile3.34.3");
    fail_unless(epoch == 3);
    ck_assert_str_eq(version, "3.34");
    ck_assert_str_eq(release, "3.fc666");
    ck_assert_str_eq(arch, "i686");
    hy_free(name);
    hy_free(version);
    hy_free(release);
    hy_free(arch);

    n = "python-meanwhile3.34.3-3:3.34-3.fc666.noarch";
    fail_if(hy_split_nevra(n, &name, &epoch, &version, &release, &arch));
    ck_assert_str_eq(name, "python-meanwhile3.34.3");
    fail_unless(epoch == 3);
    ck_assert_str_eq(version, "3.34");
    ck_assert_str_eq(release, "3.fc666");
    ck_assert_str_eq(arch, "noarch");
    hy_free(name);
    hy_free(version);
    hy_free(release);
    hy_free(arch);

    n = "easy-1.2.3-4.fc18.x86_64";
    hy_split_nevra(n, &name, &epoch, &version, &release, &arch);
    ck_assert_str_eq(name, "easy");
    fail_unless(epoch == 0);
    ck_assert_str_eq(version, "1.2.3");
    ck_assert_str_eq(release, "4.fc18");
    ck_assert_str_eq(arch, "x86_64");
    hy_free(name);
    hy_free(version);
    hy_free(release);
    hy_free(arch);

    n = "no.go";
    ret = hy_split_nevra(n, &name, &epoch, &version, &release, &arch);
    fail_unless(ret == HY_E_OP);
}
END_TEST

Suite *
util_suite(void)
{
    Suite *s = suite_create("util");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_split_nevra);
    suite_add_tcase(s, tc);

    return s;
}
