/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2009-2014 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * Most of this code was taken from Zif, libzif/zif-lock.c
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
 * SECTION:hif-lock
 * @short_description: Lock the package system.
 * @include: libhif-private.h
 * @stability: Unstable
 *
 * This object either works per-thread or per-system with a configured lock file.
 *
 * See also: #HifState
 */

#include "config.h"

#include <gio/gio.h>

#include "hif-cleanup.h"
#include "hif-lock.h"
#include "hif-utils.h"

typedef struct _HifLockPrivate	HifLockPrivate;
struct _HifLockPrivate
{
	GMutex			 mutex;
	GPtrArray		*item_array; /* of HifLockItem */
};

typedef struct {
	gpointer		 owner;
	guint			 id;
	guint			 refcount;
	HifLockMode		 mode;
	HifLockType		 type;
} HifLockItem;

G_DEFINE_TYPE (HifLock, hif_lock, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HIF_TYPE_LOCK, HifLockPrivate))

enum {
	SIGNAL_STATE_CHANGED,
	SIGNAL_LAST
};

static guint signals [SIGNAL_LAST] = { 0 };

static gpointer hif_lock_object = NULL;

/**
 * hif_lock_finalize:
 **/
static void
hif_lock_finalize (GObject *object)
{
	HifLock *lock = HIF_LOCK (object);
	HifLockPrivate *priv = GET_PRIVATE (lock);
	guint i;
	HifLockItem *item;

	/* unlock if we hold the lock */
	for (i = 0; i < priv->item_array->len; i++) {
		item = g_ptr_array_index (priv->item_array, i);
		if (item->refcount > 0) {
			g_warning ("held lock %s at shutdown",
				   hif_lock_type_to_string (item->type));
			hif_lock_release (lock, item->id, NULL);
		}
	}
	g_ptr_array_unref (priv->item_array);

	G_OBJECT_CLASS (hif_lock_parent_class)->finalize (object);
}

/**
 * hif_lock_init:
 **/
static void
hif_lock_init (HifLock *lock)
{
	HifLockPrivate *priv = GET_PRIVATE (lock);
	priv->item_array = g_ptr_array_new_with_free_func (g_free);
}

/**
 * hif_lock_class_init:
 **/
static void
hif_lock_class_init (HifLockClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	signals [SIGNAL_STATE_CHANGED] =
		g_signal_new ("state-changed",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HifLockClass, state_changed),
			      NULL, NULL, g_cclosure_marshal_VOID__UINT,
			      G_TYPE_NONE, 1, G_TYPE_UINT);

	object_class->finalize = hif_lock_finalize;
	g_type_class_add_private (klass, sizeof (HifLockPrivate));
}

/**
 * hif_lock_type_to_string:
 * @lock_type: a #HifLockType.
 *
 * Returns: The string representation of the lock type.
 *
 * Since: 0.1.0
 **/
const gchar *
hif_lock_type_to_string (HifLockType lock_type)
{
	if (lock_type == HIF_LOCK_TYPE_RPMDB)
		return "rpmdb";
	if (lock_type == HIF_LOCK_TYPE_REPO)
		return "repo";
	if (lock_type == HIF_LOCK_TYPE_METADATA)
		return "metadata";
	if (lock_type == HIF_LOCK_TYPE_CONFIG)
		return "config";
	return "unknown";
}

/**
 * hif_lock_mode_to_string:
 **/
static const gchar *
hif_lock_mode_to_string (HifLockMode lock_mode)
{
	if (lock_mode == HIF_LOCK_MODE_THREAD)
		return "thread";
	if (lock_mode == HIF_LOCK_MODE_PROCESS)
		return "process";
	return "unknown";
}

/**
 * hif_lock_get_item_by_type_mode:
 **/
static HifLockItem *
hif_lock_get_item_by_type_mode (HifLock *lock,
				HifLockType type,
				HifLockMode mode)
{
	HifLockItem *item;
	HifLockPrivate *priv = GET_PRIVATE (lock);
	guint i;

	/* search for the item that matches type */
	for (i = 0; i < priv->item_array->len; i++) {
		item = g_ptr_array_index (priv->item_array, i);
		if (item->type == type && item->mode == mode)
			return item;
	}
	return NULL;
}

