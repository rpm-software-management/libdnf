#ifndef HY_PACKAGE_H
#define HY_PACKAGE_H

#ifdef __cplusplus
extern "C" {
#endif

/* libsolv */
#include "solv/pool.h"
#include "solv/solvable.h"

/* hawkey */
#include "types.h"

#define HY_CHKSUM_MD5 REPOKEY_TYPE_MD5
#define HY_CHKSUM_SHA1 REPOKEY_TYPE_SHA1
#define HY_CHKSUM_SHA256 REPOKEY_TYPE_SHA256

/* public */
void hy_package_free(HyPackage pkg);
HyPackage hy_package_link(HyPackage pkg);
int hy_package_cmp(HyPackage pkg1, HyPackage pkg2);
int hy_package_evr_cmp(HyPackage pkg1, HyPackage pkg2);
char *hy_package_get_location(HyPackage pkg);
char *hy_package_get_nvra(HyPackage pkg);
const char *hy_package_get_name(HyPackage pkg);
const char *hy_package_get_arch(HyPackage pkg);
const unsigned char *hy_package_get_chksum(HyPackage pkg, int *type);
const char *hy_package_get_evr(HyPackage pkg);
const unsigned char *hy_package_get_hdr_chksum(HyPackage pkg, int *type);
const char *hy_package_get_reponame(HyPackage pkg);
const char *hy_package_get_summary(HyPackage pkg);
int hy_package_get_medianr(HyPackage pkg);
int hy_package_get_rpmdbid(HyPackage pkg);
int hy_package_get_size(HyPackage pkg);

HyPackageDelta hy_package_get_delta_from_evr(HyPackage pkg, const char *from_evr);
const char *hy_packagedelta_get_location(HyPackageDelta delta);
void hy_packagedelta_free(HyPackageDelta delta);

#ifdef __cplusplus
}
#endif

#endif
