#include <stdlib.h>

// hawkey
#include "util.h"

void *
hy_free(void *mem)
{
    /* currently does exactly what solv_free() does: */
    free(mem);
    return 0;
}
