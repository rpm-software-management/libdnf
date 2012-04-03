#include <assert.h>

// libsolv
#include "solv/util.h"

// hawkey
#include "repo_internal.h"


HyRepo
hy_repo_link(HyRepo repo)
{
    repo->nrefs++;
    return repo;
}

int
hy_repo_transition(HyRepo repo, enum _hy_repo_state new_state)
{
    switch (repo->state) {
    case NEW:
	if (new_state != LOADED && new_state != CACHED)
	    return 1;
	break;
    case LOADED:
	if (new_state == FL_CACHED)
	    return 1;
	break;
    case CACHED:
	if (new_state == FL_LOADED || new_state == FL_CACHED)
	    break;
	return 1;
    case FL_LOADED:
	if (new_state == FL_CACHED)
	    break;
	return 1;
    default:
	return 1;
    }
    repo->state = new_state;
    return 0;
}

HyRepo
hy_repo_create(void)
{
    HyRepo repo = solv_calloc(1, sizeof(*repo));
    repo->nrefs = 1;
    repo->state = NEW;
    return repo;
}

void
hy_repo_set_string(HyRepo repo, enum _hy_repo_param_e which, const char *str_val)
{
    switch (which) {
    case HY_REPO_NAME:
	repo->name = solv_strdup(str_val);
	break;
    case HY_REPO_MD_FN:
	repo->repomd_fn = solv_strdup(str_val);
	break;
    case HY_REPO_PRIMARY_FN:
	repo->primary_fn = solv_strdup(str_val);
	break;
    case HY_REPO_FILELISTS_FN:
	repo->filelists_fn = solv_strdup(str_val);
	break;
    default:
	assert(0);
    }
}

const char *
hy_repo_get_string(HyRepo repo, enum _hy_repo_param_e which)
{
    switch(which) {
    case HY_REPO_NAME:
	return repo->name;
    case HY_REPO_MD_FN:
	return repo->repomd_fn;
    case HY_REPO_PRIMARY_FN:
	return repo->primary_fn;
    case HY_REPO_FILELISTS_FN:
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
