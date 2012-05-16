#ifndef PACKAGELIST_H
#define PACKAGELIST_H

// libsolv
#include "solv/queue.h"

// hawkey
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

HyPackageListIter hy_packagelist_iter_create(HyPackageList plist);
void hy_packagelist_iter_free(HyPackageListIter iter);
HyPackage hy_packagelist_iter_next(HyPackageListIter iter);

#endif // PACKAGELIST_H
