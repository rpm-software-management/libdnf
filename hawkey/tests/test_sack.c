#include <check.h>
#include <stdio.h>
#include <stdlib.h>

// libsolv
#include <solv/testcase.h>

// hawkey
#include "src/sack.h"

struct TestGlobals_s {
    char *repo_dir;
};

static struct TestGlobals_s test_globals;

static void
free_test_globals(struct TestGlobals_s *tg)
{
    solv_free(tg->repo_dir);
}

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
    fail_unless(pool->nsolvables == 5);

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

int
main(int argc, const char **argv)
{
    if (argc != 2) {
	fprintf(stderr, "synopsis: %s <repo_directory>\n", argv[0]);
	exit(1);
    }
    test_globals.repo_dir = solv_strdup(argv[1]);

    int number_failed;
    Suite *s = sack_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    free_test_globals(&test_globals);
    return (number_failed == 0) ? 0 : 1;
}
