/*
 * Copyright (C) 2012-2015 Red Hat, Inc.
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

#ifndef HY_SACK_H
#define HY_SACK_H

#ifdef __cplusplus
extern "C" {
#endif

// hawkey
#include "types.h"

enum _hy_sack_sack_create_flags {
    HY_MAKE_CACHE_DIR = 1 << 0
};

enum _hy_sack_repo_load_flags {
    HY_BUILD_CACHE	= 1 << 0,
    HY_LOAD_FILELISTS	= 1 << 1,
    HY_LOAD_PRESTO	= 1 << 2,
    HY_LOAD_UPDATEINFO	= 1 << 3
};

HySack hy_sack_create(const char *cachedir, const char *arch, const char *rootdir,
		      const char* logfile, int flags);
void hy_sack_free(HySack sack);
int hy_sack_evr_cmp(HySack sack, const char *evr1, const char *evr2);
const char *hy_sack_get_cache_dir(HySack sack);
HyPackage hy_sack_get_running_kernel(HySack sack);
char *hy_sack_give_cache_fn(HySack sack, const char *reponame, const char *ext);
const char **hy_sack_list_arches(HySack sack);
void hy_sack_set_installonly(HySack sack, const char **installonly);
void hy_sack_set_installonly_limit(HySack sack, int limit);
void hy_sack_create_cmdline_repo(HySack sack);
HyPackage hy_sack_add_cmdline_package(HySack sack, const char *fn);
int hy_sack_count(HySack sack);
void hy_sack_add_excludes(HySack sack, HyPackageSet pset);
void hy_sack_add_includes(HySack sack, HyPackageSet pset);
void hy_sack_set_excludes(HySack sack, HyPackageSet pset);
void hy_sack_set_includes(HySack sack, HyPackageSet pset);
int hy_sack_repo_enabled(HySack sack, const char *reponame, int enabled);

/**
 * Load RPMDB, the system package database.
 *
 * @returns           0 on success, HY_E_IO on fatal error,
 *		      HY_E_CACHE_WRITE on cache write error.
 */
int hy_sack_load_system_repo(HySack sack, HyRepo a_hrepo, int flags);

// deprecated in 0.5.5, eligible for dropping after 2015-10-27 AND no sooner
// than in 0.5.8, use hy_advisorypkg_get_string instead
int hy_sack_load_yum_repo(HySack sack, HyRepo hrepo, int flags);
int hy_sack_load_repo(HySack sack, HyRepo hrepo, int flags);

#ifdef __cplusplus
}
#endif

#endif /* HY_SACK_H */
