/*
 * Copyright (C) 2012-2014 Red Hat, Inc.
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

// libsolv
#include <solv/testcase.h>

// hawkey
#include "src/query.h"
#include "src/query_internal.h"
#include "src/package.h"
#include "src/packageset.h"
#include "src/reldep.h"
#include "src/sack_internal.h"
#include "fixtures.h"
#include "test_suites.h"
#include "testsys.h"

static int
size_and_free(HyQuery query)
{
    int c = query_count_results(query);
    hy_query_free(query);
    return c;
}

START_TEST(test_query_sanity)
{
    HySack sack = test_globals.sack;
    fail_unless(sack != NULL);
    fail_unless(hy_sack_count(sack) == TEST_EXPECT_SYSTEM_NSOLVABLES);
    fail_unless(sack_pool(sack)->installed != NULL);

    HyQuery query = hy_query_create(sack);
    fail_unless(query != NULL);
    hy_query_free(query);
}
END_TEST

START_TEST(test_query_run_set_sanity)
{
    HySack sack = test_globals.sack;
    HyQuery q = hy_query_create(sack);
    HyPackageSet pset = hy_query_run_set(q);

    // make sure we are testing with some odd bits in the underlying map:
    fail_unless(TEST_EXPECT_SYSTEM_NSOLVABLES % 8);
    fail_unless(hy_packageset_count(pset) == TEST_EXPECT_SYSTEM_NSOLVABLES);
    hy_packageset_free(pset);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_clear)
{
    HyQuery q;

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_NEQ, "fool");
    hy_query_clear(q);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "fool");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_clone)
{
    const char *namelist[] = {"penny", "fool", NULL};
    HyQuery q = hy_query_create(test_globals.sack);

    hy_query_filter_in(q, HY_PKG_NAME, HY_EQ, namelist);
    HyQuery clone = hy_query_clone(q);
    hy_query_free(q);
    fail_unless(query_count_results(clone) == 2);
    hy_query_free(clone);
}
END_TEST

START_TEST(test_query_empty)
{
    HyQuery q = hy_query_create(test_globals.sack);
    fail_if(hy_query_filter_empty(q));
    fail_unless(query_count_results(q) == 0);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_repo)
{
    HyQuery q;

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);

    fail_unless(query_count_results(q) == TEST_EXPECT_SYSTEM_NSOLVABLES);

    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
    fail_if(query_count_results(q));

    hy_query_free(q);
}
END_TEST

START_TEST(test_query_name)
{
    HyQuery q;

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_SUBSTR, "penny");
    fail_unless(query_count_results(q) == 2);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "lane");
    fail_if(query_count_results(q));
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_evr)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_EVR, HY_EQ, "6.0-0");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_EVR, HY_GT, "5.9-0");
    fail_unless(query_count_results(q) == 2);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_EVR, HY_GT|HY_EQ, "5.0-0");
    fail_unless(query_count_results(q) == 3);
    hy_query_free(q);

    const char *evrs[] = {"6.0-0", "2-9", "5.0-0", "0-100", NULL};
    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_EVR, HY_EQ, evrs);
    fail_unless(query_count_results(q) == 3);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_epoch)
{
    HyQuery q = hy_query_create(test_globals.sack);
    fail_unless(hy_query_filter(q, HY_PKG_EPOCH, HY_GT|HY_EQ, "1"));
    fail_if(hy_query_filter_num(q, HY_PKG_EPOCH, HY_GT|HY_EQ, 1));
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_version)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_VERSION, HY_EQ, "5.0");
    fail_unless(query_count_results(q) == 2);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_VERSION, HY_GT, "5.2.1");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_VERSION, HY_GT|HY_EQ, "5.");
    fail_unless(query_count_results(q) == 3);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_location)
{
     HyQuery q = hy_query_create(test_globals.sack);
    fail_unless(hy_query_filter(q, HY_PKG_LOCATION, HY_GT,
				"tour-4-6.noarch.rpm"));
    fail_if(hy_query_filter(q, HY_PKG_LOCATION, HY_EQ,
			    "tour-4-6.noarch.rpm"));
    fail_unless(size_and_free(q) == 1);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_LOCATION, HY_EQ,
		    "mystery-devel-19.67-1.noarch.rpm");
    fail_unless(size_and_free(q) == 1);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_LOCATION, HY_EQ,
		    "mystery-19.67-1.src.rpm");
    fail_unless(size_and_free(q) == 0);
}
END_TEST

START_TEST(test_query_release)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_RELEASE, HY_EQ, "11");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_RELEASE, HY_GT, "9");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_RELEASE, HY_GT|HY_EQ, "9");
    fail_unless(query_count_results(q) == 2);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_glob)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_GLOB, "pen*");
    fail_unless(query_count_results(q) == 2);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_case)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "Penny-lib");
    fail_unless(query_count_results(q) == 0);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ|HY_ICASE, "Penny-lib");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_anded)
{
    HyQuery q;

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_SUBSTR, "penny");
    hy_query_filter(q, HY_PKG_SUMMARY, HY_SUBSTR, "ears");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_neq)
{
    HyQuery q;

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_NEQ, "penny-lib");
    fail_unless(query_count_results(q) == TEST_EXPECT_SYSTEM_PKGS - 1);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_in)
{
    HyQuery q;
    const char *namelist[] = {"penny", "fool", NULL};

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_NAME, HY_EQ, namelist);
    fail_unless(query_count_results(q) == 2);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_pkg)
{
    HyPackageSet pset;
    HyQuery q, q2;

    // setup
    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "jay");
    pset = hy_query_run_set(q);
    hy_query_free(q);
    fail_unless(hy_packageset_count(pset), 2);

    // use hy_query_filter_package_in():
    q = hy_query_create(test_globals.sack);
    // check validation works:
    fail_unless(hy_query_filter_package_in(q, HY_PKG, HY_GT, pset));
    // add the filter:
    fail_if(hy_query_filter_package_in(q, HY_PKG, HY_EQ, pset));
    hy_packageset_free(pset);

    // cloning must work
    q2 = hy_query_clone(q);
    fail_unless(query_count_results(q) == 2);
    hy_query_free(q);

    // filter on
    hy_query_filter_latest_per_arch(q2, 1);
    pset = hy_query_run_set(q2);
    fail_unless(hy_packageset_count(pset) == 1);
    HyPackage pkg = hy_packageset_get_clone(pset, 0);
    char *nvra = hy_package_get_nevra(pkg);
    ck_assert_str_eq(nvra, "jay-6.0-0.x86_64");
    solv_free(nvra);
    hy_package_free(pkg);

    hy_packageset_free(pset);
    hy_query_free(q2);
}
END_TEST

START_TEST(test_query_provides)
{
    HySack sack = test_globals.sack;
    HyQuery q;

    q = hy_query_create(sack);
    hy_query_filter_provides(q, HY_LT, "fool", "2.0");
    fail_unless(size_and_free(q) == 1);

    q = hy_query_create(sack);
    hy_query_filter_provides(q, HY_GT, "fool", "2.0");
    fail_unless(size_and_free(q) == 0);

    q = hy_query_create(sack);
    hy_query_filter_provides(q, HY_EQ, "P", "3-3");
    fail_unless(size_and_free(q) == 1);
}
END_TEST

START_TEST(test_query_recommends)
{
    HyPackageList plist;
    HyPackage pkg;
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_RECOMMENDS, HY_EQ, "baby");
    plist = hy_query_run(q);
    pkg = hy_packagelist_get(plist, 0);
    ck_assert_str_eq(hy_package_get_name(pkg), "flying");
    hy_query_free(q);
    hy_packagelist_free(plist);
}
END_TEST

START_TEST(test_query_suggests)
{
    HyPackageList plist;
    HyPackage pkg;
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_SUGGESTS, HY_EQ, "walrus");
    plist = hy_query_run(q);
    pkg = hy_packagelist_get(plist, 0);
    ck_assert_str_eq(hy_package_get_name(pkg), "flying");
    hy_query_free(q);
    hy_packagelist_free(plist);
}
END_TEST

START_TEST(test_query_supplements)
{
    HyPackageList plist;
    HyPackage pkg;
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_SUPPLEMENTS, HY_EQ, "flying");
    plist = hy_query_run(q);
    pkg = hy_packagelist_get(plist, 0);
    ck_assert_str_eq(hy_package_get_name(pkg), "baby");
    hy_query_free(q);
    hy_packagelist_free(plist);
}
END_TEST

START_TEST(test_query_enhances)
{
    HyPackageList plist;
    HyPackage pkg;
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_ENHANCES, HY_EQ, "flying");
    plist = hy_query_run(q);
    pkg = hy_packagelist_get(plist, 0);
    ck_assert_str_eq(hy_package_get_name(pkg), "walrus");
    hy_query_free(q);
    hy_packagelist_free(plist);
}
END_TEST

START_TEST(test_query_provides_in)
{
    HyPackage pkg;
    HyPackageList plist;
    char* pkg_names[] = { "P", "fool <= 2.0", "fool-lib > 3-3", NULL };
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter_provides_in(q, pkg_names);
    plist = hy_query_run(q);
    pkg = hy_packagelist_get(plist, 0);
    ck_assert_str_eq(hy_package_get_name(pkg), "fool");
    ck_assert_str_eq(hy_package_get_evr(pkg), "1-3");
    pkg = hy_packagelist_get(plist, 1);
    ck_assert_str_eq(hy_package_get_name(pkg), "penny");
    pkg = hy_packagelist_get(plist, 2);
    ck_assert_str_eq(hy_package_get_name(pkg), "fool");
    ck_assert_str_eq(hy_package_get_evr(pkg), "1-5");
    fail_unless(size_and_free(q) == 3);
    hy_packagelist_free(plist);
}
END_TEST

START_TEST(test_query_provides_in_not_found)
{
    HyPackageList plist;
    char* provides[] = { "thisisnotgoingtoexist", NULL };
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter_provides_in(q, provides);
    plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 0);
    hy_packagelist_free(plist);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_fileprovides)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_FILE, HY_EQ, "/no/answers");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_requires)
{
    HyQuery q;
    const char *repolist[] = {"main", NULL};

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_REPONAME, HY_EQ, repolist);
    hy_query_filter_requires(q, HY_EQ, "P-lib", "1");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_REPONAME, HY_EQ, repolist);
    hy_query_filter_requires(q, HY_GT, "P-lib", "5");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_REPONAME, HY_EQ, repolist);
    hy_query_filter_requires(q, HY_LT, "P-lib", "0.1");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_REPONAME, HY_EQ, repolist);
    hy_query_filter_requires(q, HY_EQ, "semolina", NULL);
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_REPONAME, HY_EQ, repolist);
    hy_query_filter_requires(q, HY_EQ|HY_LT, "semolina", "3.0");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_REPONAME, HY_EQ, repolist);
    hy_query_filter_requires(q, HY_GT, "semolina", "1.0");
    fail_unless(query_count_results(q) == 1);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_REPONAME, HY_EQ, repolist);
    hy_query_filter_requires(q, HY_NEQ, "semolina", NULL);
    fail_unless(query_count_results(q) == TEST_EXPECT_MAIN_NSOLVABLES-1);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_REPONAME, HY_EQ, repolist);
    hy_query_filter_requires(q, HY_NEQ, "semolina", "2");
    fail_unless(query_count_results(q) == TEST_EXPECT_MAIN_NSOLVABLES-1);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_REPONAME, HY_EQ, repolist);
    hy_query_filter_requires(q, HY_NEQ, "semolina", "2.1");
    fail_unless(query_count_results(q) == TEST_EXPECT_MAIN_NSOLVABLES);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_conflicts)
{
    HySack sack = test_globals.sack;
    HyQuery q = hy_query_create(sack);
    HyReldep reldep = hy_reldep_create(sack, "custard", HY_GT|HY_EQ, "1.0.1");

    fail_unless(reldep != NULL);
    hy_query_filter_reldep(q, HY_PKG_CONFLICTS, reldep);
    fail_unless(query_count_results(q) == 1);
    hy_reldep_free(reldep);
    hy_query_free(q);
}
END_TEST

START_TEST(test_upgrades_sanity)
{
    Pool *pool = sack_pool(test_globals.sack);
    Repo *r = NULL;
    int i;

    FOR_REPOS(i, r)
	if (!strcmp(r->name, "updates"))
	    break;
    fail_unless(r != NULL);
    fail_unless(r->nsolvables == TEST_EXPECT_UPDATES_NSOLVABLES);
}
END_TEST

START_TEST(test_upgrades)
{
    const char *installonly[] = {"fool", NULL};
    hy_sack_set_installonly(test_globals.sack, installonly);

    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter_upgrades(q, 1);
    fail_unless(query_count_results(q) == TEST_EXPECT_UPDATES_NSOLVABLES - 2);
    hy_query_free(q);
}
END_TEST

START_TEST(test_upgradable)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter_upgradable(q, 1);
    ck_assert_int_eq(query_count_results(q), 5);
    hy_query_free(q);
}
END_TEST

START_TEST(test_filter_latest)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "fool");
    hy_query_filter_latest_per_arch(q, 1);
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 1);
    HyPackage pkg = hy_packagelist_get(plist, 0);
    fail_if(strcmp(hy_package_get_name(pkg), "fool"));
    fail_if(strcmp(hy_package_get_evr(pkg), "1-5"));
    hy_query_free(q);
    hy_packagelist_free(plist);
}
END_TEST

START_TEST(test_filter_latest2)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "flying");
    hy_query_filter_latest_per_arch(q, 1);
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 2);
    HyPackage pkg = hy_packagelist_get(plist, 0);
    fail_if(strcmp(hy_package_get_name(pkg), "flying"));
    fail_if(strcmp(hy_package_get_evr(pkg), "3.1-0"));
    pkg = hy_packagelist_get(plist, 1);
    fail_if(strcmp(hy_package_get_name(pkg), "flying"));
    fail_if(strcmp(hy_package_get_evr(pkg), "3.2-0"));

    hy_query_free(q);
    hy_packagelist_free(plist);

}
END_TEST

START_TEST(test_filter_latest_archs)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "penny-lib");
    hy_query_filter_latest_per_arch(q, 1);
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 2); /* both architectures */
    hy_query_free(q);
    hy_packagelist_free(plist);
}
END_TEST

