#ifndef PACKAGE_H
#define PACKAGE_H

// libsolv
#include "solv/solvable.h"

// hawkey
#include "types.h"

// public
void package_free(HyPackage pkg);
int package_cmp(HyPackage pkg1, HyPackage pkg2);
int package_evr_cmp(HyPackage pkg1, HyPackage pkg2);
char *package_get_location(HyPackage pkg);
char *package_get_nvra(HyPackage pkg);
const char* package_get_name(HyPackage pkg);
const char* package_get_arch(HyPackage pkg);
const char* package_get_evr(HyPackage pkg);
const char* package_get_reponame(HyPackage pkg);
int package_get_medianr(HyPackage pkg);
int package_get_rpmdbid(HyPackage pkg);
int package_get_size(HyPackage pkg);

#endif