/**
 * hif_lock_get_item_by_id:
 **/
static HifLockItem *
hif_lock_get_item_by_id (HifLock *lock, guint id)
{
	HifLockItem *item;
	HifLockPrivate *priv = GET_PRIVATE (lock);
	guint i;

	/* search for the item that matches the ID */
	for (i = 0; i < priv->item_array->len; i++) {
		item = g_ptr_array_index (priv->item_array, i);
		if (item->id == id)
			return item;
	}
	return NULL;
}

/**
 * hif_lock_create_item:
 **/
static HifLockItem *
hif_lock_create_item (HifLock *lock, HifLockType type, HifLockMode mode)
{
	HifLockItem *item;
	HifLockPrivate *priv = GET_PRIVATE (lock);
	static guint id = 1;

	item = g_new0 (HifLockItem, 1);
	item->id = id++;
	item->type = type;
	item->owner = g_thread_self ();
	item->refcount = 1;
	item->mode = mode;
	g_ptr_array_add (priv->item_array, item);
	return item;
}

/**
 * hif_lock_get_pid:
 **/
static guint
hif_lock_get_pid (HifLock *lock, const gchar *filename, GError **error)
{
	gboolean ret;
	guint64 pid;
	gchar *endptr = NULL;
	_cleanup_free_error GError *error_local = NULL;
	_cleanup_free gchar *contents = NULL;

	g_return_val_if_fail (HIF_IS_LOCK (lock), FALSE);

	/* file doesn't exists */
	ret = g_file_test (filename, G_FILE_TEST_EXISTS);
	if (!ret) {
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_INTERNAL_ERROR,
				     "lock file not present");
		return 0;
	}

	/* get contents */
	ret = g_file_get_contents (filename, &contents, NULL, &error_local);
	if (!ret) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "lock file not set: %s",
			     error_local->message);
		return 0;
	}

	/* convert to int */
	pid = g_ascii_strtoull (contents, &endptr, 10);

	/* failed to parse */
	if (contents == endptr) {
		g_set_error (error, HIF_ERROR, HIF_ERROR_INTERNAL_ERROR,
			     "failed to parse pid: %s", contents);
		return 0;
	}

	/* too large */
	if (pid > G_MAXUINT) {
		g_set_error (error, HIF_ERROR, HIF_ERROR_INTERNAL_ERROR,
			     "pid too large %" G_GUINT64_FORMAT, pid);
		return 0;
	}
	return (guint) pid;
}

/**
 * hif_lock_get_filename_for_type:
 **/
static gchar *
hif_lock_get_filename_for_type (HifLock *lock,
				HifLockType type,
				GError **error)
{
	gchar *filename = NULL;
	filename = g_strdup_printf ("%s-%s.lock",
				    PIDFILE,
				    hif_lock_type_to_string (type));
	return filename;
}

/**
 * hif_lock_get_cmdline_for_pid:
 **/
static gchar *
hif_lock_get_cmdline_for_pid (guint pid)
{
	gboolean ret;
	_cleanup_free_error GError *error = NULL;
	_cleanup_free gchar *data = NULL;
	_cleanup_free gchar *filename = NULL;

	/* find the cmdline */
	filename = g_strdup_printf ("/proc/%i/cmdline", pid);
	ret = g_file_get_contents (filename, &data, NULL, &error);
	if (ret)
		return g_strdup_printf ("%s (%i)", data, pid);
	g_warning ("failed to get cmdline: %s", error->message);
	return g_strdup_printf ("unknown (%i)", pid);
}

/**
 * hif_lock_get_state:
 * @lock: a #HifLock instance.
 *
 * Gets a bitfield of what locks have been taken
 *
 * Returns: A bitfield.
 *
 * Since: 0.1.0
 **/
