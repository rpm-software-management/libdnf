/*
 * Copyright (C) 2012-2015 Red Hat, Inc.
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

#include <check.h>
#include <stdarg.h>
#include <unistd.h>


#include "libdnf/hy-package.h"
#include "libdnf/hy-iutil.h"
#include "libdnf/dnf-sack-private.hpp"
#include "fixtures.h"
#include "testsys.h"

/* define the global variable */
struct TestGlobals_s test_globals;

static DnfSack *
create_ut_sack(void)
{
    DnfSack *sack = dnf_sack_new();
    dnf_sack_set_cachedir(sack, test_globals.tmpdir);
    dnf_sack_set_arch(sack, TEST_FIXED_ARCH, NULL);
    dnf_sack_setup(sack, DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR, NULL);
    test_globals.sack = sack;
    g_debug("DnfSack for UT created: %p", sack);
    return sack;
}

static int
setup_with(DnfSack *sack, ...)
{
    Pool *pool = dnf_sack_get_pool(sack);
    va_list names;
    int ret = 0;

    va_start(names, sack);
    const char *name = va_arg(names, const char *);
    while (name) {
        const char *path = pool_tmpjoin(pool, test_globals.repo_dir,
                                        name, ".repo");
        int installed = !strncmp(name, HY_SYSTEM_REPO_NAME,
                                 strlen(HY_SYSTEM_REPO_NAME));

        ret |= load_repo(pool, name, path, installed);
        name = va_arg(names, const char *);
    }
    va_end(names);
    return ret;
}

static
void add_cmdline(DnfSack *sack)
{
    Pool *pool = dnf_sack_get_pool(sack);
    const char *path = pool_tmpjoin(pool, test_globals.repo_dir,
                                    "yum/tour-4-6.noarch.rpm", NULL);
    DnfPackage *pkg = dnf_sack_add_cmdline_package(sack, path);
    g_object_unref(pkg);
}

void
fixture_cmdline_only(void)
{
    DnfSack *sack = create_ut_sack();
    add_cmdline(sack);
}

void
fixture_empty(void)
{
    create_ut_sack();
}

void
fixture_greedy_only(void)
{
    DnfSack *sack = create_ut_sack();
    fail_if(setup_with(sack, "greedy", NULL));
}

void
fixture_installonly(void)
{
    DnfSack *sack = create_ut_sack();
    fail_if(setup_with(sack, "@System-k", "installonly", NULL));
}

void
fixture_system_only(void)
{
    DnfSack *sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, NULL));
}

void
fixture_verify(void)
{
    DnfSack *sack = create_ut_sack();
    fail_if(setup_with(sack, "@System-broken", NULL));
}

void
fixture_with_change(void)
{
    DnfSack *sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, "change", NULL));
}

void
fixture_with_cmdline(void)
{
    DnfSack *sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, NULL));
    add_cmdline(sack);
}

void
fixture_with_forcebest(void)
{
    DnfSack *sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, "forcebest", NULL));
}

void
fixture_with_main(void)
{
    DnfSack *sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, "main", NULL));
}

void
fixture_with_updates(void)
{
    DnfSack *sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, "updates", NULL));
}

void
fixture_with_vendor(void)
{
    DnfSack *sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, "vendor", NULL));
}

void
fixture_all(void)
{
    DnfSack *sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, "main", "updates", NULL));
}

void fixture_yum(void)
{
    DnfSack *sack = create_ut_sack();
    setup_yum_sack(sack, YUM_REPO_NAME);
}

void fixture_reset(void)
{
    DnfSack *sack = test_globals.sack;
    dnf_sack_set_installonly(sack, NULL);
    dnf_sack_set_installonly_limit(sack, 0);
    dnf_sack_set_excludes(sack, NULL);
    dnf_sack_repo_enabled(sack, "main", 1);
    dnf_sack_repo_enabled(sack, "updates", 1);
}

void setup_yum_sack(DnfSack *sack, const char *yum_repo_name)
{
    Pool *pool = dnf_sack_get_pool(sack);
    const char *repo_path = pool_tmpjoin(pool, test_globals.repo_dir,
                                         YUM_DIR_SUFFIX, NULL);
    fail_if(access(repo_path, X_OK));
    HyRepo repo = glob_for_repofiles(pool, yum_repo_name, repo_path);

    fail_if(!dnf_sack_load_repo(sack, repo,
                               DNF_SACK_LOAD_FLAG_BUILD_CACHE |
                               DNF_SACK_LOAD_FLAG_USE_FILELISTS |
                               DNF_SACK_LOAD_FLAG_USE_UPDATEINFO |
                               DNF_SACK_LOAD_FLAG_USE_PRESTO, NULL));
    fail_unless(dnf_sack_count(sack) == TEST_EXPECT_YUM_NSOLVABLES);
    hy_repo_free(repo);
}

void
teardown(void)
{
    g_object_unref(test_globals.sack);
    test_globals.sack = NULL;
}
