#ifndef SACK_INTERNAL_H
#define SACK_INTERNAL_H

#include "sack.h"

struct _Sack {
    Pool *pool;
    int provides_ready;
};

void sack_make_provides_ready(Sack sack);
void sack_same_names(Sack sack, Id name, Queue *same);
static inline Pool *sack_pool(Sack sack) { return sack->pool; }

#endif // SACK_INTERNAL_H