guint
hif_lock_get_state (HifLock *lock)
{
	HifLockItem *item;
	HifLockPrivate *priv = GET_PRIVATE (lock);
	guint bitfield = 0;
	guint i;

	g_return_val_if_fail (HIF_IS_LOCK (lock), FALSE);

	for (i = 0; i < priv->item_array->len; i++) {
		item = g_ptr_array_index (priv->item_array, i);
		bitfield += 1 << item->type;
	}
	return bitfield;
}

/**
 * hif_lock_emit_state:
 **/
static void
hif_lock_emit_state (HifLock *lock)
{
	guint bitfield = 0;
	bitfield = hif_lock_get_state (lock);
	g_signal_emit (lock, signals [SIGNAL_STATE_CHANGED], 0, bitfield);
}

/**
 * hif_lock_take:
 * @lock: a #HifLock instance.
 * @type: A #ZifLockType, e.g. %HIF_LOCK_TYPE_RPMDB
 * @mode: A #ZifLockMode, e.g. %HIF_LOCK_MODE_PROCESS
 * @error: A #GError, or %NULL
 *
 * Tries to take a lock for the packaging system.
 *
 * Returns: A lock ID greater than 0, or 0 for an error.
 *
 * Since: 0.1.0
 **/
guint
hif_lock_take (HifLock *lock,
	       HifLockType type,
	       HifLockMode mode,
	       GError **error)
{
	HifLockItem *item;
	HifLockPrivate *priv = GET_PRIVATE (lock);
	gboolean ret;
	guint id = 0;
	guint pid;
	_cleanup_free_error GError *error_local = NULL;
	_cleanup_free gchar *cmdline = NULL;
	_cleanup_free gchar *filename = NULL;
	_cleanup_free gchar *pid_filename = NULL;
	_cleanup_free gchar *pid_text = NULL;

	g_return_val_if_fail (HIF_IS_LOCK (lock), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	/* lock other threads */
	g_mutex_lock (&priv->mutex);

	/* find the lock type, and ensure we find a process lock for
	 * a thread lock */
	item = hif_lock_get_item_by_type_mode (lock, type, mode);
	if (item == NULL && mode == HIF_LOCK_MODE_THREAD) {
		item = hif_lock_get_item_by_type_mode (lock,
						       type,
						       HIF_LOCK_MODE_PROCESS);
	}

	/* create a lock file for process locks */
	if (item == NULL && mode == HIF_LOCK_MODE_PROCESS) {

		/* get the lock filename */
		filename = hif_lock_get_filename_for_type (lock,
							   type,
							   error);
		if (filename == NULL)
			goto out;

		/* does file already exists? */
		if (g_file_test (filename, G_FILE_TEST_EXISTS)) {

			/* check the pid is still valid */
			pid = hif_lock_get_pid (lock, filename, error);
			if (pid == 0)
				goto out;

			/* pid is not still running? */
			pid_filename = g_strdup_printf ("/proc/%i/cmdline",
							pid);
			ret = g_file_test (pid_filename, G_FILE_TEST_EXISTS);
			if (ret) {
				cmdline = hif_lock_get_cmdline_for_pid (pid);
				g_set_error (error,
					     HIF_ERROR,
					     HIF_ERROR_CANNOT_GET_LOCK,
					     "%s[%s] already locked by %s",
					     hif_lock_type_to_string (type),
					     hif_lock_mode_to_string (mode),
					     cmdline);
				goto out;
			}
		}

		/* create file with our process ID */
		pid_text = g_strdup_printf ("%i", getpid ());
		if (!g_file_set_contents (filename, pid_text, -1, &error_local)) {
			g_set_error (error,
				     HIF_ERROR,
				     HIF_ERROR_CANNOT_GET_LOCK,
				     "failed to obtain lock '%s': %s",
				     hif_lock_type_to_string (type),
				     error_local->message);
			goto out;
		}
	}

	/* create new lock */
	if (item == NULL) {
		item = hif_lock_create_item (lock, type, mode);
		id = item->id;
		hif_lock_emit_state (lock);
		goto out;
	}

	/* we're trying to lock something that's already locked
	 * in another thread */
	if (item->owner != g_thread_self ()) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_CANNOT_GET_LOCK,
			     "failed to obtain lock '%s' already taken by thread %p",
			     hif_lock_type_to_string (type),
			     item->owner);
		goto out;
	}

	/* increment ref count */
	item->refcount++;

	/* emit the new locking bitfield */
	hif_lock_emit_state (lock);

	/* success */
	id = item->id;
