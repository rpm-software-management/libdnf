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


#include "libdnf/hy-nevra.h"
#include "libdnf/hy-nevra-private.h"
#include "libdnf/hy-module-form.h"
#include "libdnf/hy-module-form-private.h"
#include "libdnf/dnf-reldep.h"
#include "libdnf/dnf-sack.h"
#include "libdnf/hy-subject.h"
#include "libdnf/hy-subject-private.h"
#include "fixtures.h"
#include "testshared.h"
#include "test_suites.h"

const char inp_fof[] = "four-of-fish-8:3.6.9-11.fc100.x86_64";
const char inp_fof_noepoch[] = "four-of-fish-3.6.9-11.fc100.x86_64";
const char inp_fof_nev[] = "four-of-fish-8:3.6.9";
const char inp_fof_na[] = "four-of-fish-3.6.9.i686";

/* module_forms */
/* with profile */
const char module_nsvcap[] = "module-name:stream:1:b86c854:x86_64/profile";
const char module_nsvap[] = "module-name:stream:1::x86_64/profile";
const char module_nsvcp[] = "module-name:stream:1:b86c854/profile";
const char module_nsvp[] = "module-name:stream:1/profile";
const char module_nsap[] = "module-name:stream::x86_64/profile";
const char module_nsp[] = "module-name:stream/profile";
const char module_np[] = "module-name/profile";
const char module_nap[] = "module-name::x86_64/profile";

/* without profile */
const char module_nsvca[] = "module-name:stream:1:b86c854:x86_64";
const char module_nsva[] = "module-name:stream:1::x86_64";
const char module_nsvc[] = "module-name:stream:1:b86c854";
const char module_nsv[] = "module-name:stream:1";
const char module_nsa[] = "module-name:stream::x86_64";
const char module_ns[] = "module-name:stream";
const char module_n[] = "module-name";
const char module_na[] = "module-name::x86_64";

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

    // NEVRA comparison tests
    HyNevra nevra2 = hy_nevra_create();
    hy_nevra_set_epoch(nevra2, 8);
    hy_nevra_set_string(nevra2, HY_NEVRA_NAME, "four-of-fish");
    hy_nevra_set_string(nevra2, HY_NEVRA_VERSION, "3.6.9");
    hy_nevra_set_string(nevra2, HY_NEVRA_RELEASE, "11.fc100");
    hy_nevra_set_string(nevra2, HY_NEVRA_ARCH, "x86_64");
    ck_assert_int_eq(hy_nevra_cmp(nevra, nevra2), 0);

    hy_nevra_set_epoch(nevra2, 3);
    ck_assert_int_gt(hy_nevra_cmp(nevra, nevra2), 0);

    hy_nevra_set_epoch(nevra2, 11);
    ck_assert_int_lt(hy_nevra_cmp(nevra, nevra2), 0);

    hy_nevra_set_epoch(nevra2, 8);
    hy_nevra_set_string(nevra2, HY_NEVRA_VERSION, "7.0");
    ck_assert_int_lt(hy_nevra_cmp(nevra, nevra2), 0);

    hy_nevra_set_epoch(nevra2, 8);
    hy_nevra_set_string(nevra2, HY_NEVRA_VERSION, NULL);
    ck_assert_int_gt(hy_nevra_cmp(nevra, nevra2), 0);

    hy_nevra_set_epoch(nevra2, 8);
    hy_nevra_set_string(nevra, HY_NEVRA_VERSION, NULL);
    ck_assert_int_eq(hy_nevra_cmp(nevra, nevra2), 0);

    hy_nevra_free(nevra);
    hy_nevra_free(nevra2);
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

    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), -1);
    hy_possibilities_free(iter);
    hy_subject_free(subject);
}
END_TEST

START_TEST(reldep)
{
    DnfReldep *reldep = NULL;
    HySubject subject = hy_subject_create("P-lib");
    HyPossibilities iter = hy_subject_reldep_possibilities_real(subject,
        test_globals.sack, 0);
    ck_assert_int_eq(hy_possibilities_next_reldep(iter, &reldep), 0);
    const gchar *reldep_str = dnf_reldep_to_string (reldep);
    ck_assert_str_eq(reldep_str, "P-lib");
    g_object_unref(reldep);
    ck_assert_int_eq(hy_possibilities_next_reldep(iter, &reldep), -1);
    hy_possibilities_free(iter);
    hy_subject_free(subject);
}
END_TEST

