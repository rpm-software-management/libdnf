// hawkey
#include "errno.h"

__thread int hy_errno = 0;

/**
 * Get the Hawkey errno value.
 *
 * Recommended over direct variable access since that in a far-out theory could
 * change implementation.
 */
int
hy_get_errno(void)
{
    return hy_errno;
}
