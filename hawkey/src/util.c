#include <stdlib.h>

// hawkey
#include "package.h"
#include "util.h"

void *
hy_free(void *mem)
{
    /* currently does exactly what solv_free() does: */
    free(mem);
    return 0;
}

const char *
chksum_name(int chksum_type)
{
    switch (chksum_type) {
    case HY_CHKSUM_MD5:
	return "md5";
    case HY_CHKSUM_SHA1:
	return "sha1";
    case HY_CHKSUM_SHA256:
	return "sha256";
    default:
	return NULL;
    }
}
