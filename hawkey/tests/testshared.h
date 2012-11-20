#ifndef TESTSHARED_H
#define TESTSHARED_H

// libsolv
#include <solv/pooltypes.h>

// hawkey
#include "src/repo.h"

HyRepo glob_for_repofiles(Pool *pool, const char *repo_name, const char *path);
int load_repo(Pool *pool, const char *name, const char *path, int installed);

#endif /* TESTSHARED_H */
