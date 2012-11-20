#include <wordexp.h>

// libsolv
#include <solv/pool.h>
#include <solv/repo.h>
#include <solv/testcase.h>

// hawkey
#include "testshared.h"

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
