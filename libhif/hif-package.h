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

#ifndef __HIF_PACKAGE_H
#define __HIF_PACKAGE_H

#include <glib.h>

#include "hy-package.h"

#include "hif-repo.h"
#include "hif-state.h"

/**
 * HifPackageInfo:
 * @HIF_PACKAGE_INFO_UNKNOWN:                   Unknown state
 * @HIF_PACKAGE_INFO_UPDATE:                    Package update
 * @HIF_PACKAGE_INFO_INSTALL:                   Package install
 * @HIF_PACKAGE_INFO_REMOVE:                    Package remove
 * @HIF_PACKAGE_INFO_CLEANUP:                   Package cleanup
 * @HIF_PACKAGE_INFO_OBSOLETE:                  Package obsolete
 * @HIF_PACKAGE_INFO_REINSTALL:                 Package re-install
 * @HIF_PACKAGE_INFO_DOWNGRADE:                 Package downgrade
 *
 * The info enum code.
 **/
typedef enum {
        HIF_PACKAGE_INFO_UNKNOWN                        = 0,
        HIF_PACKAGE_INFO_UPDATE                         = 11,
        HIF_PACKAGE_INFO_INSTALL                        = 12,
        HIF_PACKAGE_INFO_REMOVE                         = 13,
        HIF_PACKAGE_INFO_CLEANUP                        = 14,
        HIF_PACKAGE_INFO_OBSOLETE                       = 15,
        HIF_PACKAGE_INFO_REINSTALL                      = 19,
        HIF_PACKAGE_INFO_DOWNGRADE                      = 20,
        /*< private >*/
        HIF_PACKAGE_INFO_LAST
} HifPackageInfo;

HifRepo         *hif_package_get_repo                   (HifPackage     *pkg);
void             hif_package_set_repo                   (HifPackage     *pkg,
                                                         HifRepo        *repo);
const gchar     *hif_package_get_filename               (HifPackage     *pkg);
void             hif_package_set_filename               (HifPackage     *pkg,
                                                         const gchar    *filename);
const gchar     *hif_package_get_origin                 (HifPackage     *pkg);
void             hif_package_set_origin                 (HifPackage     *pkg,
                                                         const gchar    *origin);
const gchar     *hif_package_get_package_id             (HifPackage     *pkg);
HifPackageInfo   hif_package_get_info                   (HifPackage     *pkg);
void             hif_package_set_info                   (HifPackage     *pkg,
                                                         HifPackageInfo  info);
HifStateAction   hif_package_get_action                 (HifPackage     *pkg);
void             hif_package_set_action                 (HifPackage     *pkg,
                                                         HifStateAction  action);
gboolean         hif_package_get_user_action            (HifPackage     *pkg);
void             hif_package_set_user_action            (HifPackage     *pkg,
                                                         gboolean        user_action);
gboolean         hif_package_is_gui                     (HifPackage     *pkg);
gboolean         hif_package_is_devel                   (HifPackage     *pkg);
gboolean         hif_package_is_downloaded              (HifPackage     *pkg);
gboolean         hif_package_is_installonly             (HifPackage     *pkg);
const gchar     *hif_package_get_pkgid                  (HifPackage     *pkg);
void             hif_package_set_pkgid                  (HifPackage     *pkg,
                                                         const gchar    *pkgid);
guint            hif_package_get_cost                   (HifPackage     *pkg);
gchar           *hif_package_download                   (HifPackage     *pkg,
                                                         const gchar    *directory,
                                                         HifState       *state,
                                                         GError         **error);
gboolean         hif_package_check_filename             (HifPackage     *pkg,
                                                         gboolean       *valid,
                                                         GError         **error);

gboolean         hif_package_array_download             (GPtrArray      *packages,
                                                         const gchar    *directory,
                                                         HifState       *state,
                                                         GError         **error);

#endif /* __HIF_PACKAGE_H */
