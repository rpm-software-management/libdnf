#ifndef HY_SACK_INTERNAL_H
#define HY_SACK_INTERNAL_H

#include <stdio.h>

// libsolv
#include <solv/pool.h>

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
void sack_same_names(HySack sack, Id name, Id arch, Queue *same);
static inline Pool *sack_pool(HySack sack) { return sack->pool; }
static inline Id sack_last_solvable(HySack sack)
{
    return sack_pool(sack)->nsolvables - 1;
}

#endif // HY_SACK_INTERNAL_H
