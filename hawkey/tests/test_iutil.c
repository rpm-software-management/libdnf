/*
 * Copyright (C) 2012-2013 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>

// libsolv
#include <solv/pool.h>

// hawkey
#include "src/util.h"
#include "src/iutil.h"
#include "fixtures.h"
#include "test_suites.h"

static void
build_test_file(const char *filename)
{
    FILE *fp = fopen(filename, "w+");
    fail_if(fp == NULL);
    fail_unless(fwrite("empty", 5, 1, fp) == 1);
    fclose(fp);
}

START_TEST(test_abspath)
{
    char *abs = abspath("/cus/tard");
    ck_assert_str_eq(abs, "/cus/tard");
    hy_free(abs);

    abs = abspath("cus/tard");
    ck_assert_int_eq(abs[0], '/');
    hy_free(abs);
}
END_TEST

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
    fail_if(checksum_fp(cs1, fp));
    fail_if(checksum_stat(cs1_sum, fp));
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
    fail_if(checksum_stat(cs2, fp));
    fail_if(checksum_stat(cs2_sum, fp));
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
    checksum_fp(cs_computed, fp);
    // fails, file opened read-only:
    fail_unless(checksum_write(cs_computed, fp) == 1);
    fclose(fp);
    fp = fopen(new_file, "r+");
    fail_if(checksum_write(cs_computed, fp));
    fclose(fp);
    fp = fopen(new_file, "r");
    fail_if(checksum_read(cs_read, fp));
    fail_if(checksum_cmp(cs_computed, cs_read));
    fclose(fp);

    solv_free(new_file);
}
END_TEST

START_TEST(test_mkcachedir)
{
    const char *workdir = test_globals.tmpdir;
    char * dir;
    fail_if(asprintf(&dir, "%s%s", workdir, "/mkcachedir/sub/deep") == -1);
    fail_if(mkcachedir(dir));
    fail_if(access(dir, R_OK|X_OK|W_OK));
    free(dir);

    fail_if(asprintf(&dir, "%s%s", workdir, "/mkcachedir/sub2/wd-XXXXXX") == -1);
    fail_if(mkcachedir(dir));
    fail_if(str_endswith(dir, "XXXXXX")); /* mkcache dir changed the Xs */
    fail_if(access(dir, R_OK|X_OK|W_OK));

    /* test the globbing capability of mkcachedir */
    char *dir2;
    fail_if(asprintf(&dir2, "%s%s", workdir, "/mkcachedir/sub2/wd-XXXXXX") == -1);
    fail_if(mkcachedir(dir2));
    fail_if(strcmp(dir, dir2)); /* mkcachedir should have arrived at the same name */

    free(dir2);
    free(dir);
}
END_TEST

START_TEST(test_str_endswith)
{
    fail_unless(str_endswith("spinning", "ing"));
    fail_unless(str_endswith("spinning", "spinning"));
    fail_unless(str_endswith("", ""));
    fail_if(str_endswith("aaa", "b"));
}
END_TEST

START_TEST(test_str_startswith)
{
    fail_unless(str_startswith("spinning", "spi"));
    fail_unless(str_startswith("spinning", "spinning"));
    fail_unless(str_startswith("", ""));
    fail_if(str_startswith("aaa", "x"));
    fail_if(str_startswith("aaa", "bbbb"));
}
END_TEST

START_TEST(test_version_split)
{
    Pool *pool = pool_create();
    char evr[] = "1:5.9.3-8";
    char *epoch, *version, *release;

    pool_split_evr(pool, evr, &epoch, &version, &release);
    ck_assert_str_eq(epoch, "1");
    ck_assert_str_eq(version, "5.9.3");
    ck_assert_str_eq(release, "8");

    char evr2[] = "8.0-9";
    pool_split_evr(pool, evr2, &epoch, &version, &release);
    fail_unless(epoch == NULL);
    ck_assert_str_eq(version, "8.0");
    ck_assert_str_eq(release, "9");

    char evr3[] = "1.4";
    pool_split_evr(pool, evr3, &epoch, &version, &release);
    fail_unless(epoch == NULL);
    ck_assert_str_eq(version, "1.4");
    fail_unless(release == NULL);

    pool_free(pool);
}
END_TEST

Suite *
iutil_suite(void)
{
    Suite *s = suite_create("iutil");
    TCase *tc = tcase_create("Main");
    tcase_add_test(tc, test_abspath);
    tcase_add_test(tc, test_checksum);
    tcase_add_test(tc, test_checksum_write_read);
    tcase_add_test(tc, test_mkcachedir);
    tcase_add_test(tc, test_str_endswith);
    tcase_add_test(tc, test_str_startswith);
    tcase_add_test(tc, test_version_split);
    suite_add_tcase(s, tc);
    return s;
}
