#include <check.h>
#include <string.h>

// hawkey
#include "src/repo_internal.h"
#include "test_suites.h"

START_TEST(test_strings)
{
    HyRepo hrepo = hy_repo_create("happy2");
    ck_assert_str_eq(hy_repo_get_string(hrepo, HY_REPO_NAME), "happy2");
    hy_repo_set_string(hrepo, HY_REPO_PRESTO_FN, "tunedtoA");
    ck_assert_str_eq(hy_repo_get_string(hrepo, HY_REPO_PRESTO_FN), "tunedtoA");
    hy_repo_set_string(hrepo, HY_REPO_PRESTO_FN, "naturalE");
    ck_assert_str_eq(hy_repo_get_string(hrepo, HY_REPO_PRESTO_FN), "naturalE");
    hy_repo_free(hrepo);
}
END_TEST

Suite *
repo_suite(void)
{
    Suite *s = suite_create("Repo");
    TCase *tc = tcase_create("Core");
    tcase_add_test(tc, test_strings);
    suite_add_tcase(s, tc);

    return s;
}
