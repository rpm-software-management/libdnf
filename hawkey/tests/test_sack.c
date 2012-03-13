#include <check.h>
#include <stdio.h>

#include "src/sack.h"

START_TEST (test_sack_create)
{
    Sack sack = sack_create();
    fail_if(sack == NULL, NULL);
}
END_TEST

Suite *
sack_suite(void)
{
    Suite *s = suite_create("Sack");
    TCase *tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_sack_create);
    suite_add_tcase(s, tc_core);
    return s;
}

int
main (void)
{
    int number_failed;
    Suite *s = sack_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? 0 : 1;
}
