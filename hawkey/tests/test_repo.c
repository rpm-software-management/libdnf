#include <check.h>

// hawkey
#include "src/repo_internal.h"
#include "test_repo.h"

START_TEST(test_transition)
{
    HyRepo hrepo = hy_repo_create();
    fail_if(hy_repo_transition(hrepo, _HY_LOADED_FETCH));
    fail_unless(hy_repo_transition(hrepo, _HY_FL_WRITTEN));
    fail_if(hy_repo_transition(hrepo, _HY_WRITTEN));
    fail_if(hy_repo_transition(hrepo, _HY_FL_LOADED_CACHE));
    fail_unless(hy_repo_transition(hrepo, _HY_FL_LOADED_CACHE));
    fail_unless(hy_repo_transition(hrepo, _HY_FL_WRITTEN));
}
END_TEST

Suite *
repo_suite(void)
{
    Suite *s = suite_create("Repo");
    TCase *tc = tcase_create("Core");
    tcase_add_test(tc, test_transition);
    suite_add_tcase(s, tc);

    return s;
}
