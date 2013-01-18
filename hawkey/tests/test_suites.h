#ifndef TEST_SUITES_H
#define TEST_SUITES_H

#include <check.h>

Suite *goal_suite(void);
Suite *iutil_suite(void);
Suite *package_suite(void);
Suite *packagelist_suite(void);
Suite *packageset_suite(void);
Suite *query_suite(void);
Suite *reldep_suite(void);
Suite *repo_suite(void);
Suite *sack_suite(void);
Suite *selector_suite(void);
Suite *util_suite(void);

#endif // TEST_SUITES_H
