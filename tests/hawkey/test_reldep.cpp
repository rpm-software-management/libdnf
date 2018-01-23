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


#include <memory>

#include "libdnf/hy-package.h"
#include "libdnf/dnf-reldep.h"
#include "libdnf/dnf-reldep-list.h"
#include "libdnf/dnf-sack.h"
#include "fixtures.h"
#include "test_suites.h"
#include "testsys.h"
#include "libdnf/repo/solvable/DependencyContainer.hpp"

START_TEST(test_reldeplist_add)
{
    DnfSack *sack = test_globals.sack;
    DnfPackage *flying = by_name_repo(sack, "fool", "updates");
    std::unique_ptr<DnfReldepList> reldeplist(dnf_reldep_list_new (sack));
    std::unique_ptr<DnfReldepList> obsoletes(dnf_package_get_obsoletes (flying));

    const int count = dnf_reldep_list_count (obsoletes.get());
    fail_unless(count == 2);
    for (int i = 0; i < count; ++i) {
        DnfReldep *reldep = dnf_reldep_list_index (obsoletes.get(), i);
        dnf_reldep_list_add (reldeplist.get(), reldep);
    }

    g_object_unref (flying);
    fail_unless(dnf_reldep_list_count (reldeplist.get()) == 2);
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
