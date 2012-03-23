#define _GNU_SOURCE
#include "stdlib.h"

// hawkey
#include "src/iutil.h"
#include "test_iutil.h"
#include "testsys.h"

START_TEST(test_checksum)
{
    /* create a new file, edit it a bit */
    char *new_file = solv_dupjoin(test_globals.tmpdir,
				  "/test_stat_checksum", NULL);
    FILE *fp = fopen(new_file, "w+");
    fail_if(fp == NULL);
    fail_unless(fwrite("empty", 5, 1, fp) == 1);
    fclose(fp);

    unsigned char cs1[CHKSUM_BYTES];
    unsigned char cs2[CHKSUM_BYTES];
    unsigned char cs1_sum[CHKSUM_BYTES];
    unsigned char cs2_sum[CHKSUM_BYTES];
    bzero(cs1, CHKSUM_BYTES);
    bzero(cs2, CHKSUM_BYTES);
    bzero(cs1_sum, CHKSUM_BYTES);
    bzero(cs2_sum, CHKSUM_BYTES);
    fail_if(checksum_cmp(cs1, cs2)); // tests checksum_cmp

    /* take the first checksums */
    fail_if((fp = fopen(new_file, "r")) == NULL);
    fail_if(checksum_fp(fp, cs1));
    fail_if(checksum_stat(fp, cs1_sum));
    fclose(fp);
    /* the taken checksum are not zeros anymore */
    fail_if(checksum_cmp(cs1, cs2) == 0);
    fail_if(checksum_cmp(cs1_sum, cs2_sum) == 0);

    /* append something */
    fail_if((fp = fopen(new_file, "a")) == NULL);
    fail_unless(fwrite("X", 1, 1, fp) == 1);
    fclose(fp);

    /* take the second checksums */
    fail_if((fp = fopen(new_file, "r")) == NULL);
    fail_if(checksum_stat(fp, cs2));
    fail_if(checksum_stat(fp, cs2_sum));
    fclose(fp);
    fail_unless(checksum_cmp(cs1, cs2));
    fail_unless(checksum_cmp(cs1_sum, cs2_sum));

    solv_free(new_file);
}
END_TEST

Suite *
iutil_suite(void)
{
    Suite *s = suite_create("iutil");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_checksum);
    suite_add_tcase(s, tc);

    return s;
}
