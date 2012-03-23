#ifndef REPO_H
#define REPO_H

#include "types.h"

enum hy_repo_param_e {
    NAME,
    REPOMD_FN,
    PRIMARY_FN
};

HyRepo hy_repo_create(void);
void hy_repo_set_string(HyRepo repo, enum hy_repo_param_e which, const char *str_val);
const char *hy_repo_get_string(HyRepo repo, enum hy_repo_param_e which);
void hy_repo_free(HyRepo repo);

#endif // REPO_H
