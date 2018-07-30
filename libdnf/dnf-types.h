/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2013-2015 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2015 Colin Walters <walters@verbum.org>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */


#ifndef __DNF_TYPES_H
#define __DNF_TYPES_H

#include <gio/gio.h>

#ifdef __cplusplus
namespace libdnf {
    struct Swdb;
    struct PackageSet;
    struct Dependency;
    struct DependencyContainer;
}
typedef struct libdnf::Swdb DnfDb;
typedef struct libdnf::PackageSet DnfPackageSet;
typedef struct libdnf::Dependency DnfReldep;
typedef struct libdnf::DependencyContainer DnfReldepList;
#else
typedef struct Swdb DnfDb;
typedef struct PackageSet DnfPackageSet;
typedef struct Dependency DnfReldep;
typedef struct DependencyContainer DnfReldepList;
#endif
typedef struct ModulePackageContainer   DnfModulePackageContainer;
typedef struct _DnfContext              DnfContext;
typedef struct _DnfTransaction          DnfTransaction;
typedef struct _DnfRepoLoader           DnfRepoLoader;
typedef struct _DnfRepo                 DnfRepo;
typedef struct _DnfState                DnfState;
typedef struct _DnfSack                 DnfSack;

/**
 * DnfError:
 * @DNF_ERROR_FAILED:                           Failed with a non-specific error
 * @DNF_ERROR_INTERNAL_ERROR:                   Something horrible happened
 * @DNF_ERROR_CANNOT_GET_LOCK:                  Cannot get lock for action
 * @DNF_ERROR_CANCELLED:                        The action was cancelled
 * @DNF_ERROR_REPO_NOT_AVAILABLE:               The repo is not available
 * @DNF_ERROR_CANNOT_FETCH_SOURCE:              Cannot fetch a software repo
 * @DNF_ERROR_CANNOT_WRITE_REPO_CONFIG:         Cannot write a repo config file
 * @DNF_ERROR_PACKAGE_CONFLICTS:                Package conflict exists
 * @DNF_ERROR_NO_PACKAGES_TO_UPDATE:            No packages to update
 * @DNF_ERROR_PACKAGE_INSTALL_BLOCKED:          Package install was blocked
 * @DNF_ERROR_FILE_NOT_FOUND:                   File was not found
 * @DNF_ERROR_UNFINISHED_TRANSACTION:           An unfinished transaction exists
 * @DNF_ERROR_GPG_SIGNATURE_INVALID:            GPG signature was bad
 * @DNF_ERROR_FILE_INVALID:                     File was invalid or could not be read
 * @DNF_ERROR_REPO_NOT_FOUND:                   Source was not found
 * @DNF_ERROR_FAILED_CONFIG_PARSING:            Configuration could not be read
 * @DNF_ERROR_PACKAGE_NOT_FOUND:                Package was not found
 * @DNF_ERROR_INVALID_ARCHITECTURE:             Invalid architecture
 * @DNF_ERROR_BAD_SELECTOR:                     The selector was in some way bad
 * @DNF_ERROR_NO_SOLUTION:                      No solution was possible
 * @DNF_ERROR_BAD_QUERY:                        The query was in some way bad
 * @DNF_ERROR_NO_CAPABILITY:                    This feature was not available
 * @DNF_ERROR_NO_SPACE:                         Out of disk space
 *
 * The error code.
 **/
typedef enum {
        DNF_ERROR_FAILED                        = 1,    /* Since: 0.1.0 */
        DNF_ERROR_INTERNAL_ERROR                = 4,    /* Since: 0.1.0 */
        DNF_ERROR_CANNOT_GET_LOCK               = 26,   /* Since: 0.1.0 */
        DNF_ERROR_CANCELLED                     = 17,   /* Since: 0.1.0 */
        DNF_ERROR_REPO_NOT_AVAILABLE            = 37,   /* Since: 0.1.0 */
        DNF_ERROR_CANNOT_FETCH_SOURCE           = 64,   /* Since: 0.1.0 */
        DNF_ERROR_CANNOT_WRITE_REPO_CONFIG      = 28,   /* Since: 0.1.0 */
        DNF_ERROR_PACKAGE_CONFLICTS             = 36,   /* Since: 0.1.0 */
        DNF_ERROR_NO_PACKAGES_TO_UPDATE         = 27,   /* Since: 0.1.0 */
        DNF_ERROR_PACKAGE_INSTALL_BLOCKED       = 39,   /* Since: 0.1.0 */
        DNF_ERROR_FILE_NOT_FOUND                = 42,   /* Since: 0.1.0 */
        DNF_ERROR_UNFINISHED_TRANSACTION        = 66,   /* Since: 0.1.0 */
        DNF_ERROR_GPG_SIGNATURE_INVALID         = 30,   /* Since: 0.1.0 */
        DNF_ERROR_FILE_INVALID                  = 38,   /* Since: 0.1.0 */
        DNF_ERROR_REPO_NOT_FOUND                = 19,   /* Since: 0.1.0 */
        DNF_ERROR_FAILED_CONFIG_PARSING         = 24,   /* Since: 0.1.0 */
        DNF_ERROR_PACKAGE_NOT_FOUND             = 8,    /* Since: 0.1.0 */
        DNF_ERROR_NO_SPACE                      = 46,   /* Since: 0.2.3 */
        DNF_ERROR_INVALID_ARCHITECTURE,                 /* Since: 0.7.0 */
        DNF_ERROR_BAD_SELECTOR,                         /* Since: 0.7.0 */
        DNF_ERROR_NO_SOLUTION,                          /* Since: 0.7.0 */
        DNF_ERROR_BAD_QUERY,                            /* Since: 0.7.0 */
        DNF_ERROR_CANNOT_WRITE_CACHE,                   /* Since: 0.7.0 */
        DNF_ERROR_NO_CAPABILITY,                        /* Since: 0.7.0 */
        DNF_ERROR_REMOVAL_OF_PROTECTED_PKG,             /* Since: 0.7.0 */
        /*< private >*/
        DNF_ERROR_LAST
} DnfError;

#define DNF_ERROR                       (dnf_error_quark ())

#ifdef __cplusplus
extern "C" {
#endif
GQuark           dnf_error_quark        (void);
#ifdef __cplusplus
}
#endif

#endif
