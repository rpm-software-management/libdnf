#ifndef HY_PACKAGESET_H
#define HY_PACKAGESET_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

HyPackageSet hy_packageset_create(HySack sack);
HyPackageSet hy_packageset_clone(HyPackageSet pset);
void hy_packageset_free(HyPackageSet pset);
void hy_packageset_add(HyPackageSet pset, HyPackage pkg);
unsigned hy_packageset_count(HyPackageSet pset);
HyPackage hy_packageset_get_clone(HyPackageSet pset, int index);
int hy_packageset_has(HyPackageSet pset, HyPackage pkg);

#ifdef __cplusplus
}
#endif

#endif /* HY_PACKAGESET_H */
