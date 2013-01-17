#ifndef HY_SELECTOR_H
#define HY_SELECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

HySelector hy_selector_create(HySack sack);
void hy_selector_free(HySelector sltr);
int hy_selector_set(HySelector sltr, int keyname, int cmp_type,
		    const char *match);
HyPackageList hy_selector_matches(HySelector sltr);

#ifdef __cplusplus
}
#endif

#endif