START_TEST(test_upgrade_already_installed)
{
    /* if pkg is installed in two versions and the later is available in repos,
       it shouldn't show as a possible update. */
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "jay");
    hy_query_filter_upgrades(q, 1);
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 0);
    hy_query_free(q);
    hy_packagelist_free(plist);
}
END_TEST

START_TEST(test_downgrade)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "jay");
    hy_query_filter_downgrades(q, 1);
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 1);
    HyPackage pkg = hy_packagelist_get(plist, 0);
    ck_assert_str_eq(hy_package_get_evr(pkg), "4.9-0");
    hy_query_free(q);
    hy_packagelist_free(plist);
}
END_TEST

START_TEST(test_downgradable)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter_downgradable(q, 1);
    ck_assert_int_eq(query_count_results(q), 2);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_provides_str)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_PROVIDES, HY_EQ, "pilchard");
    ck_assert_int_eq(query_count_results(q), 2);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_PROVIDES, HY_EQ, "fool = 1-2");
    ck_assert_int_eq(query_count_results(q), 2);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_PROVIDES, HY_EQ, "thisisnotgoingtoexist");
    ck_assert_int_eq(query_count_results(q), 0);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_provides_glob)
    {
        HyQuery q = hy_query_create(test_globals.sack);
        hy_query_filter(q, HY_PKG_PROVIDES, HY_GLOB, "penny*");
        ck_assert_int_eq(query_count_results(q), 6);
        hy_query_free(q);

        HyQuery q1 = hy_query_create(test_globals.sack);
        HyQuery q2 = hy_query_create(test_globals.sack);
        hy_query_filter(q1, HY_PKG_PROVIDES, HY_GLOB, "P-l*b >= 3");
        hy_query_filter(q2, HY_PKG_PROVIDES, HY_EQ, "P-lib >= 3");
        ck_assert_int_eq(query_count_results(q1), query_count_results(q2));
        hy_query_free(q1);
        hy_query_free(q2);

        q1 = hy_query_create(test_globals.sack);
        q2 = hy_query_create(test_globals.sack);
        hy_query_filter(q1, HY_PKG_PROVIDES, HY_GLOB, "*");
        fail_unless(query_count_results(q1) == query_count_results(q2));
        hy_query_free(q1);
        hy_query_free(q2);
    }