START_TEST(reldep_fail)
{
    DnfReldep *reldep;
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

START_TEST(glob_arch)
{
    HyNevra nevra;
    HySubject subject = hy_subject_create("dog-1-2.i?86");
    HyPossibilities iter = hy_subject_nevra_possibilities_real(subject, NULL,
        test_globals.sack, HY_GLOB);
    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), 0);
    ck_assert_str_eq(nevra->arch, "i?86");
    hy_nevra_free(nevra);
    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), 0);
    fail_unless(nevra->arch == NULL);
    hy_nevra_free(nevra);
    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), -1);
    hy_possibilities_free(iter);
    hy_subject_free(subject);
}
END_TEST

START_TEST(glob_arch_fail)
{
    HyNevra nevra;
    HySubject subject = hy_subject_create("dog-1-2.i*77");
    HyPossibilities iter = hy_subject_nevra_possibilities_real(subject, NULL,
        test_globals.sack, HY_GLOB);
    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), 0);
    fail_unless(nevra->arch == NULL);
    hy_nevra_free(nevra);
    ck_assert_int_eq(hy_possibilities_next_nevra(iter, &nevra), -1);
    hy_possibilities_free(iter);
    hy_subject_free(subject);
}
END_TEST

START_TEST(module_form_nsvcap)
{
    HyModuleForm module_form = hy_module_form_create();

    ck_assert_int_eq(module_form_possibility((char *) module_nsvcap, HY_MODULE_FORM_NSVCAP, module_form), 0);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME), "module-name");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_STREAM), "stream");
    ck_assert_int_eq(hy_module_form_get_version(module_form), 1);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_CONTEXT), "b86c854");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_ARCH), "x86_64");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_PROFILE), "profile");

    hy_module_form_free(module_form);
}
END_TEST

START_TEST(module_form_nsvap)
{
    HyModuleForm module_form = hy_module_form_create();

    ck_assert_int_eq(module_form_possibility((char *) module_nsvap, HY_MODULE_FORM_NSVAP, module_form), 0);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME), "module-name");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_STREAM), "stream");
    ck_assert_int_eq(hy_module_form_get_version(module_form), 1);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_ARCH), "x86_64");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_PROFILE), "profile");

    hy_module_form_free(module_form);
}
END_TEST

START_TEST(module_form_nsvca)
{
    HyModuleForm module_form = hy_module_form_create();

    ck_assert_int_eq(module_form_possibility((char *) module_nsvca, HY_MODULE_FORM_NSVCA, module_form), 0);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME), "module-name");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_STREAM), "stream");
    ck_assert_int_eq(hy_module_form_get_version(module_form), 1);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_CONTEXT), "b86c854");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_ARCH), "x86_64");

    hy_module_form_free(module_form);
}
END_TEST

START_TEST(module_form_nsva)
{
    HyModuleForm module_form = hy_module_form_create();

    ck_assert_int_eq(module_form_possibility((char *) module_nsva, HY_MODULE_FORM_NSVA, module_form), 0);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME), "module-name");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_STREAM), "stream");
    ck_assert_int_eq(hy_module_form_get_version(module_form), 1);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_ARCH), "x86_64");

    hy_module_form_free(module_form);
}
END_TEST

START_TEST(module_form_nsvp)
{
    HyModuleForm module_form = hy_module_form_create();

    ck_assert_int_eq(module_form_possibility((char *) module_nsvp, HY_MODULE_FORM_NSVP, module_form), 0);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME), "module-name");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_STREAM), "stream");
    ck_assert_int_eq(hy_module_form_get_version(module_form), 1);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_PROFILE), "profile");

    hy_module_form_free(module_form);
}
END_TEST

START_TEST(module_form_nsvc)
{
    HyModuleForm module_form = hy_module_form_create();

    ck_assert_int_eq(module_form_possibility((char *) module_nsvc, HY_MODULE_FORM_NSVC, module_form), 0);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME), "module-name");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_STREAM), "stream");
    ck_assert_int_eq(hy_module_form_get_version(module_form), 1);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_CONTEXT), "b86c854");

    hy_module_form_free(module_form);
}
END_TEST

START_TEST(module_form_nsv)
{
    HyModuleForm module_form = hy_module_form_create();

    ck_assert_int_eq(module_form_possibility((char *) module_nsv, HY_MODULE_FORM_NSV, module_form), 0);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME), "module-name");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_STREAM), "stream");
    ck_assert_int_eq(hy_module_form_get_version(module_form), 1);

    hy_module_form_free(module_form);
}
END_TEST

START_TEST(module_form_nsap)
{
    HyModuleForm module_form = hy_module_form_create();

    ck_assert_int_eq(module_form_possibility((char *) module_nsap, HY_MODULE_FORM_NSAP, module_form), 0);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME), "module-name");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_STREAM), "stream");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_ARCH), "x86_64");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_PROFILE), "profile");

    hy_module_form_free(module_form);
}
END_TEST

