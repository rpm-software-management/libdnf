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

#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <wordexp.h>
#include <solv/repo_rpmdb.h>
#include <solv/solver.h>

#include "hy-goal.h"
#include "hy-package-private.h"
#include "hy-package.h"
#include "hy-packageset.h"
#include "hy-query.h"
#include "dnf-reldep-list.h"
#include "hy-repo.h"
#include "dnf-sack.h"
#include "hy-util.h"

#define CFG_FILE "~/.hawkey/main.config"

static const char *installonly[] = {
    "kernel",
    "kernel-bigmem",
    "kernel-enterprise",
    "kernel-smp",
    "kernel-modules",
    "kernel-debug",
    "kernel-unsupported",
    "kernel-source",
    "kernel-devel",
    "kernel-PAE",
    "kernel-PAE-debug",
    NULL
};

static void execute_print(DnfSack *sack, HyQuery q, int show_obsoletes)
{
    GPtrArray *plist;

    plist = hy_query_run(q);
    const int count = plist->len;
    for (int i = 0; i < count; ++i) {
        DnfPackage *pkg = g_ptr_array_index(plist, i);
        const char *nvra = dnf_package_get_nevra(pkg);
        const char *reponame = dnf_package_get_reponame(pkg);

        printf("found package: %s [%s]\n", nvra, reponame);
        if (strcmp(reponame, HY_SYSTEM_REPO_NAME) == 0) {
            int type;
            const unsigned char * hdrid = dnf_package_get_hdr_chksum(pkg, &type);
            char *str = hy_chksum_str(hdrid, type);

            printf("\tsha1 header id: %s\n", str);
            g_free(str);
        }
        if (show_obsoletes) {
            DnfReldepList *obsoletes = dnf_package_get_obsoletes(pkg);
            HyQuery qobs = hy_query_create(sack);

            hy_query_filter(qobs, HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
            hy_query_filter_reldep_in(qobs, HY_PKG_PROVIDES, obsoletes);
            GPtrArray *olist = hy_query_run(qobs);
            const int ocount = olist->len;
            for (int j = 0; j < ocount; ++j) {
                DnfPackage *opkg = g_ptr_array_index(olist, j);
                const char *onvra = dnf_package_get_nevra(opkg);
                printf("obsoleting: %s\n", onvra);
            }
            g_ptr_array_unref(olist);
            hy_query_free(qobs);
            g_object_unref(obsoletes);
        }
    }
    g_ptr_array_unref(plist);
}

static void search_and_print(DnfSack *sack, const char *name)
{
    HyQuery q = hy_query_create(sack);

    printf("found packages by name:\n");
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    execute_print(sack, q, 0);
    hy_query_clear(q);

    printf("\nfound packages by substring of summary:\n");
    hy_query_filter(q, HY_PKG_SUMMARY, HY_SUBSTR, name);
    execute_print(sack, q, 0);
    hy_query_free(q);
}

static void
search_filter_files(DnfSack *sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_FILE, HY_EQ, name);
    execute_print(sack, q, 0);
    hy_query_free(q);
}

static void search_filter_repos(DnfSack *sack, const char *name) {
    HyQuery q = hy_query_create(sack);

    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
    execute_print(sack, q, 0);
    hy_query_free(q);
}

static void search_anded(DnfSack *sack, const char *name_substr,
                         const char *summary_substr)
{
    HyQuery q = hy_query_create(sack);

    hy_query_filter(q, HY_PKG_NAME, HY_SUBSTR, name_substr);
    hy_query_filter(q, HY_PKG_SUMMARY, HY_SUBSTR, summary_substr);
    execute_print(sack, q, 0);
    hy_query_free(q);
}

static void search_provides(DnfSack *sack, const char *name,
                            const char *version)
{
    HyQuery q = hy_query_create(sack);

    hy_query_filter_provides(q, HY_GT, name, version);
    execute_print(sack, q, 0);
    hy_query_free(q);
}

static void updatables_query_name(DnfSack *sack, const char *name)
{
    HyQuery q = hy_query_create(sack);

    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
#if 0 // must stil work if enabled
    hy_query_filter(q, HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
#endif
    hy_query_filter_upgrades(q, 1);
    execute_print(sack, q, 0);
    hy_query_free(q);
}

static void obsoletes(DnfSack *sack)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
    DnfPackageSet *installed_pkgs = hy_query_run_set(q);
    hy_query_free(q);

    q = hy_query_create(sack);
    hy_query_filter_package_in(q, HY_PKG_OBSOLETES, HY_EQ, installed_pkgs);
    g_object_unref(installed_pkgs);
    execute_print(sack, q, 1);
    hy_query_free(q);
}

static void
dump_goal_errors(HyGoal goal)
{
    for (int i = 0; i < hy_goal_count_problems(goal); ++i) {
        char *problem = hy_goal_describe_problem(goal, i);
        printf("%s\n", problem);
        g_free(problem);
    }
}

