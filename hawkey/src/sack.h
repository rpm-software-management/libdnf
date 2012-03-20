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

HySack sack_create(void);
void sack_free(HySack sack);
void sack_create_cmdline_repo(HySack sack);
HyPackage sack_add_cmdline_rpm(HySack sack, const char *fn);
void sack_load_rpm_repo(HySack sack);
void sack_load_yum_repo(HySack sack, HyRepo repo);
void sack_solve(HySack sack, Queue *job, Map *res_map, int mode);
int sack_write_all_repos(HySack sack);

#endif // SACK_H
