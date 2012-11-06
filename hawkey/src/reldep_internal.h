#ifndef HY_RELDEP_INTERNAL_H
#define HY_RELDEP_INTERNAL_H

#include "reldep.h"

HyReldep reldep_create(Pool *pool, Id r_id);
Id reldep_id(HyReldep reldep);
HyReldepList reldeplist_from_queue(Pool *pool, Queue h);

#endif // HY_RELDEP_INTERNAL_H