END_TEST

START_TEST(test_query_rco_glob)
    {
        HyQuery q = hy_query_create(test_globals.sack);
        hy_query_filter(q, HY_PKG_REQUIRES, HY_GLOB, "*");
        ck_assert_int_eq(query_count_results(q), 5);
        hy_query_free(q);

        q = hy_query_create(test_globals.sack);
        hy_query_filter(q, HY_PKG_REQUIRES, HY_GLOB, "*oo*");
        ck_assert_int_eq(query_count_results(q), 2);
        hy_query_free(q);

        q = hy_query_create(test_globals.sack);
        hy_query_filter(q, HY_PKG_CONFLICTS, HY_GLOB, "*");
        ck_assert_int_eq(query_count_results(q), 1);
        hy_query_free(q);

        q = hy_query_create(test_globals.sack);
        hy_query_filter(q, HY_PKG_OBSOLETES, HY_GLOB, "*");
        ck_assert_int_eq(query_count_results(q), 0);
        hy_query_free(q);

        q = hy_query_create(test_globals.sack);
        hy_query_filter(q, HY_PKG_ENHANCES, HY_GLOB, "*");
        ck_assert_int_eq(query_count_results(q), 1);
        hy_query_free(q);

        q = hy_query_create(test_globals.sack);
        hy_query_filter(q, HY_PKG_RECOMMENDS, HY_GLOB, "*");
        ck_assert_int_eq(query_count_results(q), 1);
        hy_query_free(q);

        q = hy_query_create(test_globals.sack);
        hy_query_filter(q, HY_PKG_SUGGESTS, HY_GLOB, "*");
        ck_assert_int_eq(query_count_results(q), 1);
        hy_query_free(q);

        q = hy_query_create(test_globals.sack);
        hy_query_filter(q, HY_PKG_SUPPLEMENTS, HY_GLOB, "*");
        ck_assert_int_eq(query_count_results(q), 1);
        hy_query_free(q);
    }
