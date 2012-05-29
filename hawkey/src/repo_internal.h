#ifndef REPO_INTERNAL_H
#define REPO_INTERNAL_H

// libsolv
#include <solv/pooltypes.h>

// hawkey
#include "iutil.h"
#include "repo.h"

enum _hy_repo_state {
    _HY_NEW = 1,
    _HY_LOADED_FETCH,
    _HY_LOADED_CACHE,
    _HY_WRITTEN,
    _HY_FL_LOADED_FETCH,
    _HY_FL_LOADED_CACHE,
    _HY_FL_WRITTEN,
    _HY_PST_LOADED_FETCH,
    _HY_PST_LOADED_CACHE,
    _HY_PST_WRITTEN,
};

struct _HyRepo {
    int nrefs;
    char *name;
    char *repomd_fn;
    char *primary_fn;
    char *filelists_fn;
    char *presto_fn;
    enum _hy_repo_state state;
    Id filenames_repodata;
    Id presto_repodata;
    unsigned char checksum[CHKSUM_BYTES];
};

HyRepo hy_repo_link(HyRepo repo);
int hy_repo_transition(HyRepo repo, enum _hy_repo_state new_state);

#endif // REPO_INTERNAL_H
