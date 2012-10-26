#ifndef HY_PACKAGESET_INTERNAL_H
#define HY_PACKAGESET_INTERNAL_H

// libsolv
#include <solv/bitmap.h>

// hawkey
#include "packageset.h"

HyPackageSet packageset_from_bitmap(HySack sack, Map *m);
Map *packageset_get_map(HyPackageSet pset);

#endif // HY_PACKAGESET_INTERNAL_H