START_TEST(module_form_nsa)
{
    HyModuleForm module_form = hy_module_form_create();

    ck_assert_int_eq(module_form_possibility((char *) module_nsa, HY_MODULE_FORM_NSA, module_form), 0);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME), "module-name");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_STREAM), "stream");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_ARCH), "x86_64");

    hy_module_form_free(module_form);
}
END_TEST

START_TEST(module_form_nsp)
{
    HyModuleForm module_form = hy_module_form_create();

    ck_assert_int_eq(module_form_possibility((char *) module_nsp, HY_MODULE_FORM_NSP, module_form), 0);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME), "module-name");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_STREAM), "stream");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_PROFILE), "profile");

    hy_module_form_free(module_form);
}
END_TEST

START_TEST(module_form_ns)
{
    HyModuleForm module_form = hy_module_form_create();

    ck_assert_int_eq(module_form_possibility((char *) module_ns, HY_MODULE_FORM_NS, module_form), 0);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME), "module-name");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_STREAM), "stream");

    hy_module_form_free(module_form);
}
END_TEST

START_TEST(module_form_nap)
{
    HyModuleForm module_form = hy_module_form_create();

    ck_assert_int_eq(module_form_possibility((char *) module_nap, HY_MODULE_FORM_NAP, module_form), 0);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME), "module-name");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_ARCH), "x86_64");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_PROFILE), "profile");

    hy_module_form_free(module_form);
}
END_TEST

START_TEST(module_form_na)
{
    HyModuleForm module_form = hy_module_form_create();

    ck_assert_int_eq(module_form_possibility((char *) module_na, HY_MODULE_FORM_NA, module_form), 0);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME), "module-name");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_ARCH), "x86_64");

    hy_module_form_free(module_form);
}
END_TEST

START_TEST(module_form_np)
{
    HyModuleForm module_form = hy_module_form_create();

    ck_assert_int_eq(module_form_possibility((char *) module_np, HY_MODULE_FORM_NP, module_form), 0);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME), "module-name");
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_PROFILE), "profile");

    hy_module_form_free(module_form);
}
END_TEST

START_TEST(module_form_n)
{
    HyModuleForm module_form = hy_module_form_create();

    ck_assert_int_eq(module_form_possibility((char *) module_n, HY_MODULE_FORM_N, module_form), 0);
    ck_assert_str_eq(hy_module_form_get_string(module_form, HY_MODULE_FORM_NAME), "module-name");

    hy_module_form_free(module_form);
}
END_TEST

START_TEST(module_form_combined)
{
    HyModuleForm module_form;
    HySubject subject = hy_subject_create(module_nsvap);
    HyPossibilities iter = hy_subject_module_form_possibilities(subject, NULL);

    ck_assert_int_eq(hy_possibilities_next_module_form(iter, &module_form), 0);
    ck_assert_str_eq(module_form->name, "module-name");
    ck_assert_str_eq(module_form->stream, "stream");
    ck_assert_int_eq(module_form->version, 1);
    fail_unless(module_form->context == NULL);
    ck_assert_str_eq(module_form->arch, "x86_64");
    ck_assert_str_eq(module_form->profile, "profile");
    hy_module_form_free(module_form);

    ck_assert_int_eq(hy_possibilities_next_module_form(iter, &module_form), -1);
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
    tcase_add_test(tc, module_form_nsvcap);
    tcase_add_test(tc, module_form_nsvap);
    tcase_add_test(tc, module_form_nsvca);
    tcase_add_test(tc, module_form_nsva);
    tcase_add_test(tc, module_form_nsvp);
    tcase_add_test(tc, module_form_nsvc);
    tcase_add_test(tc, module_form_nsv);
    tcase_add_test(tc, module_form_nsap);
    tcase_add_test(tc, module_form_nsa);
    tcase_add_test(tc, module_form_nsp);
    tcase_add_test(tc, module_form_ns);
    tcase_add_test(tc, module_form_nap);
    tcase_add_test(tc, module_form_na);
    tcase_add_test(tc, module_form_np);
    tcase_add_test(tc, module_form_n);
    tcase_add_test(tc, module_form_combined);
    suite_add_tcase(s, tc);

    tc = tcase_create("Full");
    tcase_add_unchecked_fixture(tc, fixture_all, teardown);
    tcase_add_test(tc, reldep);
    tcase_add_test(tc, reldep_fail);
    tcase_add_test(tc, nevra_real_none);
    tcase_add_test(tc, nevra_real);
    tcase_add_test(tc, nevra_real_dash);
    tcase_add_test(tc, glob_arch);
    tcase_add_test(tc, glob_arch_fail);
    suite_add_tcase(s, tc);

    return s;
}
