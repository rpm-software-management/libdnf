#ifndef HY_REPO_H
#define HY_REPO_H

#ifdef __cplusplus
extern "C" {
#endif

// hawkey
#include "types.h"

enum _hy_repo_param_e {
    HY_REPO_NAME,
    HY_REPO_MD_FN,
    HY_REPO_PRESTO_FN,
    HY_REPO_PRIMARY_FN,
    HY_REPO_FILELISTS_FN
};

HyRepo hy_repo_create(const char *name);
void hy_repo_set_string(HyRepo repo, int which, const char *str_val);
const char *hy_repo_get_string(HyRepo repo, int which);
void hy_repo_free(HyRepo repo);

#ifdef __cplusplus
}
#endif

#endif /* HY_REPO_H */