END_TEST

START_TEST(test_query_reldep)
{
    HySack sack = test_globals.sack;
    HyPackage flying = by_name(sack, "flying");

    HyReldepList reldeplist = hy_package_get_requires(flying);
    HyReldep reldep = hy_reldeplist_get_clone(reldeplist, 0);

    HyQuery q = hy_query_create(test_globals.sack);
    fail_if(hy_query_filter_reldep(q, HY_PKG_PROVIDES, reldep));
    fail_unless(query_count_results(q) == 3);

    hy_reldep_free(reldep);
    hy_reldeplist_free(reldeplist);
    hy_package_free(flying);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_reldep_arbitrary)
{
    HySack sack = test_globals.sack;
    HyQuery query = hy_query_create(sack);
    HyReldep reldep = hy_reldep_create(sack, "P-lib", HY_GT, "3-0");

    fail_if(reldep == NULL);
    hy_query_filter_reldep(query, HY_PKG_PROVIDES, reldep);
    fail_unless(query_count_results(query) == 3);

    hy_reldep_free(reldep);
    hy_query_free(query);
}
END_TEST

START_TEST(test_filter_files)
{
    HyQuery q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_FILE, HY_EQ, "/etc/takeyouaway");
    HyPackageList plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 1);
    HyPackage pkg = hy_packagelist_get(plist, 0);
    fail_if(strcmp(hy_package_get_name(pkg), "tour"));
    hy_packagelist_free(plist);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_FILE, HY_GLOB, "/usr/*");
    plist = hy_query_run(q);
    fail_unless(hy_packagelist_count(plist) == 2);
    hy_packagelist_free(plist);
    hy_query_free(q);
}
END_TEST

