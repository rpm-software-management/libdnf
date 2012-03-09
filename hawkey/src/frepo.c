#include <assert.h>

#include "util.h"

#include "frepo.h"

FRepo
frepo_create(void)
{
    FRepo repo = solv_calloc(1, sizeof(*repo));
    return repo;
}

void
frepo_set_string(FRepo repo, enum frepo_param_e which, const char *str_val)
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
    default:
	assert(1);
    }
}

const char *
frepo_get_string(FRepo repo, enum frepo_param_e which)
{
    switch(which) {
    case NAME:
	return repo->name;
    case REPOMD_FN:
	return repo->repomd_fn;
    case PRIMARY_FN:
	return repo->primary_fn;
    default:
	assert(1);
    }
    return NULL;
}

void
frepo_free(FRepo repo)
{
    solv_free(repo->name);
    solv_free(repo->repomd_fn);
    solv_free(repo->primary_fn);
    solv_free(repo);
}