static void
erase(DnfSack *sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);

    GPtrArray *plist = hy_query_run(q);
    if (plist->len < 1) {
        printf("No installed package to erase found.\n");
        goto finish;
    }
    if (plist->len > 1)
        printf("(more than one updatables found, selecting the first one)\n");

    HyGoal goal = hy_goal_create(sack);
    hy_goal_erase(goal, g_ptr_array_index(plist, 0));

    if (hy_goal_run(goal)) {
        dump_goal_errors(goal);
        hy_goal_free(goal);
        goto finish;
    }
    g_ptr_array_unref(plist);
    plist = hy_goal_list_erasures(goal, NULL);
    printf("erasure count: %d\n", plist->len);
    for (unsigned int i = 0; i < plist->len; ++i) {
        DnfPackage *pkg = g_ptr_array_index(plist, i);
        const char *nvra = dnf_package_get_nevra(pkg);

        printf("erasing %s\n", nvra);
    }

    hy_goal_free(goal);
 finish:
    g_ptr_array_unref(plist);
    hy_query_free(q);
}

static void update(DnfSack *sack, DnfPackage *pkg)
{
    HyGoal goal = hy_goal_create(sack);

    if (hy_goal_upgrade_to_flags(goal, pkg, HY_CHECK_INSTALLED)) {
        printf("no package of that name installed, trying install\n");
        hy_goal_install(goal, pkg);
    }
    if (hy_goal_run(goal)) {
        dump_goal_errors(goal);
        // handle errors
        goto finish;
    }
    // handle upgrades
    GPtrArray *plist = hy_goal_list_upgrades(goal, NULL);
    printf("upgrade count: %d\n", plist->len);
    for (unsigned int i = 0; i < plist->len; ++i) {
        DnfPackage *upkg = g_ptr_array_index(plist, i);
        const char *nvra = dnf_package_get_nevra(upkg);
        char *location = dnf_package_get_location(upkg);
        GPtrArray *obsoleted = hy_goal_list_obsoleted_by_package(goal, upkg);
        DnfPackage *installed = g_ptr_array_index(obsoleted, 0);
        const char *nvra_installed = dnf_package_get_nevra(installed);

        printf("upgrading: %s using %s\n", nvra, location);
        printf("\tfrom: %s\n", nvra_installed);
        printf("\tsize: %lu kB\n", dnf_package_get_size(upkg) / 1024);

        g_ptr_array_unref(obsoleted);
        g_free(location);
    }
    g_ptr_array_unref(plist);
    // handle installs
    plist = hy_goal_list_installs(goal, NULL);
    printf("install count: %d\n", plist->len);
    for (unsigned int i = 0; i < plist->len; ++i) {
        DnfPackage *ipkg = g_ptr_array_index(plist, i);
        const char *nvra = dnf_package_get_nevra(ipkg);
        char *location = dnf_package_get_location(ipkg);

        printf("installing: %s using %s\n", nvra, location);
        printf("\tsize: %lu kB\n", dnf_package_get_size(ipkg) / 1024);

        g_free(location);
    }
    g_ptr_array_unref(plist);

 finish:
    hy_goal_free(goal);
}

static void update_local(DnfSack *sack, const char *fn)
{
    DnfPackage *pkg;

    pkg = dnf_sack_add_cmdline_package(sack, fn);
    if (pkg) {
        update(sack, pkg);
        g_object_unref(pkg);
    }
}

static void update_remote(DnfSack *sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    GPtrArray *plist;

    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
    hy_query_filter_latest_per_arch(q, 1);

    plist = hy_query_run(q);
    if (plist->len == 0) {
        printf("no updatables found\n");
        goto finish;
    }
    if (plist->len > 1)
        printf("(more than one updatables found, selecting the first one)\n");
    update(sack, g_ptr_array_index(plist, 0));
finish:
    g_ptr_array_unref(plist);
    hy_query_free(q);
}

static HyRepo
config_repo(const char *name, const char *md_repo,
            const char *md_primary_xml, const char *md_filelists)
{
    HyRepo repo = hy_repo_create(name);
    hy_repo_set_string(repo, HY_REPO_MD_FN, md_repo);
    hy_repo_set_string(repo, HY_REPO_PRIMARY_FN, md_primary_xml);
    hy_repo_set_string(repo, HY_REPO_FILELISTS_FN, md_filelists);
    return repo;
}

static void
rtrim(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len - 1] == '\n')
        str[len - 1 ] = '\0';
}

