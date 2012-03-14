#include <check.h>

// libsolv
#include <solv/testcase.h>

// hawkey
#include "src/query.h"
#include "testsys.h"
#include "test_query.h"

static void
setup(void)
{
    Sack sack = sack_create();
    Pool *pool = sack->pool;
    Repo *r = repo_create(pool, SYSTEM_REPO_NAME);
    const char *repo = pool_tmpjoin(pool, test_globals.repo_dir,
				    "system.repo", 0);
    FILE *fp = fopen(repo, "r");

    testcase_add_susetags(r,  fp, 0);
    pool_set_installed(pool, r);
    fail_unless(pool->nsolvables == 5);

    fclose(fp);
    test_globals.sack = sack;
}

static void
setup_with_updates(void)
{
    setup();
    Pool *pool = test_globals.sack->pool;
    Repo *r = repo_create(pool, "updates");
    const char *repo = pool_tmpjoin(pool, test_globals.repo_dir,
				    "updates.repo", 0);

    FILE *fp = fopen(repo, "r");

    testcase_add_susetags(r, fp, 0);
    fclose(fp);
}

static void
teardown(void)
{
    sack_free(test_globals.sack);
    test_globals.sack = NULL;
}

static int
count_results(Query q)
{
    PackageList plist = query_run(q);
    int ret = packagelist_count(plist);
    packagelist_free(plist);
    return ret;
}

START_TEST(test_query_sanity)
{
    Sack sack = test_globals.sack;
    fail_unless(sack != NULL);
    fail_unless(sack->pool->nsolvables == 5);
    fail_unless(sack->pool->installed != NULL);

    Query query = query_create(sack);
    fail_unless(query != NULL);
    query_free(query);
}
END_TEST

START_TEST(test_query_repo)
{
    Query q;

    q = query_create(test_globals.sack);
    query_filter(q, KN_PKG_REPO, FT_EQ, SYSTEM_REPO_NAME);

    fail_unless(count_results(q) == 3);

    query_free(q);

    q = query_create(test_globals.sack);
    query_filter(q, KN_PKG_REPO, FT_LT|FT_GT, SYSTEM_REPO_NAME);
    fail_if(count_results(q));

    query_free(q);
}
END_TEST

START_TEST(test_query_name)
{
    Query q;

    q = query_create(test_globals.sack);
    query_filter(q, KN_PKG_NAME, FT_SUBSTR, "penny");
    fail_unless(count_results(q) == 2);
    query_free(q);

    q = query_create(test_globals.sack);
    query_filter(q, KN_PKG_NAME, FT_EQ, "lane");
    fail_if(count_results(q));
    query_free(q);
}
END_TEST

START_TEST(test_query_anded)
{
    Query q;

    q = query_create(test_globals.sack);
    query_filter(q, KN_PKG_NAME, FT_SUBSTR, "penny");
    query_filter(q, KN_PKG_SUMMARY, FT_SUBSTR, "ears");
    fail_unless(count_results(q) == 1);
    query_free(q);
}
END_TEST

START_TEST(test_updates_sanity)
{
    Pool *pool = test_globals.sack->pool;
    Repo *r = NULL;
    int i;

    FOR_REPOS(i, r)
	if (!strcmp(r->name, "updates"))
	    break;
    fail_unless(r != NULL);
    fail_unless(r->nsolvables == 1);
}
END_TEST

START_TEST(test_updates)
{
    Query q = query_create(test_globals.sack);
    query_filter_updates(q, 1);
    fail_unless(count_results(q) == 1);
    query_free(q);
}
END_TEST

Suite *
query_suite(void)
{
    Suite *s = suite_create("Query");
    TCase *tc;

    tc = tcase_create("Core");
    tcase_add_unchecked_fixture(tc, setup, teardown);
    tcase_add_test(tc, test_query_sanity);
    tcase_add_test(tc, test_query_repo);
    tcase_add_test(tc, test_query_name);
    tcase_add_test(tc, test_query_anded);
    suite_add_tcase(s, tc);

    tc = tcase_create("Updates");
    tcase_add_unchecked_fixture(tc, setup_with_updates, teardown);
    tcase_add_test(tc, test_updates_sanity);
    tcase_add_test(tc, test_updates);
    suite_add_tcase(s, tc);

    return s;
}