START_TEST(test_filter_sourcerpm)
{
    HyQuery q = hy_query_create(test_globals.sack);
    fail_unless(hy_query_filter(q, HY_PKG_SOURCERPM, HY_GT, "tour-4-6.src.rpm"));
    fail_if(hy_query_filter(q, HY_PKG_SOURCERPM, HY_EQ, "tour-4-6.src.rpm"));
    fail_unless(size_and_free(q) == 1);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_SOURCERPM, HY_EQ, "mystery-19.67-1.src.rpm");
    fail_unless(size_and_free(q) == 1);

    q = hy_query_create(test_globals.sack);
    hy_query_filter(q, HY_PKG_SOURCERPM, HY_EQ,
		    "mystery-devel-19.67-1.noarch.rpm");
    fail_unless(size_and_free(q) == 0);
}
END_TEST

START_TEST(test_filter_description)
{
    HyQuery q = hy_query_create(test_globals.sack);
    fail_if(hy_query_filter(q, HY_PKG_DESCRIPTION, HY_SUBSTR,
			    "Magical development files for mystery."));
    fail_unless(size_and_free(q) == 1);
}
END_TEST

START_TEST(test_filter_obsoletes)
{
    HySack sack = test_globals.sack;
    HyQuery q = hy_query_create(sack);
    HyPackageSet pset = hy_packageset_create(sack); // empty

    fail_if(hy_query_filter_package_in(q, HY_PKG_OBSOLETES, HY_EQ, pset));
    fail_unless(query_count_results(q) == 0);
    hy_query_clear(q);

    hy_packageset_add(pset, by_name(sack, "penny"));
    hy_query_filter_package_in(q, HY_PKG_OBSOLETES, HY_EQ, pset);
    fail_unless(query_count_results(q) == 1);

    hy_query_free(q);
    hy_packageset_free(pset);
}
END_TEST