static int
read_repopaths(char **md_repo, char **md_primary_xml, char **md_filelists,
               char **md_repo_updates, char **md_primary_updates_xml,
               char **md_filelists_updates)
{
    wordexp_t word_vector;

    *md_repo = *md_primary_xml = *md_filelists = *md_repo_updates = \
        *md_primary_updates_xml = *md_filelists_updates = NULL;

    if (wordexp(CFG_FILE, &word_vector, 0))
        return 1;
    if (word_vector.we_wordc < 1) {
        wordfree(&word_vector);
        return 1;
    }
    const char *cfgpath = word_vector.we_wordv[0];
    FILE *f = fopen(cfgpath, "r");
    if (!f) {
        wordfree(&word_vector);
        return 1;
    }
    size_t size = 0;
    if ((getline(md_repo, &size, f) < 0) ||
        (getline(md_primary_xml, &size, f) < 0) ||
        (getline(md_filelists, &size, f) < 0) ||
        (getline(md_repo_updates, &size, f) < 0) ||
        (getline(md_primary_updates_xml, &size, f) < 0) ||
        (getline(md_filelists_updates, &size, f) < 0)) {
        free(*md_repo);
        free(*md_primary_xml);
        free(*md_filelists);
        free(*md_repo_updates);
        free(*md_primary_updates_xml);
        free(*md_filelists_updates);
        fclose(f);
        wordfree(&word_vector);
        return 1;
    }
    fclose(f);
    wordfree(&word_vector);
    rtrim(*md_repo);
    rtrim(*md_primary_xml);
    rtrim(*md_filelists);
    rtrim(*md_repo_updates);
    rtrim(*md_primary_updates_xml);
    rtrim(*md_filelists_updates);
    return 0;
}

static int
need_filelists(int argc, const char **argv)
{
    return argc == 3 && !strcmp(argv[1], "-f");
}

int main(int argc, const char **argv)
{
    DnfSack *sack = dnf_sack_new ();
    HyRepo repo;
    char *md_repo;
    char *md_primary_xml;
    char *md_filelists;
    char *md_repo_updates;
    char *md_primary_updates_xml;
    char *md_filelists_updates;
    int ret;
    g_autoptr(GError) error = NULL;

    if (!dnf_sack_setup(sack, DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR, NULL))
        return 1;

    if (read_repopaths(&md_repo, &md_primary_xml, &md_filelists, &md_repo_updates,
                       &md_primary_updates_xml, &md_filelists_updates)) {
        fprintf(stderr,
                "This is hawkey testing hack, it needs a readable %s file "
                "containing the following paths on separate lines:\n"
                "<main repomd.xml path>\n"
                "<main primary.xml.gz path>\n"
                "<main filelist.xml.gz path>\n"
                "<updates repomd.xml path>\n"
                "<updates primary.xml.gz path>\n"
                "<updates filelists.xml.gz path>\n", CFG_FILE);
        return 1;
    }
    int load_flags = DNF_SACK_LOAD_FLAG_BUILD_CACHE;
    /* rpmdb */
    repo = hy_repo_create(HY_SYSTEM_REPO_NAME);
    dnf_sack_load_system_repo(sack, NULL, load_flags, &error);
    hy_repo_free(repo);

    if (need_filelists(argc, argv))
        load_flags |= DNF_SACK_LOAD_FLAG_USE_FILELISTS;
    /* Fedora repo */
    repo = config_repo("Fedora", md_repo, md_primary_xml, md_filelists);
    ret = dnf_sack_load_repo(sack, repo, load_flags, &error);
    assert(ret == 0); (void)ret;
    hy_repo_free(repo);
    /* Fedora updates repo */
    repo = config_repo("updates", md_repo_updates, md_primary_updates_xml,
                       md_filelists_updates);
    ret = dnf_sack_load_repo(sack, repo, load_flags, &error);
    assert(ret == 0); (void)ret;
    hy_repo_free(repo);
    free(md_repo);
    free(md_primary_xml);
    free(md_filelists);
    free(md_repo_updates);
    free(md_primary_updates_xml);
    free(md_filelists_updates);

    dnf_sack_set_installonly(sack, installonly);
    dnf_sack_set_installonly_limit(sack, 3);

    if (argc == 2 && !strcmp(argv[1], "-o")) {
        obsoletes(sack);
    } else if (argc == 2) {
        search_and_print(sack, argv[1]);
    } else if (argc == 3 && !strcmp(argv[1], "-f")) {
        search_filter_files(sack, argv[2]);
    } else if (argc == 3 && !strcmp(argv[1], "-r")) {
        search_filter_repos(sack, argv[2]);
    } else if (argc == 3 && !strcmp(argv[1], "-u")) {
        updatables_query_name(sack, argv[2]);
    } else if (argc == 3 && !strcmp(argv[1], "-ul")) {
        update_local(sack, argv[2]);
    } else if (argc == 3 && !strcmp(argv[1], "-ur")) {
        update_remote(sack, argv[2]);
    } else if (argc == 3 && !strcmp(argv[1], "-e")) {
        erase(sack, argv[2]);
    } else if (argc == 3) {
        search_anded(sack, argv[1], argv[2]);
    } else if (argc == 4 && !strcmp(argv[1], "-p")) {
        search_provides(sack, argv[2], argv[3]);
    }

    g_object_unref(sack);

    return 0;
}
