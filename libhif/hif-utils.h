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
 *
 * The error code.
 **/
typedef enum {
	HIF_ERROR_FAILED,			/* Since: 0.1.0 */
	HIF_ERROR_INTERNAL_ERROR,		/* Since: 0.1.0 */
	HIF_ERROR_CANNOT_GET_LOCK,		/* Since: 0.1.0 */
	HIF_ERROR_CANCELLED,			/* Since: 0.1.0 */
	HIF_ERROR_SOURCE_NOT_AVAILABLE,		/* Since: 0.1.0 */
	HIF_ERROR_CANNOT_FETCH_SOURCE,		/* Since: 0.1.0 */
	HIF_ERROR_CANNOT_WRITE_SOURCE_CONFIG,	/* Since: 0.1.0 */
	HIF_ERROR_PACKAGE_CONFLICTS,		/* Since: 0.1.0 */
	HIF_ERROR_NO_PACKAGES_TO_UPDATE,	/* Since: 0.1.0 */
	HIF_ERROR_PACKAGE_INSTALL_BLOCKED,	/* Since: 0.1.0 */
	HIF_ERROR_FILE_NOT_FOUND,		/* Since: 0.1.0 */
	HIF_ERROR_UNFINISHED_TRANSACTION,	/* Since: 0.1.0 */
	/*< private >*/
	HIF_ERROR_LAST
} HifError;

#define HIF_ERROR				(hif_error_quark ())

GQuark		 hif_error_quark		(void);
gboolean	 hif_rc_to_gerror		(gint			 rc,
						 GError			**error);

#endif /* __HIF_UTILS_H */
