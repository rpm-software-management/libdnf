#ifndef HY_PACKAGE_INTERNAL_H
#define HY_PACKAGE_INTERNAL_H

#include "package.h"

struct _HyPackage {
    int nrefs;
    Id id;
    Pool *pool;
};

struct _HyPackageDelta {
    char *location;
};

HyPackage package_create(Pool *pool, Id id);
static inline Id package_id(HyPackage pkg) { return pkg->id; }
HyPackage package_from_solvable(Solvable *s);
HyPackageDelta delta_create(void);

#endif // HY_PACKAGE_INTERNAL_H
