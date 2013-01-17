#ifndef HY_SELECTOR_INTERNAL_H
#define HY_SELECTOR_INTERNAL_H

#include "selector.h"

struct _HySelector {
    HySack sack;
    struct _Filter *f_arch;
    struct _Filter *f_evr;
    struct _Filter *f_name;
    struct _Filter *f_provides;
};

static inline HySack selector_sack(HySelector sltr) { return sltr->sack; }

#endif // HY_SELECTOR_INTERNAL_H
