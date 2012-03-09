#ifndef FREPO_H
#define FREPO_H

enum frepo_param_e {
    NAME,
    REPOMD_FN,
    PRIMARY_FN
};

struct _FRepo {
    char *name;
    char *repomd_fn;
    char *primary_fn;
};

typedef struct _FRepo * FRepo; // can't call this Repo, nameclash libsolv

FRepo frepo_create(void);
void frepo_set_string(FRepo repo, enum frepo_param_e which, const char *str_val);
const char *frepo_get_string(FRepo repo, enum frepo_param_e which);
void frepo_free(FRepo repo);

#endif // FREPO_H
