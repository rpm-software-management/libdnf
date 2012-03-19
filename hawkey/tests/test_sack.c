#include <check.h>
#include <stdio.h>
#include <stdlib.h>

// libsolv
#include <solv/testcase.h>

// hawkey
#include "src/sack.h"
#include "testsys.h"

START_TEST(test_sack_create)
{
    Sack sack = sack_create();
    fail_if(sack == NULL, NULL);
    fail_if(sack->pool == NULL, NULL);
    sack_free(sack);
}
END_TEST

START_TEST(test_repo_load)
{
    Sack sack = sack_create();
    Pool *pool = sack->pool;
    Repo *r = repo_create(sack->pool, SYSTEM_REPO_NAME);
    const char *repo = pool_tmpjoin(pool, test_globals.repo_dir, "system.repo", 0);
    FILE *fp = fopen(repo, "r");

    testcase_add_susetags(r,  fp, 0);
    fail_unless(pool->nsolvables == TEST_EXPECT_SYSTEM_NSOLVABLES);

    fclose(fp);
    sack_free(sack);
}
END_TEST

Suite *
sack_suite(void)
{
    Suite *s = suite_create("Sack");
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_sack_create);
    tcase_add_test(tc_core, test_repo_load);
    suite_add_tcase(s, tc_core);
    return s;
}
