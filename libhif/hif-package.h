/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013-2014 Richard Hughes <richard@hughsie.com>
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

#if !defined (__LIBHIF_H) && !defined (HIF_COMPILATION)
#error "Only <libhif.h> can be included directly."
#endif

#ifndef __HIF_PACKAGE_H
#define __HIF_PACKAGE_H

#include <glib.h>

#include <hawkey/package.h>

#include "hif-source.h"

/**
 * HifPackageInfo:
 * @HIF_PACKAGE_INFO_UNKNOWN:			Unknown state
 * @HIF_PACKAGE_INFO_UPDATE:			Package update
 * @HIF_PACKAGE_INFO_INSTALL:			Package install
 * @HIF_PACKAGE_INFO_REMOVE:			Package remove
 * @HIF_PACKAGE_INFO_CLEANUP:			Package cleanup
 * @HIF_PACKAGE_INFO_OBSOLETE:			Package obsolete
 * @HIF_PACKAGE_INFO_REINSTALL:			Package re-install
 * @HIF_PACKAGE_INFO_DOWNGRADE:			Package downgrade
 *
 * The info enum code.
 **/
typedef enum {
	HIF_PACKAGE_INFO_UNKNOWN			= 0,
	HIF_PACKAGE_INFO_UPDATE				= 11,
	HIF_PACKAGE_INFO_INSTALL			= 12,
	HIF_PACKAGE_INFO_REMOVE				= 13,
	HIF_PACKAGE_INFO_CLEANUP			= 14,
	HIF_PACKAGE_INFO_OBSOLETE			= 15,
	HIF_PACKAGE_INFO_REINSTALL			= 19,
	HIF_PACKAGE_INFO_DOWNGRADE			= 20,
	/*< private >*/
	HIF_PACKAGE_INFO_LAST
} HifPackageInfo;

HifSource	*hif_package_get_source			(HyPackage	 pkg);
void		 hif_package_set_source			(HyPackage	 pkg,
							 HifSource	*src);
const gchar	*hif_package_get_filename		(HyPackage	 pkg);
void		 hif_package_set_filename		(HyPackage	 pkg,
							 const gchar	*filename);
const gchar	*hif_package_get_origin			(HyPackage	 pkg);
void		 hif_package_set_origin			(HyPackage	 pkg,
							 const gchar	*origin);
const gchar	*hif_package_get_id			(HyPackage	 pkg);
HifPackageInfo	 hif_package_get_info			(HyPackage	 pkg);
void		 hif_package_set_info			(HyPackage	 pkg,
							 HifPackageInfo	 info);
HifPackageInfo	 hif_package_get_action			(HyPackage	 pkg);
void		 hif_package_set_action			(HyPackage	 pkg,
							 HifPackageInfo	 action);
gboolean	 hif_package_get_user_action		(HyPackage	 pkg);
void		 hif_package_set_user_action		(HyPackage	 pkg,
							 gboolean	 user_action);
gboolean	 hif_package_is_gui			(HyPackage	 pkg);
gboolean	 hif_package_is_devel			(HyPackage	 pkg);
gboolean	 hif_package_is_downloaded		(HyPackage	 pkg);
gboolean	 hif_package_is_installonly		(HyPackage	 pkg);
const gchar	*hif_package_get_pkgid			(HyPackage	 pkg);
const gchar	*hif_package_get_nevra			(HyPackage	 pkg);
const gchar	*hif_package_get_description		(HyPackage	 pkg);
guint		 hif_package_get_cost			(HyPackage	 pkg);
gchar		*hif_package_download			(HyPackage	 pkg,
							 const gchar	*directory,
							 HifState	*state,
							 GError		**error);
gboolean	 hif_package_check_filename		(HyPackage	 pkg,
							 gboolean	*valid,
							 GError		**error);

gboolean	 hif_package_array_download		(GPtrArray	*packages,
							 const gchar	*directory,
							 HifState	*state,
							 GError		**error);

#endif /* __HIF_PACKAGE_H */