START_TEST(test_filter_reponames)
{
    HyQuery q;
    const char *repolist[]  = {"main", "updates", NULL};
    const char *repolist2[] = {"main",  NULL};
    const char *repolist3[] = {"foo", "bar",  NULL};

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_REPONAME, HY_EQ, repolist);
    fail_unless(query_count_results(q) == TEST_EXPECT_MAIN_NSOLVABLES \
					+ TEST_EXPECT_UPDATES_NSOLVABLES);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_REPONAME, HY_EQ, repolist2);
    fail_unless(query_count_results(q) == TEST_EXPECT_MAIN_NSOLVABLES);
    hy_query_free(q);

    q = hy_query_create(test_globals.sack);
    hy_query_filter_in(q, HY_PKG_REPONAME, HY_EQ, repolist3);
    fail_unless(query_count_results(q) == 0);
    hy_query_free(q);
}
END_TEST

START_TEST(test_excluded)
{
    HySack sack = test_globals.sack;
    HyQuery q = hy_query_create_flags(sack, HY_IGNORE_EXCLUDES);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "jay");

    HyPackageSet pset = hy_query_run_set(q);
    hy_sack_add_excludes(sack, pset);
    hy_packageset_free(pset);
    hy_query_free(q);

    q = hy_query_create_flags(sack, 0);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "jay");
    fail_unless(query_count_results(q) == 0);
    hy_query_free(q);

    q = hy_query_create_flags(sack, HY_IGNORE_EXCLUDES);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "jay");
    fail_unless(query_count_results(q) > 0);
    hy_query_free(q);
}
END_TEST

