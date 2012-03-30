#include <assert.h>

// libsolv
#include "solv/util.h"

// hawkey
#include "repo_internal.h"


HyRepo hy_repo_link(HyRepo repo)
{
    repo->nrefs++;
    return repo;
}

HyRepo
hy_repo_create(void)
{
    HyRepo repo = solv_calloc(1, sizeof(*repo));
    repo->nrefs = 1;
    return repo;
}

void
hy_repo_set_string(HyRepo repo, enum hy_repo_param_e which, const char *str_val)
{
    switch (which) {
    case NAME:
	repo->name = solv_strdup(str_val);
	break;
    case REPOMD_FN:
	repo->repomd_fn = solv_strdup(str_val);
	break;
    case PRIMARY_FN:
	repo->primary_fn = solv_strdup(str_val);
	break;
    case FILELISTS_FN:
	repo->filelists_fn = solv_strdup(str_val);
	break;
    default:
	assert(0);
    }
}

const char *
hy_repo_get_string(HyRepo repo, enum hy_repo_param_e which)
{
    switch(which) {
    case NAME:
	return repo->name;
    case REPOMD_FN:
	return repo->repomd_fn;
    case PRIMARY_FN:
	return repo->primary_fn;
    case FILELISTS_FN:
	return repo->filelists_fn;
    default:
	assert(0);
    }
    return NULL;
}

#include <stdio.h>

void
hy_repo_free(HyRepo repo)
{
    if (--repo->nrefs > 0)
	return;

    solv_free(repo->name);
    solv_free(repo->repomd_fn);
    solv_free(repo->primary_fn);
    solv_free(repo->filelists_fn);
    solv_free(repo);
}
