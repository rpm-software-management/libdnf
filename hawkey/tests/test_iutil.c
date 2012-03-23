#define _GNU_SOURCE
#include "stdlib.h"

// hawkey
#include "src/iutil.h"
#include "test_iutil.h"
#include "testsys.h"

static void
build_test_file(const char *filename)
{
    FILE *fp = fopen(filename, "w+");
    fail_if(fp == NULL);
    fail_unless(fwrite("empty", 5, 1, fp) == 1);
    fclose(fp);
}

START_TEST(test_checksum)
{
    /* create a new file, edit it a bit */
    char *new_file = solv_dupjoin(test_globals.tmpdir,
				  "/test_checksum", NULL);
    build_test_file(new_file);

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
    FILE *fp;
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

START_TEST(test_checksum_write_read)
{
    char *new_file = solv_dupjoin(test_globals.tmpdir,
				  "/test_checksum_write_read", NULL);
    build_test_file(new_file);

    unsigned char cs_computed[CHKSUM_BYTES];
    unsigned char cs_read[CHKSUM_BYTES];
    FILE *fp = fopen(new_file, "r");
    checksum_fp(fp, cs_computed);
    // fails, file opened read-only:
    fail_unless(checksum_write(fp, cs_computed) == 1);
    fclose(fp);
    fp = fopen(new_file, "r+");
    fail_if(checksum_write(fp, cs_computed));
    fclose(fp);
    fp = fopen(new_file, "r");
    fail_if(checksum_read(fp, cs_read));
    fail_if(checksum_cmp(cs_computed, cs_read));
    fclose(fp);

    solv_free(new_file);
}
END_TEST

Suite *
iutil_suite(void)
{
    Suite *s = suite_create("iutil");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_checksum);
    tcase_add_test(tc, test_checksum_write_read);
    suite_add_tcase(s, tc);

    return s;
}
