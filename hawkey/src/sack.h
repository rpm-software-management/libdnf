#ifndef SACK_H
#define SACK_H

// libsolv
#include "solv/pool.h"

// hawkey
#include "frepo.h"
#include "package.h"
#include "types.h"

#define SYSTEM_REPO_NAME "@System"
#define CMDLINE_REPO_NAME "@commandline"

Sack sack_create(void);
void sack_free(Sack sack);
void sack_create_cmdline_repo(Sack sack);
Package sack_add_cmdline_rpm(Sack sack, const char *fn);
void sack_load_rpm_repo(Sack sack);
void sack_load_yum_repo(Sack sack, FRepo repo);
void sack_solve(Sack sack, Queue *job, Map *res_map, int mode);
int sack_write_all_repos(Sack sack);

#endif // SACK_H
