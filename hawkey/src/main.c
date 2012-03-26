#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <wordexp.h>

// libsolv
#include "solv/repo_rpmdb.h"
#include "solv/solver.h"
#include "solv/util.h"

// hawkey
#include "repo.h"
#include "goal.h"
#include "package_internal.h"
#include "query.h"
#include "sack.h"

#define CFG_FILE "~/.hawkey/main.config"

static void execute_print(HySack sack, HyQuery q, int show_obsoletes)
{
    HyPackageList plist;
    HyPackageListIter iter;
    HyPackage pkg;

    plist = hy_query_run(q);
    iter = hy_packagelist_iter_create(plist);
    while ((pkg = hy_packagelist_iter_next(iter)) != NULL) {
	char *nvra = hy_package_get_nvra(pkg);
	printf("found package: %s [%s]\n",
	       nvra,
	       hy_package_get_reponame(pkg));
	if (show_obsoletes) {
	    HyPackageList olist = hy_packagelist_of_obsoletes(sack, pkg);
	    HyPackageListIter oiter = hy_packagelist_iter_create(olist);
	    HyPackage opkg;
	    char *onvra;
	    while ((opkg = hy_packagelist_iter_next(oiter)) != NULL) {
		onvra = hy_package_get_nvra(opkg);
		printf("obsoleting: %s\n", onvra);
		solv_free(onvra);
	    }
	    hy_packagelist_iter_free(oiter);
	    hy_packagelist_free(olist);
	}
	solv_free(nvra);
    }
    hy_packagelist_iter_free(iter);
    hy_packagelist_free(plist);
}

static void execute_queue(HyQuery q, Queue *job)
{
    HyPackageList plist;
    HyPackageListIter iter;
    HyPackage pkg;

    plist = hy_query_run(q);
    iter = hy_packagelist_iter_create(plist);
    while ((pkg = hy_packagelist_iter_next(iter)) != NULL) {
	queue_push2(job, SOLVER_SOLVABLE|SOLVER_INSTALL, package_id(pkg));
    }
    hy_packagelist_iter_free(iter);
    hy_packagelist_free(plist);
}

static void search_and_print(HySack sack, const char *name)
{
    HyPackage pkg;
    HyPackageList plist;
    HyPackageListIter iter;

    plist = sack_f_by_name(sack, name);
    iter = hy_packagelist_iter_create(plist);
    while ((pkg = hy_packagelist_iter_next(iter)) != NULL)
       	printf("found package by name: (%s, %s, %s)\n", hy_package_get_name(pkg),
	       hy_package_get_evr(pkg),
	       hy_package_get_arch(pkg));
    hy_packagelist_iter_free(iter);
    hy_packagelist_free(plist);

    plist = sack_f_by_summary(sack, name);
    iter = hy_packagelist_iter_create(plist);
    while ((pkg = hy_packagelist_iter_next(iter)) != NULL) {
	char *nvra = hy_package_get_nvra(pkg);
       	printf("found package by summary: %s\n", nvra);
	solv_free(nvra);
    }
    hy_packagelist_iter_free(iter);
    hy_packagelist_free(plist);
}

static void search_filter_repos(HySack sack, const char *name) {
    HyQuery q = hy_query_create(sack);

    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPO, HY_NEQ, SYSTEM_REPO_NAME);
    execute_print(sack, q, 0);
    hy_query_free(q);
}

static void search_anded(HySack sack, const char *name_substr,
			 const char *summary_substr)
{
    HyQuery q = hy_query_create(sack);

    hy_query_filter(q, HY_PKG_NAME, HY_SUBSTR, name_substr);
    hy_query_filter(q, HY_PKG_SUMMARY, HY_SUBSTR, summary_substr);
    execute_print(sack, q, 0);
    hy_query_free(q);
}

static void search_provides(HySack sack, const char *name,
			    const char *version)
{
    HyQuery q = hy_query_create(sack);

    hy_query_filter_provides(q, HY_GT, name, version);
    execute_print(sack, q, 0);
    hy_query_free(q);
}

static void resolve_install(HySack sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    Queue job;

    queue_init(&job);

    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    execute_queue(q, &job);
    assert(job.count == 2); // XXX: the search mechanism for this case is broken.
    hy_sack_solve(sack, &job, NULL, 0);
    queue_free(&job);
    hy_query_free(q);
}

