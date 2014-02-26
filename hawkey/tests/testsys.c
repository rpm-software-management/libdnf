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
#include <check.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

// libsolv
#include <solv/repo.h>
#include <solv/util.h>

// hawkey
#include "src/goal.h"
#include "src/package.h"
#include "src/package_internal.h"
#include "src/query.h"
#include "src/sack_internal.h"
#include "src/util.h"
#include "testsys.h"

void
assert_nevra_eq(HyPackage pkg, const char *nevra)
{
    char *pkg_nevra = hy_package_get_nevra(pkg);
    ck_assert_str_eq(pkg_nevra, nevra);
    solv_free(pkg_nevra);
}

HyPackage
by_name(HySack sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    HyPackageList plist = hy_query_run(q);
    hy_query_free(q);
    HyPackage pkg = hy_packagelist_get_clone(plist, 0);
    hy_packagelist_free(plist);

    return pkg;
}

HyPackage
by_name_repo(HySack sack, const char *name, const char *repo)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPONAME, HY_EQ, repo);
    HyPackageList plist = hy_query_run(q);
    hy_query_free(q);
    HyPackage pkg = hy_packagelist_get_clone(plist, 0);
    hy_packagelist_free(plist);

    return pkg;
}

void
dump_packagelist(HyPackageList plist, int free)
{
    for (int i = 0; i < hy_packagelist_count(plist); ++i) {
	HyPackage pkg = hy_packagelist_get(plist, i);
	Solvable *s = pool_id2solvable(package_pool(pkg), package_id(pkg));
	char *nvra = hy_package_get_nevra(pkg);
	printf("\t%s @%s\n", nvra, s->repo->name);
	hy_free(nvra);
    }
    if (free)
	hy_packagelist_free(plist);
}

void
dump_query_results(HyQuery query)
{
    HyPackageList plist = hy_query_run(query);
    dump_packagelist(plist, 1);
}

void
dump_goal_results(HyGoal goal)
{
    printf("installs:\n");
    dump_packagelist(hy_goal_list_installs(goal), 1);
    printf("upgrades:\n");
    dump_packagelist(hy_goal_list_upgrades(goal), 1);
    printf("erasures:\n");
    dump_packagelist(hy_goal_list_erasures(goal), 1);
    printf("reinstalls:\n");
    dump_packagelist(hy_goal_list_reinstalls(goal), 1);
}

int
logfile_size(HySack sack)
{
    const int fd = fileno(sack->log_out);
    struct stat st;

    fstat(fd, &st);
    return st.st_size;
}

int
query_count_results(HyQuery query)
{
    HyPackageList plist = hy_query_run(query);
    int ret = hy_packagelist_count(plist);
    hy_packagelist_free(plist);
    return ret;
}
