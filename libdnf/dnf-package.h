/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2013-2015 Richard Hughes <richard@hughsie.com>
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

#ifndef __DNF_PACKAGE_H
#define __DNF_PACKAGE_H

#include "hy-package.h"

#include "dnf-utils.h"
#include "dnf-repo.h"
#include "dnf-state.h"
#include "libdnf/repo/RpmPackage.hpp"

#ifdef __cplusplus
extern "C" {
#endif

DnfRepo *dnf_package_get_repo(DnfPackage *pkg);
void dnf_package_set_repo(DnfPackage *pkg, DnfRepo *repo);
const char *dnf_package_get_filename(DnfPackage *pkg);
DEPRECATED() void dnf_package_set_filename(DnfPackage *pkg, const char *filename);
DEPRECATED() const char *dnf_package_get_origin(DnfPackage *pkg);
DEPRECATED() void dnf_package_set_origin(DnfPackage *pkg, const char *origin);
const char *dnf_package_get_package_id(DnfPackage *pkg);
DnfStateAction dnf_package_get_action(DnfPackage *pkg);
void dnf_package_set_action(DnfPackage *pkg, DnfStateAction action);
const char *dnf_package_get_pkgid(DnfPackage *pkg);
void dnf_package_set_pkgid(DnfPackage *pkg, const char *pkgid);
bool dnf_package_check_filename(DnfPackage *pkg, bool *valid, GError **error);
DEPRECATED() int dnf_package_get_info(DnfPackage *pkg);
DEPRECATED() void dnf_package_set_info(DnfPackage *pkg, int info);
DEPRECATED() bool dnf_package_is_gui(DnfPackage *pkg);
DEPRECATED() bool dnf_package_is_devel(DnfPackage *pkg);
DEPRECATED() bool dnf_package_is_downloaded(DnfPackage *pkg);
DEPRECATED() bool dnf_package_is_installonly(DnfPackage *pkg);
DEPRECATED() unsigned int dnf_package_get_cost(DnfPackage *pkg);
DEPRECATED() char *dnf_package_download(DnfPackage *pkg, const char *directory, DnfState *state, GError **error);

bool dnf_package_array_download(GPtrArray *packages, const char *directory, DnfState *state, GError **error);
unsigned long dnf_package_array_get_download_size(GPtrArray *packages);

#ifdef __cplusplus
};
#endif

#endif /* __DNF_PACKAGE_H */
