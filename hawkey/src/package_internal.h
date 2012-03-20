#ifndef PACKAGE_INTERNAL_H
#define PACKAGE_INTERNAL_H

#include "package.h"

struct _Package {
    Id id;
    Pool *pool;
};

Package package_create(Pool *pool, Id id);
static inline Id package_id(Package pkg) { return pkg->id; }
Package package_from_solvable(Solvable *s);

#endif // PACKAGE_INTERNAL_H
