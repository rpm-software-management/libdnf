#include <check.h>
#include <stdio.h>
#include <stdlib.h>

// libsolv
#include <solv/util.h>

// hawkey
#include "testsys.h"
#include "test_sack.h"

struct TestGlobals_s test_globals;

static void
free_test_globals(struct TestGlobals_s *tg)
{
    solv_free(tg->repo_dir);
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
