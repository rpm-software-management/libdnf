#ifndef SACK_INTERNAL_H
#define SACK_INTERNAL_H

#include <stdio.h>

// hawkey
#include "sack.h"

struct _HySack {
    Pool *pool;
    int provides_ready;
    char *cache_dir;
    Queue installonly;
    FILE *log_out;
};

void sack_make_provides_ready(HySack sack);
void sack_log(HySack sack, int level, const char *format, ...);
void sack_same_names(HySack sack, Id name, Queue *same);
static inline Pool *sack_pool(HySack sack) { return sack->pool; }

#endif // SACK_INTERNAL_H
