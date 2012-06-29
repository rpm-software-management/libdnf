#ifndef HY_PACKAGELIST_H
#define HY_PACKAGELIST_H

#ifdef __cplusplus
extern "C" {
#endif

/* libsolv */
#include "solv/queue.h"

/* hawkey */
#include "package.h"
#include "sack.h"
#include "types.h"

HyPackageList hy_packagelist_create(void);
HyPackageList hy_packagelist_of_obsoletes(HySack sack, HyPackage pkg);
void hy_packagelist_free(HyPackageList plist);
int hy_packagelist_count(HyPackageList plist);
HyPackage hy_packagelist_get(HyPackageList plist, int index);
HyPackage hy_packagelist_get_clone(HyPackageList plist, int index);
void hy_packagelist_push(HyPackageList plist, HyPackage pkg);

#define FOR_PACKAGELIST(pkg, pkglist, i)						\
    for (i = 0; (pkg = hy_packagelist_get(pkglist, i++)) != NULL; )

#ifdef __cplusplus
}
#endif

#endif /* HY_PACKAGELIST_H */
