#define _GNU_SOURCE
#include <assert.h>
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

// libsolv
#include <solv/util.h>

// hawkey
#include "fixtures.h"
#include "test_goal.h"
#include "test_iutil.h"
#include "test_query.h"
#include "test_package.h"
#include "test_packagelist.h"
#include "test_packageset.h"
#include "test_reldep.h"
#include "test_repo.h"
#include "test_sack.h"
#include "test_util.h"
#include "testsys.h"

static int
init_test_globals(struct TestGlobals_s *tg, const char *repo_dir)
{
    int const len = strlen(repo_dir);
    if (repo_dir[len -1] != '/')
	tg->repo_dir = solv_dupjoin(repo_dir, "/", NULL);
    else
	tg->repo_dir = solv_strdup(repo_dir);
    tg->tmpdir = solv_strdup(UNITTEST_DIR);
    if (mkdtemp(tg->tmpdir) == NULL)
	return 1;
    tg->sack = NULL;
    return 0;
}

static void
free_test_globals(struct TestGlobals_s *tg)
{
    solv_free(tg->tmpdir);
    solv_free(tg->repo_dir);
}

int
main(int argc, const char **argv)
{
    int number_failed;
    struct stat s;

    if (argc != 2) {
	fprintf(stderr, "synopsis: %s <repo_directory>\n", argv[0]);
	exit(1);
    }
    if (stat(argv[1], &s) || !S_ISDIR(s.st_mode)) {
	fprintf(stderr, "can not read repos at '%s'.\n", argv[1]);
	exit(1);
    }
    if (init_test_globals(&test_globals, argv[1])) {
	fprintf(stderr, "failed initializing test engine.\n");
	exit(1);
    }
    printf("Tests using directory: %s\n", test_globals.tmpdir);

    SRunner *sr = srunner_create(sack_suite());
    srunner_add_suite(sr, iutil_suite());
    srunner_add_suite(sr, util_suite());
    srunner_add_suite(sr, reldep_suite());
    srunner_add_suite(sr, repo_suite());
    srunner_add_suite(sr, package_suite());
    srunner_add_suite(sr, packagelist_suite());
    srunner_add_suite(sr, packageset_suite());
    srunner_add_suite(sr, query_suite());
    srunner_add_suite(sr, goal_suite());
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    free_test_globals(&test_globals);
    return (number_failed == 0) ? 0 : 1;
}
