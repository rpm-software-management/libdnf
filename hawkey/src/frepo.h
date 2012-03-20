#ifndef FREPO_H
#define FREPO_H

#include "types.h"

enum frepo_param_e {
    NAME,
    REPOMD_FN,
    PRIMARY_FN
};

HyRepo frepo_create(void);
void frepo_set_string(HyRepo repo, enum frepo_param_e which, const char *str_val);
const char *frepo_get_string(HyRepo repo, enum frepo_param_e which);
void frepo_free(HyRepo repo);

#endif // FREPO_H
