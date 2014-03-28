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

#include "hif-lock.h"
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

static guint _hif_lock_state_changed = 0;

static void
hif_lock_state_changed_cb (HifLock *lock, guint bitfield, gpointer user_data)
{
	g_debug ("lock state now %i", bitfield);
	_hif_lock_state_changed++;
}

static void
hif_lock_func (void)
{
	HifLock *lock;
	gboolean ret;
	guint lock_id1;
	guint lock_id2;
	GError *error = NULL;

	lock = hif_lock_new ();
	g_assert (lock != NULL);
	g_signal_connect (lock, "state-changed",
			  G_CALLBACK (hif_lock_state_changed_cb), NULL);

	/* nothing yet! */
	g_assert_cmpint (hif_lock_get_state (lock), ==, 0);
	ret = hif_lock_release (lock, 999, &error);
	g_assert_error (error, HIF_ERROR, HIF_ERROR_INTERNAL_ERROR);
	g_assert (!ret);
	g_clear_error (&error);

	/* take one */
	lock_id1 = hif_lock_take (lock,
				  HIF_LOCK_TYPE_RPMDB,
				  HIF_LOCK_MODE_PROCESS,
				  &error);
	g_assert_no_error (error);
	g_assert (lock_id1 != 0);
	g_assert_cmpint (hif_lock_get_state (lock), ==, 1 << HIF_LOCK_TYPE_RPMDB);
	g_assert_cmpint (_hif_lock_state_changed, ==, 1);

	/* take a different one */
	lock_id2 = hif_lock_take (lock,
				  HIF_LOCK_TYPE_REPO,
				  HIF_LOCK_MODE_PROCESS,
				  &error);
	g_assert_no_error (error);
	g_assert (lock_id2 != 0);
	g_assert (lock_id2 != lock_id1);
	g_assert_cmpint (hif_lock_get_state (lock), ==, 1 << HIF_LOCK_TYPE_RPMDB | 1 << HIF_LOCK_TYPE_REPO);
	g_assert_cmpint (_hif_lock_state_changed, ==, 2);

	/* take two */
	lock_id1 = hif_lock_take (lock,
				  HIF_LOCK_TYPE_RPMDB,
				  HIF_LOCK_MODE_PROCESS,
				  &error);
	g_assert_no_error (error);
	g_assert (lock_id1 != 0);
	g_assert_cmpint (hif_lock_get_state (lock), ==, 1 << HIF_LOCK_TYPE_RPMDB | 1 << HIF_LOCK_TYPE_REPO);

	/* release one */
	ret = hif_lock_release (lock, lock_id1, &error);
	g_assert_no_error (error);
	g_assert (ret);

	/* release different one */
	ret = hif_lock_release (lock, lock_id2, &error);
	g_assert_no_error (error);
	g_assert (ret);

	/* release two */
	ret = hif_lock_release (lock, lock_id1, &error);
	g_assert_no_error (error);
	g_assert (ret);

	/* no more! */
	ret = hif_lock_release (lock, lock_id1, &error);
	g_assert_error (error, HIF_ERROR, HIF_ERROR_INTERNAL_ERROR);
	g_assert (!ret);
	g_clear_error (&error);
	g_assert_cmpint (hif_lock_get_state (lock), ==, 0);
	g_assert_cmpint (_hif_lock_state_changed, ==, 6);

	g_object_unref (lock);
}

static gpointer
hif_self_test_lock_thread_one (gpointer data)
{
	GError *error = NULL;
	guint lock_id;
	HifLock *lock = HIF_LOCK (data);

	g_usleep (G_USEC_PER_SEC / 100);
	lock_id = hif_lock_take (lock,
				 HIF_LOCK_TYPE_REPO,
				 HIF_LOCK_MODE_PROCESS,
				 &error);
	g_assert_error (error, HIF_ERROR, HIF_ERROR_CANNOT_GET_LOCK);
	g_assert_cmpint (lock_id, ==, 0);
	g_error_free (error);
	return NULL;
}

static void
hif_lock_threads_func (void)
{
	gboolean ret;
	GError *error = NULL;
	GThread *one;
	guint lock_id;
	HifLock *lock;

	/* take in master thread */
	lock = hif_lock_new ();
	lock_id = hif_lock_take (lock,
				 HIF_LOCK_TYPE_REPO,
				 HIF_LOCK_MODE_PROCESS,
				 &error);
	g_assert_no_error (error);
	g_assert_cmpint (lock_id, >, 0);

	/* attempt to take in slave thread (should fail) */
	one = g_thread_new ("hif-lock-one",
			    hif_self_test_lock_thread_one,
			    lock);

	/* block, waiting for thread */
	g_usleep (G_USEC_PER_SEC);

	/* release lock */
	ret = hif_lock_release (lock, lock_id, &error);
	g_assert_no_error (error);
	g_assert (ret);

	g_thread_unref (one);
	g_object_unref (lock);
}

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
	g_test_add_func ("/libhif/lock", hif_lock_func);
	g_test_add_func ("/libhif/lock[threads]", hif_lock_threads_func);
	g_test_add_func ("/libhif/source", ch_test_source_func);
	g_test_add_func ("/libhif/utils", hif_utils_func);

	return g_test_run ();
}

