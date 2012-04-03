#ifndef REPO_INTERNAL_H
#define REPO_INTERNAL_H

#include "repo.h"

enum _hy_repo_state {
    NEW,
    LOADED,
    CACHED,
    FL_LOADED,
    FL_CACHED,
};

struct _HyRepo {
    int nrefs;
    char *name;
    char *repomd_fn;
    char *primary_fn;
    char *filelists_fn;
    enum _hy_repo_state state;
};

HyRepo hy_repo_link(HyRepo repo);
int hy_repo_transition(HyRepo repo, enum _hy_repo_state new_state);

#endif // REPO_INTERNAL_H
