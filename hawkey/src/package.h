/*
 * Copyright (C) 2012-2014 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef HY_PACKAGE_H
#define HY_PACKAGE_H

#ifdef __cplusplus
extern "C" {
#endif

// hawkey
#include "types.h"
#include <glib.h>

// deprecated in 0.4.12, eligible for dropping after 2014-07-07 AND no sooner
// than in 0.4.15, use HyAdvisoryType instead
typedef enum {
	HY_UPDATE_SEVERITY_SECURITY = 0,	/* ordered by rough importance */
	HY_UPDATE_SEVERITY_IMPORTANT = 1,
	HY_UPDATE_SEVERITY_BUGFIX = 2,
	HY_UPDATE_SEVERITY_NORMAL = 3,
	HY_UPDATE_SEVERITY_ENHANCEMENT = 4,
	HY_UPDATE_SEVERITY_LOW = 5,
	HY_UPDATE_SEVERITY_UNKNOWN = 6,
} HyUpdateSeverity;

/* public */
void hy_package_free(HyPackage pkg);
HyPackage hy_package_link(HyPackage pkg);
int hy_package_identical(HyPackage pkg1, HyPackage pkg2);
int hy_package_installed(HyPackage pkg);
int hy_package_cmp(HyPackage pkg1, HyPackage pkg2);
int hy_package_evr_cmp(HyPackage pkg1, HyPackage pkg2);

char *hy_package_get_location(HyPackage pkg);
const char *hy_package_get_baseurl(HyPackage pkg);
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
unsigned long long hy_package_get_downloadsize(HyPackage pkg);
unsigned long long hy_package_get_epoch(HyPackage pkg);
unsigned long long hy_package_get_hdr_end(HyPackage pkg);
unsigned long long hy_package_get_installsize(HyPackage pkg);
unsigned long long hy_package_get_medianr(HyPackage pkg);
unsigned long long hy_package_get_rpmdbid(HyPackage pkg);
unsigned long long hy_package_get_size(HyPackage pkg);
unsigned long long hy_package_get_buildtime(HyPackage pkg);
unsigned long long hy_package_get_installtime(HyPackage pkg);

HyReldepList hy_package_get_conflicts(HyPackage pkg);
HyReldepList hy_package_get_enhances(HyPackage pkg);
HyReldepList hy_package_get_obsoletes(HyPackage pkg);
HyReldepList hy_package_get_provides(HyPackage pkg);
HyReldepList hy_package_get_recommends(HyPackage pkg);
HyReldepList hy_package_get_requires(HyPackage pkg);
HyReldepList hy_package_get_suggests(HyPackage pkg);
HyReldepList hy_package_get_supplements(HyPackage pkg);
HyStringArray hy_package_get_files(HyPackage pkg);
HyAdvisoryList hy_package_get_advisories(HyPackage pkg, int cmp_type);

typedef void (*HyUserdataDestroy) (void *userdata);
void *hy_package_get_userdata(HyPackage pkg);
void hy_package_set_userdata(HyPackage pkg, void *userdata, HyUserdataDestroy destroy_func);

HyPackageDelta hy_package_get_delta_from_evr(HyPackage pkg, const char *from_evr);
const char *hy_packagedelta_get_location(HyPackageDelta delta);
const char *hy_packagedelta_get_baseurl(HyPackageDelta delta);
unsigned long long hy_packagedelta_get_downloadsize(HyPackageDelta delta);
const unsigned char *hy_packagedelta_get_chksum(HyPackageDelta delta, int *type);
void hy_packagedelta_free(HyPackageDelta delta);

#ifdef __cplusplus
}
#endif

#endif
