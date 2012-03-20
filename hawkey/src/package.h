#ifndef PACKAGE_H
#define PACKAGE_H

// libsolv
#include "solv/solvable.h"

// hawkey
#include "types.h"

// public
void package_free(Package pkg);
int package_cmp(Package pkg1, Package pkg2);
int package_evr_cmp(Package pkg1, Package pkg2);
char *package_get_location(Package pkg);
char *package_get_nvra(Package pkg);
const char* package_get_name(Package pkg);
const char* package_get_arch(Package pkg);
const char* package_get_evr(Package pkg);
const char* package_get_reponame(Package pkg);
int package_get_medianr(Package pkg);
int package_get_rpmdbid(Package pkg);
int package_get_size(Package pkg);

#endif
