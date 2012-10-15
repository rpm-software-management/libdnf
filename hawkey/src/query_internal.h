#ifndef HY_QUERY_INTERNAL_H
#define HY_QUERY_INTERNAL_H

// hawkey
#include "query.h"

union _Match {
    char *str;
    int num;
};

struct _Filter {
    int filter_type;
    int keyname;
    union _Match *matches;
    int nmatches;
    char *evr;
};

struct _Filter *filter_create(int nmatches);
void filter_reinit(struct _Filter *f, int nmatches);
void filter_free(struct _Filter *f);

#endif // HY_QUERY_INTERNAL_H
