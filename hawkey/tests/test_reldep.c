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

// hawkey
#include "src/package.h"
#include "src/reldep.h"
#include "src/sack.h"
#include "fixtures.h"
#include "test_suites.h"
#include "testsys.h"

START_TEST(test_reldeplist_add)
{
    HySack sack = test_globals.sack;
    HyPackage flying = by_name_repo(sack, "fool", "updates");
    HyReldepList reldeplist = hy_reldeplist_create(sack);
    HyReldepList obsoletes = hy_package_get_obsoletes(flying);

    const int count = hy_reldeplist_count(obsoletes);
    fail_unless(count == 2);
    for (int i = 0; i < count; ++i) {
	HyReldep reldep = hy_reldeplist_get_clone(obsoletes, i);
	hy_reldeplist_add(reldeplist, reldep);
	hy_reldep_free(reldep);
    }

    hy_package_free(flying);
    fail_unless(hy_reldeplist_count(reldeplist) == 2);
    hy_reldeplist_free(obsoletes);
    hy_reldeplist_free(reldeplist);
}
END_TEST

Suite *
reldep_suite(void)
{
    Suite *s = suite_create("Reldep");
    TCase *tc = tcase_create("Core");
    tcase_add_unchecked_fixture(tc, fixture_with_updates, teardown);
    tcase_add_test(tc, test_reldeplist_add);
    suite_add_tcase(s, tc);

    return s;
}