START_TEST(test_disabled_repo)
{
    HySack sack = test_globals.sack;
    HyQuery q;

    q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_EVR, HY_EQ, "4.9-0");
    fail_unless(size_and_free(q) == 1);
    q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "jay");
    ck_assert_int_eq(size_and_free(q), 5);

    hy_sack_repo_enabled(sack, "main", 0);

    q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_EVR, HY_EQ, "4.9-0");
    fail_unless(size_and_free(q) == 0);
    q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, "jay");
    ck_assert_int_eq(size_and_free(q), 2);
}
END_TEST

START_TEST(test_query_nevra_glob)
{
    HySack sack = test_globals.sack;
    HyQuery q;
    HyPackageList plist;

    q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NEVRA, HY_GLOB, "p*4-1*");
    plist = hy_query_run(q);

    ck_assert_int_eq(hy_packagelist_count(plist), 2);
    HyPackage pkg1 = hy_packagelist_get(plist, 0);
    HyPackage pkg2 = hy_packagelist_get(plist, 1);
    char *nevra1 = hy_package_get_nevra(pkg1);
    char *nevra2 = hy_package_get_nevra(pkg2);
    ck_assert_str_eq(nevra1, "penny-4-1.noarch");
    ck_assert_str_eq(nevra2, "penny-lib-4-1.x86_64");
    solv_free(nevra1);
    solv_free(nevra2);
    hy_packagelist_free(plist);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_nevra)
{
    HySack sack = test_globals.sack;
    HyQuery q;
    HyPackageList plist;

    q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NEVRA, HY_EQ, "penny-4-1.noarch");
    plist = hy_query_run(q);

    ck_assert_int_eq(hy_packagelist_count(plist), 1);
    HyPackage pkg1 = hy_packagelist_get(plist, 0);
    char *nevra1 = hy_package_get_nevra(pkg1);
    ck_assert_str_eq(nevra1, "penny-4-1.noarch");
    solv_free(nevra1);
    hy_packagelist_free(plist);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_multiple_flags)
{
    HySack sack = test_globals.sack;
    HyQuery q;
    HyPackageList plist;

    q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_NOT | HY_GLOB, "p*");
    plist = hy_query_run(q);

    ck_assert_int_eq(hy_packagelist_count(plist), 8);
    hy_packagelist_free(plist);
    hy_query_free(q);
}
END_TEST

START_TEST(test_query_apply)
{
    HySack sack = test_globals.sack;
    HyQuery q;
    HyPackageList plist;

    q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_NOT | HY_GLOB, "j*");
    struct _HyQuery _q = *q;
    fail_unless(_q.result == NULL);
    ck_assert_int_gt(_q.nfilters, 0);
    ck_assert_int_eq(_q.applied, 0);
    hy_query_apply(q);
    _q = *q;
    fail_unless(_q.result != NULL);
    ck_assert_int_eq(_q.nfilters, 0);
    ck_assert_int_eq(_q.applied, 1);
    hy_query_filter(q, HY_PKG_NAME, HY_NOT | HY_GLOB, "p*");
    _q = *q;
    fail_unless(_q.result != NULL);
    ck_assert_int_gt(_q.nfilters, 0);
    ck_assert_int_eq(_q.applied, 0);
    plist = hy_query_run(q);

    ck_assert_int_eq(hy_packagelist_count(plist), 6);
    hy_packagelist_free(plist);
    hy_query_free(q);
}
END_TEST

