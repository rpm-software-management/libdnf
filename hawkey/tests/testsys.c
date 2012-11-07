#define _GNU_SOURCE
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <wordexp.h>
#include <sys/stat.h>
#include <sys/types.h>

// libsolv
#include <solv/repo.h>
#include <solv/testcase.h>

// hawkey
#include "src/package.h"
#include "src/query.h"
#include "src/repo.h"
#include "src/sack_internal.h"
#include "src/util.h"
#include "testsys.h"

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

void
dump_packagelist(HyPackageList plist)
{
    for (int i = 0; i < hy_packagelist_count(plist); ++i) {
	HyPackage pkg = hy_packagelist_get(plist, i);
	char *nvra = hy_package_get_nvra(pkg);
	printf("\t%s\n", nvra);
	hy_free(nvra);
    }
}

HyRepo
glob_for_repofiles(Pool *pool, const char *repo_name, const char *path)
{
    HyRepo repo = hy_repo_create(repo_name);
    const char *tmpl;
    wordexp_t word_vector;

    tmpl = pool_tmpjoin(pool, path, "/repomd.xml", NULL);
    if (wordexp(tmpl, &word_vector, 0) || word_vector.we_wordc < 1)
	goto fail;
    hy_repo_set_string(repo, HY_REPO_MD_FN, word_vector.we_wordv[0]);

    tmpl = pool_tmpjoin(pool, path, "/*primary.xml.gz", NULL);
    if (wordexp(tmpl, &word_vector, WRDE_REUSE) || word_vector.we_wordc < 1)
	goto fail;
    hy_repo_set_string(repo, HY_REPO_PRIMARY_FN, word_vector.we_wordv[0]);

    tmpl = pool_tmpjoin(pool, path, "/*filelists.xml.gz", NULL);
    if (wordexp(tmpl, &word_vector, WRDE_REUSE) || word_vector.we_wordc < 1)
	goto fail;
    hy_repo_set_string(repo, HY_REPO_FILELISTS_FN, word_vector.we_wordv[0]);

    tmpl = pool_tmpjoin(pool, path, "/*prestodelta.xml.gz", NULL);
    if (wordexp(tmpl, &word_vector, WRDE_REUSE) || word_vector.we_wordc < 1)
	goto fail;
    hy_repo_set_string(repo, HY_REPO_PRESTO_FN, word_vector.we_wordv[0]);

    wordfree(&word_vector);
    return repo;

 fail:
    wordfree(&word_vector);
    hy_repo_free(repo);
    return NULL;
}

int
load_repo(Pool *pool, const char *name, const char *path, int installed)
{
    Repo *r = repo_create(pool, name);
    FILE *fp = fopen(path, "r");

    if (!fp)
	return 1;
    testcase_add_testtags(r,  fp, 0);
    if (installed)
	pool_set_installed(pool, r);
    fclose(fp);
    return 0;
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

HyRepo
repo_by_name(Pool *pool, const char *name)
{
    Repo *repo;
    int rid;
    FOR_REPOS(rid, repo)
	if (!strcmp(repo->name, name))
	    return repo->appdata;
    return NULL;
}
