/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013 Richard Hughes <richard@hughsie.com>
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

#ifndef __HIF_UTILS_H
#define __HIF_UTILS_H

#include <glib.h>

/**
 * HifError:
 * @HIF_ERROR_FAILED:				Failed with a non-specific error
 * @HIF_ERROR_INTERNAL_ERROR:			Something horrible happened
 * @HIF_ERROR_CANNOT_GET_LOCK:			Cannot get lock for action
 * @HIF_ERROR_CANCELLED:			The action was cancelled
 * @HIF_ERROR_SOURCE_NOT_AVAILABLE:		The source is not available
 * @HIF_ERROR_CANNOT_FETCH_SOURCE:		Cannot fetch a software source
 * @HIF_ERROR_CANNOT_WRITE_SOURCE_CONFIG:	Cannot write a repo config file
 * @HIF_ERROR_PACKAGE_CONFLICTS:		Package conflict exists
 * @HIF_ERROR_NO_PACKAGES_TO_UPDATE:		No packages to update
 * @HIF_ERROR_PACKAGE_INSTALL_BLOCKED:		Package install was blocked
 * @HIF_ERROR_FILE_NOT_FOUND:			File was not found
 * @HIF_ERROR_UNFINISHED_TRANSACTION:		An unfinished transaction exists
 * @HIF_ERROR_GPG_SIGNATURE_INVALID:		GPG signature was bad
 * @HIF_ERROR_FILE_INVALID:			File was invalid or could not be read
 * @HIF_ERROR_SOURCE_NOT_FOUND:			Source was not found
 * @HIF_ERROR_FAILED_CONFIG_PARSING:		Configuration could not be read
 * @HIF_ERROR_PACKAGE_NOT_FOUND:		Package was not found
 *
 * The error code.
 **/
typedef enum {
	HIF_ERROR_FAILED			= 1,	/* Since: 0.1.0 */
	HIF_ERROR_INTERNAL_ERROR		= 4,	/* Since: 0.1.0 */
	HIF_ERROR_CANNOT_GET_LOCK		= 26,	/* Since: 0.1.0 */
	HIF_ERROR_CANCELLED			= 17,	/* Since: 0.1.0 */
	HIF_ERROR_SOURCE_NOT_AVAILABLE		= 37,	/* Since: 0.1.0 */
	HIF_ERROR_CANNOT_FETCH_SOURCE		= 64,	/* Since: 0.1.0 */
	HIF_ERROR_CANNOT_WRITE_SOURCE_CONFIG	= 28,	/* Since: 0.1.0 */
	HIF_ERROR_PACKAGE_CONFLICTS		= 36,	/* Since: 0.1.0 */
	HIF_ERROR_NO_PACKAGES_TO_UPDATE		= 27,	/* Since: 0.1.0 */
	HIF_ERROR_PACKAGE_INSTALL_BLOCKED	= 39,	/* Since: 0.1.0 */
	HIF_ERROR_FILE_NOT_FOUND		= 42,	/* Since: 0.1.0 */
	HIF_ERROR_UNFINISHED_TRANSACTION	= 66,	/* Since: 0.1.0 */
	HIF_ERROR_GPG_SIGNATURE_INVALID		= 30,	/* Since: 0.1.0 */
	HIF_ERROR_FILE_INVALID			= 38,	/* Since: 0.1.0 */
	HIF_ERROR_SOURCE_NOT_FOUND		= 19,	/* Since: 0.1.0 */
	HIF_ERROR_FAILED_CONFIG_PARSING		= 24,	/* Since: 0.1.0 */
	HIF_ERROR_PACKAGE_NOT_FOUND		= 8,	/* Since: 0.1.0 */
	/*< private >*/
	HIF_ERROR_LAST
} HifError;

#define HIF_ERROR				(hif_error_quark ())

GQuark		 hif_error_quark		(void);
gboolean	 hif_rc_to_gerror		(gint			 rc,
						 GError			**error);
gchar		*hif_realpath			(const gchar		*path);

#endif /* __HIF_UTILS_H */
