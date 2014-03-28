/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014 Richard Hughes <richard@hughsie.com>
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

#include "config.h"

#include <glib-object.h>
#include <hawkey/errno.h>

#include "hif-source.h"
#include "hif-utils.h"

#if 0
/**
 * cd_test_get_filename:
 **/
static gchar *
hif_test_get_filename (const gchar *filename)
{
	char full_tmp[PATH_MAX];
	gchar *full = NULL;
	gchar *path;
	gchar *tmp;

	path = g_build_filename (TESTDATADIR, filename, NULL);
	tmp = realpath (path, full_tmp);
	if (tmp == NULL)
		goto out;
	full = g_strdup (full_tmp);
out:
	g_free (path);
	return full;
}
#endif

static void
ch_test_source_func (void)
{
	HifSource *source;
	source = hif_source_new ();
	g_object_unref (source);
}

static void
hif_utils_func (void)
{
	GError *error = NULL;
	gboolean ret;

	/* success */
	ret = hif_rc_to_gerror (0, &error);
	g_assert_no_error (error);
	g_assert (ret);

	/* failure */
	ret = hif_rc_to_gerror (HY_E_LIBSOLV, &error);
	g_assert_error (error, HIF_ERROR, HIF_ERROR_FAILED);
	g_assert (!ret);
	g_clear_error (&error);

	/* new error enum */
	ret = hif_rc_to_gerror (999, &error);
	g_assert_error (error, HIF_ERROR, HIF_ERROR_FAILED);
	g_assert (!ret);
	g_clear_error (&error);
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	/* only critical and error are fatal */
	g_log_set_fatal_mask (NULL, G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL);

	/* tests go here */
	g_test_add_func ("/libhif/source", ch_test_source_func);
	g_test_add_func ("/libhif/utils", hif_utils_func);

	return g_test_run ();
}

