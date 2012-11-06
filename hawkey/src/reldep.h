#ifndef HY_RELDEP_H
#define HY_RELDEP_H

#ifdef __cplusplus
extern "C" {
#endif

/* hawkey */
#include "types.h"

void hy_reldep_free(HyReldep reldep);
HyReldep hy_reldep_clone(HyReldep reldep);
char *hy_reldep_str(HyReldep reldep);

void hy_reldeplist_free(HyReldepList reldeplist);
int hy_reldeplist_count(HyReldepList reldeplist);
HyReldep hy_reldeplist_get_clone(HyReldepList reldeplist, int index);

#ifdef __cplusplus
}
#endif

#endif /* HY_RELDEP_H */
