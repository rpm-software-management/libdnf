#ifndef HY_UTIL_H
#define HY_UTIL_H

void *hy_free(void *mem);
const char *chksum_name(int chksum_type);
char *chksum_str(const unsigned char *chksum, int type);

#endif /* HY_UTIL_H */