out:
	/* unlock other threads */
	g_mutex_unlock (&priv->mutex);
	return id;
}

/**
 * hif_lock_release:
 * @lock: a #HifLock instance.
 * @id: A lock ID, as given by zif_lock_take()
 * @error: A #GError, or %NULL
 *
 * Tries to release a lock for the packaging system.
 *
 * Returns: %TRUE if we locked, else %FALSE and the error is set
 *
 * Since: 0.1.0
 **/
gboolean
hif_lock_release (HifLock *lock, guint id, GError **error)
{
	HifLockItem *item;
	HifLockPrivate *priv = GET_PRIVATE (lock);
	gboolean ret = FALSE;
	_cleanup_free_error GError *error_local = NULL;
	_cleanup_free gchar *filename = NULL;
	_cleanup_unref_object GFile *file = NULL;

	g_return_val_if_fail (HIF_IS_LOCK (lock), FALSE);
	g_return_val_if_fail (id != 0, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	/* lock other threads */
	g_mutex_lock (&priv->mutex);

	/* never took */
	item = hif_lock_get_item_by_id (lock, id);
	if (item == NULL) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "Lock was never taken with id %i", id);
		goto out;
	}

	/* not the same thread */
	if (item->owner != g_thread_self ()) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "Lock %s was not taken by this thread",
			     hif_lock_type_to_string (item->type));
		goto out;
	}

	/* idecrement ref count */
	item->refcount--;

	/* delete file for process locks */
	if (item->refcount == 0 &&
	    item->mode == HIF_LOCK_MODE_PROCESS) {

		/* get the lock filename */
		filename = hif_lock_get_filename_for_type (lock,
							   item->type,
							   error);
		if (filename == NULL)
			goto out;

		/* unlink */
		file = g_file_new_for_path (filename);
		ret = g_file_delete (file, NULL, &error_local);
		if (!ret) {
			g_set_error (error,
				     HIF_ERROR,
				     HIF_ERROR_INTERNAL_ERROR,
				     "failed to write: %s",
				     error_local->message);
			goto out;
		}
	}

	/* no thread now owns this lock */
	if (item->refcount == 0)
		g_ptr_array_remove (priv->item_array, item);

	/* emit the new locking bitfield */
	hif_lock_emit_state (lock);

	/* success */
	ret = TRUE;
out:
	/* unlock other threads */
	g_mutex_unlock (&priv->mutex);
	return ret;
}

/**
 * hif_lock_release_noerror:
 * @lock: a #HifLock instance.
 * @id: A lock ID, as given by zif_lock_take()
 *
 * Tries to release a lock for the packaging system. This method
 * should not be used lightly as no error will be returned.
 *
 * Since: 0.1.0
 **/
void
hif_lock_release_noerror (HifLock *lock, guint id)
{
	_cleanup_free_error GError *error = NULL;
	if (!hif_lock_release (lock, id, &error))
		g_warning ("Handled locally: %s", error->message);
}
/**
 * hif_lock_new:
 *
 * Creates a new #HifLock.
 *
 * Returns: (transfer full): a #HifLock
 *
 * Since: 0.1.0
 **/
HifLock *
hif_lock_new (void)
{
	if (hif_lock_object != NULL) {
		g_object_ref (hif_lock_object);
	} else {
		hif_lock_object = g_object_new (HIF_TYPE_LOCK, NULL);
		g_object_add_weak_pointer (hif_lock_object, &hif_lock_object);
	}
	return HIF_LOCK (hif_lock_object);
}