Suite *
query_suite(void)
{
    Suite *s = suite_create("Query");
    TCase *tc;

    tc = tcase_create("Core");
    tcase_add_unchecked_fixture(tc, fixture_system_only, teardown);
    tcase_add_test(tc, test_query_sanity);
    tcase_add_test(tc, test_query_run_set_sanity);
    tcase_add_test(tc, test_query_clear);
    tcase_add_test(tc, test_query_clone);
    tcase_add_test(tc, test_query_empty);
    tcase_add_test(tc, test_query_repo);
    tcase_add_test(tc, test_query_name);
    tcase_add_test(tc, test_query_evr);
    tcase_add_test(tc, test_query_epoch);
    tcase_add_test(tc, test_query_version);
    tcase_add_test(tc, test_query_release);
    tcase_add_test(tc, test_query_glob);
    tcase_add_test(tc, test_query_case);
    tcase_add_test(tc, test_query_anded);
    tcase_add_test(tc, test_query_neq);
    tcase_add_test(tc, test_query_in);
    tcase_add_test(tc, test_query_pkg);
    tcase_add_test(tc, test_query_provides);
    tcase_add_test(tc, test_query_fileprovides);
    tcase_add_test(tc, test_query_nevra);
    tcase_add_test(tc, test_query_nevra_glob);
    tcase_add_test(tc, test_query_multiple_flags);
    tcase_add_test(tc, test_query_apply);
    suite_add_tcase(s, tc);

    tc = tcase_create("Updates");
    tcase_add_unchecked_fixture(tc, fixture_with_updates, teardown);
    tcase_add_test(tc, test_upgrades_sanity);
    tcase_add_test(tc, test_upgrades);
    tcase_add_test(tc, test_upgradable);
    tcase_add_test(tc, test_filter_latest);
    tcase_add_test(tc, test_query_provides_in);
    tcase_add_test(tc, test_query_provides_in_not_found);
    suite_add_tcase(s, tc);

    tc = tcase_create("Main");
    tcase_add_unchecked_fixture(tc, fixture_with_main, teardown);
    tcase_add_test(tc, test_upgrade_already_installed);
    tcase_add_test(tc, test_downgrade);
    tcase_add_test(tc, test_downgradable);
    tcase_add_test(tc, test_query_provides_str);
    tcase_add_test(tc, test_query_provides_glob);
    tcase_add_test(tc, test_query_rco_glob);
    tcase_add_test(tc, test_query_recommends);
    tcase_add_test(tc, test_query_suggests);
    tcase_add_test(tc, test_query_supplements);
    tcase_add_test(tc, test_query_enhances);
    tcase_add_test(tc, test_query_reldep);
    tcase_add_test(tc, test_query_reldep_arbitrary);
    tcase_add_test(tc, test_query_requires);
    tcase_add_test(tc, test_query_conflicts);
    suite_add_tcase(s, tc);

    tc = tcase_create("Full");
    tcase_add_unchecked_fixture(tc, fixture_all, teardown);
    tcase_add_test(tc, test_filter_latest2);
    tcase_add_test(tc, test_filter_latest_archs);
    tcase_add_test(tc, test_filter_obsoletes);
    tcase_add_test(tc, test_filter_reponames);
    suite_add_tcase(s, tc);

    tc = tcase_create("Filelists etc.");
    tcase_add_unchecked_fixture(tc, fixture_yum, teardown);
    tcase_add_test(tc, test_filter_files);
    tcase_add_test(tc, test_filter_sourcerpm);
    tcase_add_test(tc, test_filter_description);
    tcase_add_test(tc, test_query_location);
    suite_add_tcase(s, tc);

    tc = tcase_create("Excluding");
    tcase_add_unchecked_fixture(tc, fixture_with_main, teardown);
    tcase_add_checked_fixture(tc, fixture_reset, NULL);
    tcase_add_test(tc, test_excluded);
    tcase_add_test(tc, test_disabled_repo);
    suite_add_tcase(s, tc);

    return s;
}
