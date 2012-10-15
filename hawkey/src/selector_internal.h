#ifndef HY_SELECTOR_INTERNAL_H
#define HY_SELECTOR_INTERNAL_H

#include "selector.h"

struct _HySelector {
    HySack sack;
    struct _Filter *f_arch;
    struct _Filter *f_name;
};

#endif // HY_SELECTOR_INTERNAL_H