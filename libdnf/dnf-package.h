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

#include <glib.h>

#include "hy-package.h"

#include "dnf-repo.h"
#include "dnf-state.h"

G_BEGIN_DECLS

/**
 * DnfPackageInfo:
 * @DNF_PACKAGE_INFO_UNKNOWN:                   Unknown state
 * @DNF_PACKAGE_INFO_UPDATE:                    Package update
 * @DNF_PACKAGE_INFO_INSTALL:                   Package install
 * @DNF_PACKAGE_INFO_REMOVE:                    Package remove
 * @DNF_PACKAGE_INFO_CLEANUP:                   Package cleanup
 * @DNF_PACKAGE_INFO_OBSOLETE:                  Package obsolete
 * @DNF_PACKAGE_INFO_REINSTALL:                 Package re-install
 * @DNF_PACKAGE_INFO_DOWNGRADE:                 Package downgrade
 *
 * The info enum code.
 **/
typedef enum {
        DNF_PACKAGE_INFO_UNKNOWN                        = 0,
        DNF_PACKAGE_INFO_UPDATE                         = 11,
        DNF_PACKAGE_INFO_INSTALL                        = 12,
        DNF_PACKAGE_INFO_REMOVE                         = 13,
        DNF_PACKAGE_INFO_CLEANUP                        = 14,
        DNF_PACKAGE_INFO_OBSOLETE                       = 15,
        DNF_PACKAGE_INFO_REINSTALL                      = 19,
        DNF_PACKAGE_INFO_DOWNGRADE                      = 20,
        /*< private >*/
        DNF_PACKAGE_INFO_LAST
} DnfPackageInfo;

DnfRepo         *dnf_package_get_repo                   (DnfPackage     *pkg);
void             dnf_package_set_repo                   (DnfPackage     *pkg,
                                                         DnfRepo        *repo);
const gchar     *dnf_package_get_filename               (DnfPackage     *pkg);
void             dnf_package_set_filename               (DnfPackage     *pkg,
                                                         const gchar    *filename);
const gchar     *dnf_package_get_origin                 (DnfPackage     *pkg);
void             dnf_package_set_origin                 (DnfPackage     *pkg,
                                                         const gchar    *origin);
const gchar     *dnf_package_get_package_id             (DnfPackage     *pkg);
DnfPackageInfo   dnf_package_get_info                   (DnfPackage     *pkg);
void             dnf_package_set_info                   (DnfPackage     *pkg,
                                                         DnfPackageInfo  info);
DnfStateAction   dnf_package_get_action                 (DnfPackage     *pkg);
void             dnf_package_set_action                 (DnfPackage     *pkg,
                                                         DnfStateAction  action);
gboolean         dnf_package_get_user_action            (DnfPackage     *pkg);
void             dnf_package_set_user_action            (DnfPackage     *pkg,
                                                         gboolean        user_action);
gboolean         dnf_package_is_gui                     (DnfPackage     *pkg);
gboolean         dnf_package_is_devel                   (DnfPackage     *pkg);
gboolean         dnf_package_is_local                   (DnfPackage     *pkg);
gchar           *dnf_package_get_local_baseurl          (DnfPackage     *pkg,
                                                         GError         **error);
gboolean         dnf_package_is_downloaded              (DnfPackage     *pkg);
gboolean         dnf_package_is_installonly             (DnfPackage     *pkg);
const gchar     *dnf_package_get_pkgid                  (DnfPackage     *pkg);
void             dnf_package_set_pkgid                  (DnfPackage     *pkg,
                                                         const gchar    *pkgid);
guint            dnf_package_get_cost                   (DnfPackage     *pkg);
gchar           *dnf_package_download                   (DnfPackage     *pkg,
                                                         const gchar    *directory,
                                                         DnfState       *state,
                                                         GError         **error);
gboolean         dnf_package_check_filename             (DnfPackage     *pkg,
                                                         gboolean       *valid,
                                                         GError         **error);

gboolean         dnf_package_array_download             (GPtrArray      *packages,
                                                         const gchar    *directory,
                                                         DnfState       *state,
                                                         GError         **error);
guint64          dnf_package_array_get_download_size    (GPtrArray      *packages);

G_END_DECLS

#endif /* __DNF_PACKAGE_H */
