#ifndef PACKAGELIST_H
#define PACKAGELIST_H

// libsolv
#include "solv/queue.h"

// hawkey
#include "package.h"
#include "sack.h"
#include "types.h"

HyPackageList packagelist_create(void);
HyPackageList packagelist_of_obsoletes(HySack sack, HyPackage pkg);
void packagelist_free(HyPackageList plist);
int packagelist_count(HyPackageList plist);
HyPackage packagelist_get(HyPackageList plist, int index);
void packagelist_push(HyPackageList plist, HyPackage pkg);

HyPackageListIter packagelist_iter_create(HyPackageList plist);
void packagelist_iter_free(HyPackageListIter iter);
HyPackage packagelist_iter_next(HyPackageListIter iter);

#endif // PACKAGELIST_H