static void updatables_query_all(HySack sack)
{
    Queue job;
    queue_init(&job);
    queue_push2(&job, SOLVER_UPDATE|SOLVER_SOLVABLE_ALL, 0);
    hy_sack_solve(sack, &job, NULL, 0);
    queue_free(&job);
}

static void updatables_query_name(HySack sack, const char *name)
{
    HyQuery q = hy_query_create(sack);

    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
#if 0 // must stil work if enabled
    hy_query_filter(q, HY_PKG_REPO, HY_NEQ, SYSTEM_REPO_NAME);
#endif
    hy_query_filter_updates(q, 1);
    execute_print(sack, q, 0);
    hy_query_free(q);
}

static void obsoletes(HySack sack)
{
    HyQuery q = hy_query_create(sack);

    hy_query_filter_obsoleting(q, 1);
    execute_print(sack, q, 1);
    hy_query_free(q);
}

static void
dump_goal_errors(HyGoal goal)
{
    for (int i = 0; i < hy_goal_count_problems(goal); ++i) {
	char *problem = hy_goal_describe_problem(goal, i);
	printf("%s\n", problem);
	solv_free(problem);
    }
}

static void
erase(HySack sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPO, HY_EQ, SYSTEM_REPO_NAME);

    HyPackageList plist = hy_query_run(q);
    if (hy_packagelist_count(plist) < 1) {
	printf("No installed package to erase found.\n");
	goto finish;
    }
    if (hy_packagelist_count(plist) > 1)
	printf("(more than one updatables found, selecting the first one)\n");

    HyGoal goal = hy_goal_create(sack);
    hy_goal_erase(goal, hy_packagelist_get(plist, 0));

    if (hy_goal_go(goal)) {
	dump_goal_errors(goal);
	hy_goal_free(goal);
	goto finish;
    }
    hy_packagelist_free(plist);
    plist = hy_goal_list_erasures(goal);
    printf("erasure count: %d\n", hy_packagelist_count(plist));
    for (int i = 0; i < hy_packagelist_count(plist); ++i) {
	HyPackage pkg = hy_packagelist_get(plist, i);
	char *nvra = hy_package_get_nvra(pkg);

	printf("erasing %s\n", nvra);
	solv_free(nvra);
    }

    hy_goal_free(goal);
 finish:
    hy_packagelist_free(plist);
    hy_query_free(q);
}

static void update(HySack sack, HyPackage pkg)
{
    HyGoal goal = hy_goal_create(sack);

    if (hy_goal_update(goal, pkg)) {
	printf("no package of that name installed, trying install\n");
	hy_goal_install(goal, pkg);
    }
    if (hy_goal_go(goal)) {
	dump_goal_errors(goal);
	// handle errors
	goto finish;
    }
    // handle upgrades
    HyPackageList plist = hy_goal_list_upgrades(goal);
    printf("upgrade count: %d\n", hy_packagelist_count(plist));
    for (int i = 0; i < hy_packagelist_count(plist); ++i) {
	HyPackage pkg = hy_packagelist_get(plist, i);
	char *nvra = hy_package_get_nvra(pkg);
	char *location = hy_package_get_location(pkg);
	HyPackage installed = hy_goal_package_upgrades(goal, pkg);
	char *nvra_installed = hy_package_get_nvra(installed);

	printf("upgrading: %s using %s\n", nvra, location);
	printf("\tfrom: %s\n", nvra_installed);
	printf("\tsize: %d kB\n", hy_package_get_size(pkg) / 1024);

	solv_free(nvra_installed);
	hy_package_free(installed);
	solv_free(location);
	solv_free(nvra);
    }
    hy_packagelist_free(plist);
    // handle installs
    plist = hy_goal_list_installs(goal);
    printf("install count: %d\n", hy_packagelist_count(plist));
    for (int i = 0; i < hy_packagelist_count(plist); ++i) {
	HyPackage pkg = hy_packagelist_get(plist, i);
	char *nvra = hy_package_get_nvra(pkg);
	char *location = hy_package_get_location(pkg);

	printf("installing: %s using %s\n", nvra, location);
	printf("\tsize: %d kB\n", hy_package_get_size(pkg) / 1024);

	solv_free(location);
	solv_free(nvra);
    }
    hy_packagelist_free(plist);

 finish:
    hy_goal_free(goal);
}

