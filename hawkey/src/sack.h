#ifndef SACK_H
#define SACK_H

#include "frepo.h"
#include "package.h"
#include "pool.h"

#define SYSTEM_REPO_NAME "@System"
#define CMDLINE_REPO_NAME "@commandline"

struct _Sack {
    Pool *pool;
    int provides_ready;
};

typedef struct _Sack * Sack;

Sack sack_create(void);
void sack_free(Sack sack);
void sack_create_cmdline_repo(Sack sack);
Package sack_add_cmdline_rpm(Sack sack, const char *fn);
void sack_load_rpm_repo(Sack sack);
void sack_load_yum_repo(Sack sack, FRepo repo);
void sack_solve(Sack sack, Queue *job, Map *res_map, int mode);
int sack_write_all_repos(Sack sack);

// internal
void sack_make_provides_ready(Sack sack);
void sack_same_names(Sack sack, Id name, Queue *same);

#endif
