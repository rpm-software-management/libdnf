#ifndef HY_PACKAGESET_INTERNAL_H
#define HY_PACKAGESET_INTERNAL_H

// libsolv
#include <solv/pooltypes.h>
#include <solv/bitmap.h>

// hawkey
#include "packageset.h"

unsigned map_count(Map *m);
HyPackageSet packageset_from_bitmap(HySack sack, Map *m);
Map *packageset_get_map(HyPackageSet pset);
Id packageset_get_pkgid(HyPackageSet pset, int index, Id previous);

#endif // HY_PACKAGESET_INTERNAL_H
