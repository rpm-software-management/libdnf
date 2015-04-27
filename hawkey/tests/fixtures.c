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

// hawkey
#include "src/package.h"
#include "src/repo.h"
#include "src/iutil.h"
#include "src/sack_internal.h"
#include "fixtures.h"
#include "testsys.h"

/* define the global variable */
struct TestGlobals_s test_globals;

static HySack
create_ut_sack(void)
{
    HySack sack = hy_sack_create(test_globals.tmpdir, TEST_FIXED_ARCH, NULL,
				 NULL, HY_MAKE_CACHE_DIR);
    test_globals.sack = sack;
    HY_LOG_INFO("HySack for UT created: %p", sack);
    return sack;
}

static int
setup_with(HySack sack, ...)
{
    Pool *pool = sack_pool(sack);
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
void add_cmdline(HySack sack)
{
    Pool *pool = sack_pool(sack);
    const char *path = pool_tmpjoin(pool, test_globals.repo_dir,
				    "yum/tour-4-6.noarch.rpm", NULL);
    HyPackage pkg = hy_sack_add_cmdline_package(sack, path);
    hy_package_free(pkg);
}

void
fixture_cmdline_only(void)
{
    HySack sack = create_ut_sack();
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
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, "greedy", NULL));
}

void
fixture_installonly(void)
{
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, "@System-k", "installonly", NULL));
}

void
fixture_system_only(void)
{
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, NULL));
}

void
fixture_verify(void)
{
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, "@System-broken", NULL));
}

void
fixture_with_change(void)
{
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, "change", NULL));
}

void
fixture_with_cmdline(void)
{
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, NULL));
    add_cmdline(sack);
}

void
fixture_with_forcebest(void)
{
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, "forcebest", NULL));
}

void
fixture_with_main(void)
{
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, "main", NULL));
}

void
fixture_with_updates(void)
{
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, "updates", NULL));
}

void
fixture_with_vendor(void)
{
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, "vendor", NULL));
}

void
fixture_all(void)
{
    HySack sack = create_ut_sack();
    fail_if(setup_with(sack, HY_SYSTEM_REPO_NAME, "main", "updates", NULL));
}

void fixture_yum(void)
{
    HySack sack = create_ut_sack();
    setup_yum_sack(sack, YUM_REPO_NAME);
}

void fixture_reset(void)
{
    HySack sack = test_globals.sack;
    hy_sack_set_installonly(sack, NULL);
    hy_sack_set_installonly_limit(sack, 0);
    hy_sack_set_excludes(sack, NULL);
    hy_sack_repo_enabled(sack, "main", 1);
    hy_sack_repo_enabled(sack, "updates", 1);
}

void setup_yum_sack(HySack sack, const char *yum_repo_name)
{
    Pool *pool = sack_pool(sack);
    const char *repo_path = pool_tmpjoin(pool, test_globals.repo_dir,
					 YUM_DIR_SUFFIX, NULL);
    fail_if(access(repo_path, X_OK));
    HyRepo repo = glob_for_repofiles(pool, yum_repo_name, repo_path);

    fail_if(hy_sack_load_repo(sack, repo,
                              HY_BUILD_CACHE |
                              HY_LOAD_FILELISTS |
                              HY_LOAD_UPDATEINFO |
                              HY_LOAD_PRESTO));
    fail_unless(hy_sack_count(sack) == TEST_EXPECT_YUM_NSOLVABLES);
    hy_repo_free(repo);
}

void
teardown(void)
{
    hy_sack_free(test_globals.sack);
    test_globals.sack = NULL;
}
