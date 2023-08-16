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

#include "libdnf/hy-goal.h"
#include "libdnf/hy-package.h"
#include "libdnf/hy-package-private.hpp"
#include "libdnf/hy-query.h"
#include "libdnf/dnf-sack-private.hpp"
#include "libdnf/hy-util.h"

#include "testsys.h"

#include <solv/repo.h>
#include <solv/util.h>

#include <check.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

void
assert_nevra_eq(DnfPackage *pkg, const char *nevra)
{
    const char *pkg_nevra = dnf_package_get_nevra(pkg);
    ck_assert_str_eq(pkg_nevra, nevra);
}

DnfPackage *
by_name(DnfSack *sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    GPtrArray *plist = hy_query_run(q);
    hy_query_free(q);
    auto pkg = static_cast<DnfPackage *>(g_object_ref(g_ptr_array_index(plist, 0)));
    g_ptr_array_unref(plist);

    return pkg;
}

DnfPackage *
by_name_repo(DnfSack *sack, const char *name, const char *repo)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPONAME, HY_EQ, repo);
    GPtrArray *plist = hy_query_run(q);
    hy_query_free(q);
    auto pkg = static_cast<DnfPackage *>(g_object_ref(g_ptr_array_index(plist, 0)));
    g_ptr_array_unref(plist);

    return pkg;
}

void
dump_packagelist(GPtrArray *plist, int free)
{
    for (guint i = 0; i < plist->len; ++i) {
        auto pkg = static_cast<DnfPackage *>(g_ptr_array_index(plist, i));
        Solvable *s = pool_id2solvable(dnf_package_get_pool(pkg), dnf_package_get_id(pkg));
        const char *nvra = dnf_package_get_nevra(pkg);
        printf("\t%s @%s\n", nvra, s->repo->name);
    }
    if (free)
        g_ptr_array_unref(plist);
}

void
dump_query_results(HyQuery query)
{
    GPtrArray *plist = hy_query_run(query);
    dump_packagelist(plist, 1);
}

void
dump_goal_results(HyGoal goal)
{
    printf("installs:\n");
    dump_packagelist(hy_goal_list_installs(goal, NULL), 1);
    printf("upgrades:\n");
    dump_packagelist(hy_goal_list_upgrades(goal, NULL), 1);
    printf("erasures:\n");
    dump_packagelist(hy_goal_list_erasures(goal, NULL), 1);
    printf("reinstalls:\n");
    dump_packagelist(hy_goal_list_reinstalls(goal, NULL), 1);
}

int
query_count_results(HyQuery query)
{
    GPtrArray *plist = hy_query_run(query);
    int ret = plist->len;
    g_ptr_array_unref(plist);
    return ret;
}
