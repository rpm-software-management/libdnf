#ifndef FREPO_H
#define FREPO_H

#include "types.h"

enum frepo_param_e {
    NAME,
    REPOMD_FN,
    PRIMARY_FN
};

FRepo frepo_create(void);
void frepo_set_string(FRepo repo, enum frepo_param_e which, const char *str_val);
const char *frepo_get_string(FRepo repo, enum frepo_param_e which);
void frepo_free(FRepo repo);

#endif // FREPO_H
