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


#include "libdnf/nevra.hpp"
#include "libdnf/nsvcap.hpp"
#include "libdnf/dnf-reldep.h"
#include "libdnf/dnf-sack.h"
#include "libdnf/hy-subject.h"
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
const char module_nsvp[] = "module-name:stream:1/profile";
const char module_nsap[] = "module-name:stream::x86_64/profile";
const char module_nsp[] = "module-name:stream/profile";
const char module_np[] = "module-name/profile";
const char module_nap[] = "module-name::x86_64/profile";

/* without profile */
const char module_nsvca[] = "module-name:stream:1:b86c854:x86_64";
const char module_nsva[] = "module-name:stream:1::x86_64";
const char module_nsvc[] = "module-name:stream:1:b86c854";
const char module_nsvc2[] = "module-name:stream:1:muj-cont_ext.5";
const char module_nsv[] = "module-name:stream:1";
const char module_nsa[] = "module-name:stream::x86_64";
const char module_ns[] = "module-name:stream";
const char module_n[] = "module-name";
const char module_na[] = "module-name::x86_64";

START_TEST(nevra1)
{
    libdnf::Nevra nevra;
    ck_assert(nevra.parse(inp_fof, HY_FORM_NEVRA));
    ck_assert_str_eq(nevra.getName().c_str(), "four-of-fish");
    ck_assert_int_eq(nevra.getEpoch(), 8);
    ck_assert_str_eq(nevra.getVersion().c_str(), "3.6.9");
    ck_assert_str_eq(nevra.getRelease().c_str(), "11.fc100");
    ck_assert_str_eq(nevra.getArch().c_str(), "x86_64");

    // NEVRA comparison tests
    libdnf::Nevra nevra2;
    nevra2.setEpoch(8);
    nevra2.setName("four-of-fish");
    nevra2.setVersion("3.6.9");
    nevra2.setRelease("11.fc100");
    nevra2.setArch("x86_64");
    ck_assert_int_eq(nevra.compare(nevra2), 0);

    nevra2.setEpoch(3);
    ck_assert_int_gt(nevra.compare(nevra2), 0);

    nevra2.setEpoch(11);
    ck_assert_int_lt(nevra.compare(nevra2), 0);

    nevra2.setEpoch(8);
    nevra2.setVersion("7.0");
    ck_assert_int_lt(nevra.compare(nevra2), 0);

    nevra2.setEpoch(8);
    nevra2.setVersion("");
    ck_assert_int_gt(nevra.compare(nevra2), 0);

    nevra2.setEpoch(8);
    nevra.setVersion("");
    ck_assert_int_eq(nevra.compare(nevra2), 0);
}
END_TEST


START_TEST(nevra2)
{
    libdnf::Nevra nevra;
    ck_assert(nevra.parse(inp_fof_noepoch, HY_FORM_NEVRA));
    ck_assert_str_eq(nevra.getName().c_str(), "four-of-fish");
    ck_assert_int_eq(nevra.getEpoch(), libdnf::Nevra::EPOCH_NOT_SET);
    ck_assert_str_eq(nevra.getVersion().c_str(), "3.6.9");
    ck_assert_str_eq(nevra.getRelease().c_str(), "11.fc100");
    ck_assert_str_eq(nevra.getArch().c_str(), "x86_64");
}
END_TEST

START_TEST(nevr)
{
    libdnf::Nevra nevra;
    ck_assert(nevra.parse(inp_fof, HY_FORM_NEVR));
    ck_assert_str_eq(nevra.getName().c_str(), "four-of-fish");
    ck_assert_int_eq(nevra.getEpoch(), 8);
    ck_assert_str_eq(nevra.getVersion().c_str(), "3.6.9");
    ck_assert_str_eq(nevra.getRelease().c_str(), "11.fc100.x86_64");
    fail_unless(nevra.getArch().empty());
}
END_TEST

START_TEST(nevr_fail)
{
    libdnf::Nevra nevra;
    ck_assert(!nevra.parse("four-of", HY_FORM_NEVR));
}
END_TEST

START_TEST(nev)
{
    libdnf::Nevra nevra;
    ck_assert(nevra.parse(inp_fof_nev, HY_FORM_NEV));
    ck_assert_str_eq(nevra.getName().c_str(), "four-of-fish");
    ck_assert_int_eq(nevra.getEpoch(), 8);
    ck_assert_str_eq(nevra.getVersion().c_str(), "3.6.9");
    fail_unless(nevra.getRelease().empty());
    fail_unless(nevra.getArch().empty());
}
END_TEST

START_TEST(na)
{
    libdnf::Nevra nevra;
    ck_assert(nevra.parse(inp_fof_na, HY_FORM_NA));
    ck_assert_str_eq(nevra.getName().c_str(), "four-of-fish-3.6.9");
    ck_assert_int_eq(nevra.getEpoch(), libdnf::Nevra::EPOCH_NOT_SET);
    fail_unless(nevra.getVersion().empty());
    ck_assert_str_eq(nevra.getArch().c_str(), "i686");
}
END_TEST

START_TEST(module_form_nsvcap)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_nsvcap, HY_MODULE_FORM_NSVCAP));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
    ck_assert_str_eq(nsvcap.getStream().c_str(), "stream");
    ck_assert_str_eq(nsvcap.getVersion().c_str(), "1");
    ck_assert_str_eq(nsvcap.getContext().c_str(), "b86c854");
    ck_assert_str_eq(nsvcap.getArch().c_str(), "x86_64");
    ck_assert_str_eq(nsvcap.getProfile().c_str(), "profile");
}
END_TEST

