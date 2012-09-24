#include <stdlib.h>
#include <strings.h>

// hawkey
#include "iutil.h"
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

int
chksum_type(const char *chksum_name)
{
    if (!strcasecmp(chksum_name, "md5"))
	return HY_CHKSUM_MD5;
    if (!strcasecmp(chksum_name, "sha1"))
	return HY_CHKSUM_SHA1;
    if (!strcasecmp(chksum_name, "sha256"))
	return HY_CHKSUM_SHA256;
    return 0;
}

char *
chksum_str(const unsigned char *chksum, int type)
{
    int length = checksum_type2length(type);
    char *s = solv_malloc(2 * length + 1);
    solv_bin2hex(chksum, length, s);

    return s;
}
