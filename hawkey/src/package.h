#ifndef HY_PACKAGE_H
#define HY_PACKAGE_H

#ifdef __cplusplus
extern "C" {
#endif

// hawkey
#include "types.h"

/* public */
void hy_package_free(HyPackage pkg);
HyPackage hy_package_link(HyPackage pkg);
int hy_package_identical(HyPackage pkg1, HyPackage pkg2);
int hy_package_cmp(HyPackage pkg1, HyPackage pkg2);
int hy_package_evr_cmp(HyPackage pkg1, HyPackage pkg2);

char *hy_package_get_location(HyPackage pkg);
char *hy_package_get_nevra(HyPackage pkg);
char *hy_package_get_sourcerpm(HyPackage pkg);
char *hy_package_get_version(HyPackage pkg);
char *hy_package_get_release(HyPackage pkg);

const char *hy_package_get_name(HyPackage pkg);
const char *hy_package_get_arch(HyPackage pkg);
const unsigned char *hy_package_get_chksum(HyPackage pkg, int *type);
const char *hy_package_get_description(HyPackage pkg);
const char *hy_package_get_evr(HyPackage pkg);
const char *hy_package_get_license(HyPackage pkg);
const unsigned char *hy_package_get_hdr_chksum(HyPackage pkg, int *type);
const char *hy_package_get_packager(HyPackage pkg);
const char *hy_package_get_reponame(HyPackage pkg);
const char *hy_package_get_summary(HyPackage pkg);
const char *hy_package_get_url(HyPackage pkg);
unsigned long long hy_package_get_epoch(HyPackage pkg);
unsigned long long hy_package_get_medianr(HyPackage pkg);
unsigned long long hy_package_get_rpmdbid(HyPackage pkg);
unsigned long long hy_package_get_size(HyPackage pkg);
unsigned long long hy_package_get_buildtime(HyPackage pkg);
unsigned long long hy_package_get_installtime(HyPackage pkg);

HyReldepList hy_package_get_conflicts(HyPackage pkg);
HyReldepList hy_package_get_obsoletes(HyPackage pkg);
HyReldepList hy_package_get_provides(HyPackage pkg);
HyReldepList hy_package_get_requires(HyPackage pkg);

HyPackageDelta hy_package_get_delta_from_evr(HyPackage pkg, const char *from_evr);
const char *hy_packagedelta_get_location(HyPackageDelta delta);
void hy_packagedelta_free(HyPackageDelta delta);

#ifdef __cplusplus
}
#endif

#endif