START_TEST(module_form_nsvap)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_nsvap, HY_MODULE_FORM_NSVAP));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
    ck_assert_str_eq(nsvcap.getStream().c_str(), "stream");
    ck_assert_str_eq(nsvcap.getVersion().c_str(), "1");
    ck_assert_str_eq(nsvcap.getArch().c_str(), "x86_64");
    ck_assert_str_eq(nsvcap.getProfile().c_str(), "profile");
}
END_TEST

START_TEST(module_form_nsvca)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_nsvca, HY_MODULE_FORM_NSVCA));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
    ck_assert_str_eq(nsvcap.getStream().c_str(), "stream");
    ck_assert_str_eq(nsvcap.getVersion().c_str(), "1");
    ck_assert_str_eq(nsvcap.getContext().c_str(), "b86c854");
    ck_assert_str_eq(nsvcap.getArch().c_str(), "x86_64");
}
END_TEST

START_TEST(module_form_nsva)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_nsva, HY_MODULE_FORM_NSVA));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
    ck_assert_str_eq(nsvcap.getStream().c_str(), "stream");
    ck_assert_str_eq(nsvcap.getVersion().c_str(), "1");
    ck_assert_str_eq(nsvcap.getArch().c_str(), "x86_64");
}
END_TEST

START_TEST(module_form_nsvp)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_nsvp, HY_MODULE_FORM_NSVP));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
    ck_assert_str_eq(nsvcap.getStream().c_str(), "stream");
    ck_assert_str_eq(nsvcap.getVersion().c_str(), "1");
    ck_assert_str_eq(nsvcap.getProfile().c_str(), "profile");
}
END_TEST

START_TEST(module_form_nsvc)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_nsvc, HY_MODULE_FORM_NSVC));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
    ck_assert_str_eq(nsvcap.getStream().c_str(), "stream");
    ck_assert_str_eq(nsvcap.getVersion().c_str(), "1");
    ck_assert_str_eq(nsvcap.getContext().c_str(), "b86c854");
}
END_TEST

START_TEST(module_form_nsvc2)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_nsvc2, HY_MODULE_FORM_NSVC));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
    ck_assert_str_eq(nsvcap.getStream().c_str(), "stream");
    ck_assert_str_eq(nsvcap.getVersion().c_str(), "1");
    ck_assert_str_eq(nsvcap.getContext().c_str(), "muj-cont_ext.5");
}
END_TEST

START_TEST(module_form_nsv)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_nsv, HY_MODULE_FORM_NSV));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
    ck_assert_str_eq(nsvcap.getStream().c_str(), "stream");
    ck_assert_str_eq(nsvcap.getVersion().c_str(), "1");
}
END_TEST

START_TEST(module_form_nsap)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_nsap, HY_MODULE_FORM_NSAP));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
    ck_assert_str_eq(nsvcap.getStream().c_str(), "stream");
    ck_assert_str_eq(nsvcap.getArch().c_str(), "x86_64");
    ck_assert_str_eq(nsvcap.getProfile().c_str(), "profile");
}
END_TEST

START_TEST(module_form_nsa)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_nsa, HY_MODULE_FORM_NSA));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
    ck_assert_str_eq(nsvcap.getStream().c_str(), "stream");
    ck_assert_str_eq(nsvcap.getArch().c_str(), "x86_64");
}
END_TEST

START_TEST(module_form_nsp)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_nsp, HY_MODULE_FORM_NSP));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
    ck_assert_str_eq(nsvcap.getStream().c_str(), "stream");
    ck_assert_str_eq(nsvcap.getProfile().c_str(), "profile");
}
END_TEST

START_TEST(module_form_ns)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_ns, HY_MODULE_FORM_NS));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
    ck_assert_str_eq(nsvcap.getStream().c_str(), "stream");
}
END_TEST

START_TEST(module_form_nap)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_nap, HY_MODULE_FORM_NAP));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
    ck_assert_str_eq(nsvcap.getArch().c_str(), "x86_64");
    ck_assert_str_eq(nsvcap.getProfile().c_str(), "profile");
}
END_TEST

START_TEST(module_form_na)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_na, HY_MODULE_FORM_NA));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
    ck_assert_str_eq(nsvcap.getArch().c_str(), "x86_64");
}
END_TEST

START_TEST(module_form_np)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_np, HY_MODULE_FORM_NP));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
    ck_assert_str_eq(nsvcap.getProfile().c_str(), "profile");
}
END_TEST

START_TEST(module_form_n)
{
    libdnf::Nsvcap nsvcap;
    ck_assert(nsvcap.parse(module_n, HY_MODULE_FORM_N));
    ck_assert_str_eq(nsvcap.getName().c_str(), "module-name");
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
    tcase_add_test(tc, module_form_nsvcap);
    tcase_add_test(tc, module_form_nsvap);
    tcase_add_test(tc, module_form_nsvca);
    tcase_add_test(tc, module_form_nsva);
    tcase_add_test(tc, module_form_nsvp);
    tcase_add_test(tc, module_form_nsvc);
    tcase_add_test(tc, module_form_nsvc2);
    tcase_add_test(tc, module_form_nsv);
    tcase_add_test(tc, module_form_nsap);
    tcase_add_test(tc, module_form_nsa);
    tcase_add_test(tc, module_form_nsp);
    tcase_add_test(tc, module_form_ns);
    tcase_add_test(tc, module_form_nap);
    tcase_add_test(tc, module_form_na);
    tcase_add_test(tc, module_form_np);
    tcase_add_test(tc, module_form_n);
    suite_add_tcase(s, tc);

    tc = tcase_create("Full");
    tcase_add_unchecked_fixture(tc, fixture_all, teardown);
    suite_add_tcase(s, tc);

    return s;
}
