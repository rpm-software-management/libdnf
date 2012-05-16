#ifndef PACKAGE_H
#define PACKAGE_H

// libsolv
#include "solv/solvable.h"

// hawkey
#include "types.h"

// public
void hy_package_free(HyPackage pkg);
HyPackage hy_package_link(HyPackage pkg);
int hy_package_cmp(HyPackage pkg1, HyPackage pkg2);
int hy_package_evr_cmp(HyPackage pkg1, HyPackage pkg2);
char *hy_package_get_location(HyPackage pkg);
char *hy_package_get_nvra(HyPackage pkg);
const char* hy_package_get_name(HyPackage pkg);
const char* hy_package_get_arch(HyPackage pkg);
const char* hy_package_get_evr(HyPackage pkg);
const char* hy_package_get_reponame(HyPackage pkg);
const char* hy_package_get_summary(HyPackage pkg);
int hy_package_get_medianr(HyPackage pkg);
int hy_package_get_rpmdbid(HyPackage pkg);
int hy_package_get_size(HyPackage pkg);

#endif
