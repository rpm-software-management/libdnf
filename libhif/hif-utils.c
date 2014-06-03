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

/**
 * SECTION:hif-utils
 * @short_description: Helper functions for libhif
 * @include: libhif-private.h
 * @stability: Unstable
 *
 * These functions are used internally in libhif for various things.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <hawkey/errno.h>

#include "hif-cleanup.h"
#include "hif-utils.h"

/**
 * hif_error_quark:
 *
 * Returns a #GQuark for the error domain used in libhif
 *
 * Returns: an error quark
 **/
GQuark
hif_error_quark (void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string ("HifError");
	return quark;
}

/**
 * hif_rc_to_gerror:
 * @rc: A hawkey return code value
 * @error: A #GError, or %NULL
 *
 * Converts the hawkey return code error into a native GError
 *
 * Returns: %FALSE if an error was set
 */
gboolean
hif_rc_to_gerror (gint rc, GError **error)
{
	if (rc == 0)
		return TRUE;
	switch (rc) {
	case HY_E_FAILED:
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_FAILED,
				     "general runtime error");
		break;
	case HY_E_OP:
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_FAILED,
				     "client programming error");
		break;
	case HY_E_LIBSOLV:
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_FAILED,
				     "error propagated from libsolv");
		break;
	case HY_E_IO:
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_FAILED,
				     "I/O error");
		break;
	case HY_E_CACHE_WRITE:
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_FAILED,
				     "cache write error");
		break;
	case HY_E_QUERY:
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_FAILED,
				     "ill-formed query");
		break;
	case HY_E_ARCH:
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_FAILED,
				     "unknown arch");
		break;
	case HY_E_VALIDATION:
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_FAILED,
				     "validation check failed");
		break;
	case HY_E_SELECTOR:
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_FAILED,
				     "ill-specified selector");
		break;
	case HY_E_NO_SOLUTION:
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_FAILED,
				     "goal found no solutions");
		break;
	default:
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_FAILED,
			     "no matching error enum %i", rc);
		break;
	}
	return FALSE;
}
