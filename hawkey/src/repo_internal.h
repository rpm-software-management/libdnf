#ifndef REPO_INTERNAL_H
#define REPO_INTERNAL_H

#include "repo.h"

struct _HyRepo {
    int nrefs;
    char *name;
    char *repomd_fn;
    char *primary_fn;
    char *filelists_fn;
    int from_cache;
};

HyRepo hy_repo_link(HyRepo repo);

#endif // REPO_INTERNAL_H