static void update_local(HySack sack, const char *fn)
{
    HyPackage pkg;

    hy_sack_create_cmdline_repo(sack);
    pkg = hy_sack_add_cmdline_rpm(sack, fn);
    if (pkg) {
	update(sack, pkg);
	hy_package_free(pkg);
    }
}

static void update_remote(HySack sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    HyPackageList plist;

    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPO, HY_NEQ, SYSTEM_REPO_NAME);
    hy_query_filter_latest(q, 1);

    plist = hy_query_run(q);
    if (hy_packagelist_count(plist) == 0) {
	printf("no updatables found\n");
	goto finish;
    }
    if (hy_packagelist_count(plist) > 1)
	printf("(more than one updatables found, selecting the first one)\n");
    update(sack, hy_packagelist_get(plist, 0));
finish:
    hy_packagelist_free(plist);
    hy_query_free(q);
}

static HyRepo
config_repo(const char *name, const char *md_repo, const char *md_primary_xml)
{
    HyRepo repo = hy_repo_create();
    hy_repo_set_string(repo, NAME, name);
    hy_repo_set_string(repo, REPOMD_FN, md_repo);
    hy_repo_set_string(repo, PRIMARY_FN, md_primary_xml);
    return repo;
}

static void
rtrim(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len - 1] == '\n')
	str[len - 1 ] = '\0';
}

int
read_repopaths(char **md_repo, char **md_primary_xml,
	       char **md_repo_updates, char **md_primary_updates_xml)
{
    wordexp_t word_vector;

    *md_repo = *md_primary_xml = *md_repo_updates = *md_primary_updates_xml = NULL;

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
	(getline(md_repo_updates, &size, f) < 0) ||
	(getline(md_primary_updates_xml, &size, f) < 0)) {
	solv_free(*md_repo);
	solv_free(*md_primary_xml);
	solv_free(*md_repo_updates);
	solv_free(*md_primary_updates_xml);
	fclose(f);
	wordfree(&word_vector);
	return 1;
    }
    fclose(f);
    wordfree(&word_vector);
    rtrim(*md_repo);
    rtrim(*md_primary_xml);
    rtrim(*md_repo_updates);
    rtrim(*md_primary_updates_xml);
    return 0;
}

int main(int argc, const char **argv)
{
    HySack sack = hy_sack_create();
    HyRepo repo;
    char *md_repo;
    char *md_primary_xml;
    char *md_repo_updates;
    char *md_primary_updates_xml;

    if (read_repopaths(&md_repo, &md_primary_xml, &md_repo_updates,
		       &md_primary_updates_xml)) {
	fprintf(stderr,
		"This is hawkey testing hack, it needs a readable %s file "
		"containing the following paths on separate lines:\n"
		"<main repomd.xml path>\n"
		"<main primary.xml.gz path>\n"
		"<updates repomd.xml path>\n"
		"<updates primary.xml.gz path>\n", CFG_FILE);
	return 1;
    }
    /* rpmdb */
    hy_sack_load_rpm_repo(sack);
    /* Fedora repo */
    repo = config_repo("Fedora", md_repo, md_primary_xml);
    hy_sack_load_yum_repo(sack, repo);
    hy_repo_free(repo);
    /* Fedora updates repo */
    repo = config_repo("updates", md_repo_updates, md_primary_updates_xml);
    hy_sack_load_yum_repo(sack, repo);
    hy_repo_free(repo);
    solv_free(md_repo);
    solv_free(md_primary_xml);
    solv_free(md_repo_updates);
    solv_free(md_primary_updates_xml);

    if (argc == 2 && !strcmp(argv[1], "-u")) {
	updatables_query_all(sack);
    } else if (argc == 2 && !strcmp(argv[1], "-o")) {
	obsoletes(sack);
    } else if (argc == 2) {
	search_and_print(sack, argv[1]);
    } else if (argc == 3 && !strcmp(argv[1], "-r")) {
	search_filter_repos(sack, argv[2]);
    } else if (argc == 3 && !strcmp(argv[1], "-s")) {
	resolve_install(sack, argv[2]);
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

    hy_sack_write_all_repos(sack);
    hy_sack_free(sack);

    return 0;
}
