/*
 * Copyright (C) 2013 Red Hat, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <string.h>

// hawkey
#include "src/nevra.h"
#include "src/nevra_internal.h"
#include "src/reldep.h"
#include "src/sack.h"
#include "src/subject.h"
#include "src/subject_internal.h"
#include "fixtures.h"
#include "testshared.h"
#include "test_suites.h"

const char inp_fof[] = "four-of-fish-8:3.6.9-11.fc100.x86_64";
const char inp_fof_noepoch[] = "four-of-fish-3.6.9-11.fc100.x86_64";
const char inp_fof_nev[] = "four-of-fish-8:3.6.9";
const char inp_fof_na[] = "four-of-fish-3.6.9.i686";

START_TEST(nevra1)
{
    HyNevra nevra = hy_nevra_create();

    ck_assert_int_eq(
	nevra_possibility((char *) inp_fof, HY_FORM_NEVRA, nevra), 0);
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_NAME), "four-of-fish");
    ck_assert_int_eq(hy_nevra_get_epoch(nevra), 8);
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_VERSION), "3.6.9");
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_RELEASE), "11.fc100");
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_ARCH), "x86_64");

    hy_nevra_free(nevra);
}
END_TEST


START_TEST(nevra2)
{
    HyNevra nevra = hy_nevra_create();

    ck_assert_int_eq(
	nevra_possibility((char *) inp_fof_noepoch, HY_FORM_NEVRA, nevra), 0);
    ck_assert_str_eq(nevra->name, "four-of-fish");
    ck_assert_int_eq(nevra->epoch, -1);
    ck_assert_str_eq(nevra->version, "3.6.9");
    ck_assert_str_eq(nevra->release, "11.fc100");
    ck_assert_str_eq(nevra->arch, "x86_64");

    hy_nevra_free(nevra);
}
END_TEST

START_TEST(nevr)
{
    HyNevra nevra = hy_nevra_create();

    ck_assert_int_eq(
	nevra_possibility((char *) inp_fof, HY_FORM_NEVR, nevra), 0);
    ck_assert_str_eq(nevra->name, "four-of-fish");
    ck_assert_int_eq(nevra->epoch, 8);
    ck_assert_str_eq(nevra->version, "3.6.9");
    ck_assert_str_eq(nevra->release, "11.fc100.x86_64");
    fail_unless(nevra->arch == NULL);

    hy_nevra_free(nevra);
}
END_TEST

START_TEST(nevr_fail)
{
    HyNevra nevra = hy_nevra_create();

    ck_assert_int_eq(
	nevra_possibility((char *) "four-of", HY_FORM_NEVR, nevra), -1);

    hy_nevra_free(nevra);
}
END_TEST

START_TEST(nev)
{
    HyNevra nevra = hy_nevra_create();

    ck_assert_int_eq(
	nevra_possibility((char *) inp_fof_nev, HY_FORM_NEV, nevra), 0);
    ck_assert_str_eq(nevra->name, "four-of-fish");
    ck_assert_int_eq(nevra->epoch, 8);
    ck_assert_str_eq(nevra->version, "3.6.9");
    fail_unless(nevra->release == NULL);
    fail_unless(nevra->arch == NULL);

    hy_nevra_free(nevra);
}
END_TEST

START_TEST(na)
{
    HyNevra nevra = hy_nevra_create();

    ck_assert_int_eq(
	nevra_possibility((char *) inp_fof_na, HY_FORM_NA, nevra), 0);
    ck_assert_str_eq(nevra->name, "four-of-fish-3.6.9");
    ck_assert_int_eq(nevra->epoch, -1);
    fail_unless(nevra->version == NULL);
    ck_assert_str_eq(nevra->arch, "i686");

    hy_nevra_free(nevra);
}
END_TEST

START_TEST(combined1)
{
    HyNevra nevra;
    HySubject subject = hy_subject_create(inp_fof);
    HyPossibilities iter = hy_subject_nevra_possibilities(subject, NULL);
    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), 0);
    ck_assert_str_eq(nevra->name, "four-of-fish");
    ck_assert_int_eq(nevra->epoch, 8);
    ck_assert_str_eq(nevra->version, "3.6.9");
    ck_assert_str_eq(nevra->release, "11.fc100");
    ck_assert_str_eq(nevra->arch, "x86_64");
    hy_nevra_free(nevra);

    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), 0);
    ck_assert_str_eq(nevra->name, "four-of-fish");
    ck_assert_int_eq(nevra->epoch, 8);
    ck_assert_str_eq(nevra->version, "3.6.9");
    ck_assert_str_eq(nevra->release, "11.fc100.x86_64");
    fail_unless((nevra->arch) == NULL);
    hy_nevra_free(nevra);

    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), -1);
    hy_possibilities_free(iter);
    hy_subject_free(subject);
}
END_TEST

START_TEST(combined2)
{
    HyNevra nevra;
    HySubject subject = hy_subject_create(inp_fof_noepoch);
    HyPossibilities iter = hy_subject_nevra_possibilities(subject, NULL);

    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), 0);
    ck_assert_str_eq(nevra->name, "four-of-fish");
    ck_assert_int_eq(nevra->epoch, -1);
    ck_assert_str_eq(nevra->version, "3.6.9");
    ck_assert_str_eq(nevra->release, "11.fc100");
    ck_assert_str_eq(nevra->arch, "x86_64");
    hy_nevra_free(nevra);
    
    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), 0);
    ck_assert_str_eq(nevra->name, "four-of-fish");
    ck_assert_int_eq(nevra->epoch, -1);
    ck_assert_str_eq(nevra->version, "3.6.9");
    ck_assert_str_eq(nevra->release, "11.fc100.x86_64");
    fail_unless(nevra->arch == NULL);
    hy_nevra_free(nevra);

    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), 0);
    ck_assert_str_eq(nevra->name, "four-of-fish-3.6.9");
    ck_assert_int_eq(nevra->epoch, -1);
    ck_assert_str_eq(nevra->version, "11.fc100.x86_64");
    fail_unless(nevra->release == NULL);
    fail_unless(nevra->arch == NULL);
    hy_nevra_free(nevra);

    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), 0);
    ck_assert_str_eq(nevra->name, "four-of-fish-3.6.9-11.fc100");
    ck_assert_int_eq(nevra->epoch, -1);
    fail_unless(nevra->version == NULL);
    fail_unless(nevra->release == NULL);
    ck_assert_str_eq(nevra->arch, "x86_64");
    hy_nevra_free(nevra);

    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), 0);
    ck_assert_str_eq(nevra->name, "four-of-fish-3.6.9-11.fc100.x86_64");
    ck_assert_int_eq(nevra->epoch, -1);
    fail_unless(nevra->version == NULL);
    fail_unless(nevra->release == NULL);
    fail_unless(nevra->arch == NULL);
    hy_nevra_free(nevra);

    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), -1);
    hy_possibilities_free(iter);
    hy_subject_free(subject);
}
END_TEST

START_TEST(reldep)
{
    HyReldep reldep = NULL;
    HySubject subject = hy_subject_create("P-lib");
    HyPossibilities iter = hy_subject_reldep_possibilities_real(subject,
	test_globals.sack, 0);
    ck_assert_int_eq(hy_possibilities_next_reldep(iter, &reldep), 0);
    char *reldep_str = hy_reldep_str(reldep);
    ck_assert_str_eq(reldep_str, "P-lib");
    solv_free(reldep_str);
    hy_reldep_free(reldep);
    ck_assert_int_eq(hy_possibilities_next_reldep(iter, &reldep), -1);
    hy_possibilities_free(iter);
    hy_subject_free(subject);
}
END_TEST

START_TEST(reldep_fail)
{
    HyReldep reldep;
    HySubject subject = hy_subject_create("Package not exist");
    HyPossibilities iter = hy_subject_reldep_possibilities_real(subject,
	test_globals.sack, 0);
    ck_assert_int_eq(hy_possibilities_next_reldep(iter, &reldep), -1);
    hy_possibilities_free(iter);
    hy_subject_free(subject);
}
END_TEST

START_TEST(nevra_real_none)
{
    HyNevra nevra;
    HySubject subject = hy_subject_create(inp_fof_noepoch);
    HyPossibilities iter = hy_subject_nevra_possibilities_real(subject, NULL,
	test_globals.sack, 0);
    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), -1);
    hy_possibilities_free(iter);
    hy_subject_free(subject);
}
END_TEST

START_TEST(nevra_real)
{
    HyNevra nevra;
    HySubject subject = hy_subject_create("pilchard-1.2.4-1.x86_64");
    HyPossibilities iter = hy_subject_nevra_possibilities_real(subject, NULL,
	test_globals.sack, 0);

    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), 0);
    ck_assert_str_eq(nevra->name, "pilchard");
    ck_assert_int_eq(nevra->epoch, -1);
    ck_assert_str_eq(nevra->version, "1.2.4");
    ck_assert_str_eq(nevra->release, "1");
    ck_assert_str_eq(nevra->arch, "x86_64");
    hy_nevra_free(nevra);

    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), 0);
    ck_assert_str_eq(nevra->name, "pilchard");
    ck_assert_int_eq(nevra->epoch, -1);
    ck_assert_str_eq(nevra->version, "1.2.4");
    ck_assert_str_eq(nevra->release, "1.x86_64");
    fail_unless(nevra->arch == NULL);
    hy_nevra_free(nevra);

    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), -1);
    hy_possibilities_free(iter);
    hy_subject_free(subject);
}
END_TEST

START_TEST(nevra_real_dash)
{
    HyNevra nevra;
    HySubject subject = hy_subject_create("penny-lib");
    HyPossibilities iter = hy_subject_nevra_possibilities_real(subject, NULL,
	test_globals.sack, 0);
    
    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), 0);
    ck_assert_str_eq(nevra->name, "penny-lib");
    fail_unless(nevra->epoch == -1);
    fail_unless(nevra->version == NULL);
    fail_unless(nevra->release == NULL);
    fail_unless(nevra->arch == NULL);
    hy_nevra_free(nevra);
    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), -1);
    hy_possibilities_free(iter);
    hy_subject_free(subject);
}
END_TEST


Suite *
subject_suite(void)
{
    Suite *s = suite_create("Subject");
    TCase *tc = tcase_create("Core");
    tcase_add_test(tc, nevra1);
    tcase_add_test(tc, nevra2);
    tcase_add_test(tc, nevr);
    tcase_add_test(tc, nevr_fail);
    tcase_add_test(tc, nev);
    tcase_add_test(tc, na);
    tcase_add_test(tc, combined1);
    tcase_add_test(tc, combined2);
    suite_add_tcase(s, tc);

    tc = tcase_create("Full");
    tcase_add_unchecked_fixture(tc, fixture_all, teardown);
    tcase_add_test(tc, reldep);
    tcase_add_test(tc, reldep_fail);
    tcase_add_test(tc, nevra_real_none);
    tcase_add_test(tc, nevra_real);
    tcase_add_test(tc, nevra_real_dash);
    suite_add_tcase(s, tc);

    return s;
}
