/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2009-2015 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * Most of this code was taken from Zif, libzif/zif-lock.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or(at your option) any later version.
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
 * SECTION:dnf-lock
 * @short_description: Lock the package system.
 * @include: libdnf.h
 * @stability: Unstable
 *
 * This object either works per-thread or per-system with a configured lock file.
 *
 * See also: #DnfState
 */


#include <gio/gio.h>

#include "catch-error.hpp"
#include "dnf-lock.h"
#include "dnf-types.h"
#include "dnf-utils.h"

typedef struct
{
    GMutex          mutex;
    GPtrArray      *item_array; /* of DnfLockItem */
    gchar          *lock_dir;
} DnfLockPrivate;

typedef struct {
    gpointer            owner;
    guint               id;
    guint               refcount;
    DnfLockMode         mode;
    DnfLockType         type;
} DnfLockItem;

G_DEFINE_TYPE_WITH_PRIVATE(DnfLock, dnf_lock, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (static_cast<DnfLockPrivate *>(dnf_lock_get_instance_private (o)))

enum {
    SIGNAL_STATE_CHANGED,
    SIGNAL_LAST
};

static guint signals [SIGNAL_LAST] = { 0 };

static gpointer dnf_lock_object = NULL;

/**
 * dnf_lock_finalize:
 **/
static void
dnf_lock_finalize(GObject *object)
{
    DnfLock *lock = DNF_LOCK(object);
    DnfLockPrivate *priv = GET_PRIVATE(lock);
    guint i;

    /* unlock if we hold the lock */
    for (i = 0; i < priv->item_array->len; i++) {
        auto item = static_cast<DnfLockItem *>(g_ptr_array_index(priv->item_array, i));
        if (item->refcount > 0) {
            g_warning("held lock %s at shutdown",
                      dnf_lock_type_to_string(item->type));
            dnf_lock_release(lock, item->id, NULL);
        }
    }
    g_ptr_array_unref(priv->item_array);
    g_free(priv->lock_dir);

    G_OBJECT_CLASS(dnf_lock_parent_class)->finalize(object);
}

/**
 * dnf_lock_init:
 **/
static void
dnf_lock_init(DnfLock *lock)
{
    DnfLockPrivate *priv = GET_PRIVATE(lock);
    priv->item_array = g_ptr_array_new_with_free_func(g_free);
    priv->lock_dir = g_strdup("/var/run");
}

/**
 * dnf_lock_class_init:
 **/
static void
dnf_lock_class_init(DnfLockClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    signals [SIGNAL_STATE_CHANGED] =
        g_signal_new("state-changed",
                     G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(DnfLockClass, state_changed),
                     NULL, NULL, g_cclosure_marshal_VOID__UINT,
                     G_TYPE_NONE, 1, G_TYPE_UINT);

    object_class->finalize = dnf_lock_finalize;
}

/**
 * dnf_lock_type_to_string:
 * @lock_type: a #DnfLockType.
 *
 * Returns: The string representation of the lock type.
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_lock_type_to_string(DnfLockType lock_type)
{
    if (lock_type == DNF_LOCK_TYPE_RPMDB)
        return "rpmdb";
    if (lock_type == DNF_LOCK_TYPE_REPO)
        return "src";
    if (lock_type == DNF_LOCK_TYPE_METADATA)
        return "metadata";
    if (lock_type == DNF_LOCK_TYPE_CONFIG)
        return "config";
    return "unknown";
}

/**
 * dnf_lock_mode_to_string:
 **/
static const gchar *
dnf_lock_mode_to_string(DnfLockMode lock_mode)
{
    if (lock_mode == DNF_LOCK_MODE_THREAD)
        return "thread";
    if (lock_mode == DNF_LOCK_MODE_PROCESS)
        return "process";
    return "unknown";
}

/**
 * dnf_lock_get_item_by_type_mode:
 **/
static DnfLockItem *
dnf_lock_get_item_by_type_mode(DnfLock *lock,
                DnfLockType type,
                DnfLockMode mode)
{
    DnfLockPrivate *priv = GET_PRIVATE(lock);
    guint i;

    /* search for the item that matches type */
    for (i = 0; i < priv->item_array->len; i++) {
        auto item = static_cast<DnfLockItem *>(g_ptr_array_index(priv->item_array, i));
        if (item->type == type && item->mode == mode)
            return item;
    }
    return NULL;
}

/**
 * dnf_lock_get_item_by_id:
 **/
static DnfLockItem *
dnf_lock_get_item_by_id(DnfLock *lock, guint id)
{
    DnfLockPrivate *priv = GET_PRIVATE(lock);
    guint i;

    /* search for the item that matches the ID */
    for (i = 0; i < priv->item_array->len; i++) {
        auto item = static_cast<DnfLockItem *>(g_ptr_array_index(priv->item_array, i));
        if (item->id == id)
            return item;
    }
    return NULL;
}

/**
 * dnf_lock_create_item:
 **/
static DnfLockItem *
dnf_lock_create_item(DnfLock *lock, DnfLockType type, DnfLockMode mode)
{
    DnfLockItem *item;
    DnfLockPrivate *priv = GET_PRIVATE(lock);
    static guint id = 1;

    item = g_new0(DnfLockItem, 1);
    item->id = id++;
    item->type = type;
    item->owner = g_thread_self();
    item->refcount = 1;
    item->mode = mode;
    g_ptr_array_add(priv->item_array, item);
    return item;
}

/**
 * dnf_lock_get_pid:
 **/
static guint
dnf_lock_get_pid(DnfLock *lock, const gchar *filename, GError **error)
{
    gboolean ret;
    guint64 pid;
    gchar *endptr = NULL;
    g_autoptr(GError) error_local = NULL;
    g_autofree gchar *contents = NULL;

    g_return_val_if_fail(DNF_IS_LOCK(lock), FALSE);

    /* file doesn't exists */
    ret = g_file_test(filename, G_FILE_TEST_EXISTS);
    if (!ret) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_INTERNAL_ERROR,
                            "lock file not present");
        return 0;
    }

    /* get contents */
    ret = g_file_get_contents(filename, &contents, NULL, &error_local);
    if (!ret) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "lock file not set: %s",
                    error_local->message);
        return 0;
    }

    /* convert to int */
    pid = g_ascii_strtoull(contents, &endptr, 10);

    /* failed to parse */
    if (contents == endptr) {
        g_set_error(error, DNF_ERROR, DNF_ERROR_INTERNAL_ERROR,
                    "failed to parse pid: %s", contents);
        return 0;
    }

    /* too large */
    if (pid > G_MAXUINT) {
        g_set_error(error, DNF_ERROR, DNF_ERROR_INTERNAL_ERROR,
                    "pid too large %" G_GUINT64_FORMAT, pid);
        return 0;
    }
    return(guint) pid;
}

/**
 * dnf_lock_get_filename_for_type:
 **/
static gchar *
dnf_lock_get_filename_for_type(DnfLock *lock, DnfLockType type)
{
    DnfLockPrivate *priv = GET_PRIVATE(lock);
    return g_strdup_printf("%s/dnf-%s.lock",
                           priv->lock_dir,
                           dnf_lock_type_to_string(type));
}

/**
 * dnf_lock_get_cmdline_for_pid:
 **/
static gchar *
dnf_lock_get_cmdline_for_pid(guint pid)
{
    gboolean ret;
    g_autoptr(GError) error = NULL;
    g_autofree gchar *data = NULL;
    g_autofree gchar *filename = NULL;

    /* find the cmdline */
    filename = g_strdup_printf("/proc/%i/cmdline", pid);
    ret = g_file_get_contents(filename, &data, NULL, &error);
    if (ret)
        return g_strdup_printf("%s(%i)", data, pid);
    g_warning("failed to get cmdline: %s", error->message);
    return g_strdup_printf("unknown(%i)", pid);
}

/**
 * dnf_lock_set_lock_dir:
 * @lock: a #DnfLock instance.
 * @lock_dir: the directory to use for lock files
 *
 * Sets the directory to use for lock files.
 *
 * Since: 0.1.4
 **/
void
dnf_lock_set_lock_dir(DnfLock *lock, const gchar *lock_dir)
{
    DnfLockPrivate *priv = GET_PRIVATE(lock);
    g_return_if_fail(DNF_IS_LOCK(lock));
    g_free(priv->lock_dir);
    priv->lock_dir = g_strdup(lock_dir);
}

/**
 * dnf_lock_get_state:
 * @lock: a #DnfLock instance.
 *
 * Gets a bitfield of what locks have been taken
 *
 * Returns: A bitfield.
 *
 * Since: 0.1.0
 **/
guint
dnf_lock_get_state(DnfLock *lock)
{
    DnfLockPrivate *priv = GET_PRIVATE(lock);
    guint bitfield = 0;
    guint i;

    g_return_val_if_fail(DNF_IS_LOCK(lock), FALSE);

    for (i = 0; i < priv->item_array->len; i++) {
        auto item = static_cast<DnfLockItem *>(g_ptr_array_index(priv->item_array, i));
        bitfield += 1 << item->type;
    }
    return bitfield;
}

/**
 * dnf_lock_emit_state:
 **/
static void
dnf_lock_emit_state(DnfLock *lock)
{
    guint bitfield = 0;
    bitfield = dnf_lock_get_state(lock);
    g_signal_emit(lock, signals [SIGNAL_STATE_CHANGED], 0, bitfield);
}

/**
 * dnf_lock_take:
 * @lock: a #DnfLock instance.
 * @type: A #ZifLockType, e.g. %DNF_LOCK_TYPE_RPMDB
 * @mode: A #ZifLockMode, e.g. %DNF_LOCK_MODE_PROCESS
 * @error: A #GError, or %NULL
 *
 * Tries to take a lock for the packaging system.
 *
 * Returns: A lock ID greater than 0, or 0 for an error.
 *
 * Since: 0.1.0
 **/
guint
dnf_lock_take(DnfLock *lock,
              DnfLockType type,
              DnfLockMode mode,
              GError **error) try
{
    DnfLockItem *item;
    DnfLockPrivate *priv = GET_PRIVATE(lock);
    gboolean ret;
    guint id = 0;
    guint pid;
    g_autoptr(GError) error_local = NULL;
    g_autofree gchar *cmdline = NULL;
    g_autofree gchar *filename = NULL;
    g_autofree gchar *pid_filename = NULL;
    g_autofree gchar *pid_text = NULL;

    g_return_val_if_fail(DNF_IS_LOCK(lock), FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    /* lock other threads */
    g_mutex_lock(&priv->mutex);

    /* find the lock type, and ensure we find a process lock for
     * a thread lock */
    item = dnf_lock_get_item_by_type_mode(lock, type, mode);
    if (item == NULL && mode == DNF_LOCK_MODE_THREAD) {
        item = dnf_lock_get_item_by_type_mode(lock,
                                              type,
                                              DNF_LOCK_MODE_PROCESS);
    }

    /* create a lock file for process locks */
    if (item == NULL && mode == DNF_LOCK_MODE_PROCESS) {

        /* does lock file already exists? */
        filename = dnf_lock_get_filename_for_type(lock, type);
        if (g_file_test(filename, G_FILE_TEST_EXISTS)) {

            /* check the pid is still valid */
            pid = dnf_lock_get_pid(lock, filename, error);
            if (pid == 0)
                goto out;

            /* pid is not still running? */
            pid_filename = g_strdup_printf("/proc/%i/cmdline", pid);
            ret = g_file_test(pid_filename, G_FILE_TEST_EXISTS);
            if (ret) {
                cmdline = dnf_lock_get_cmdline_for_pid(pid);
                g_set_error(error,
                            DNF_ERROR,
                            DNF_ERROR_CANNOT_GET_LOCK,
                            "%s[%s] already locked by %s",
                            dnf_lock_type_to_string(type),
                            dnf_lock_mode_to_string(mode),
                            cmdline);
                goto out;
            }
        }

        /* create file with our process ID */
        pid_text = g_strdup_printf("%i", getpid());
        if (!g_file_set_contents(filename, pid_text, -1, &error_local)) {
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_CANNOT_GET_LOCK,
                        "failed to obtain lock '%s': %s",
                        dnf_lock_type_to_string(type),
                        error_local->message);
            goto out;
        }
    }

    /* create new lock */
    if (item == NULL) {
        item = dnf_lock_create_item(lock, type, mode);
        id = item->id;
        dnf_lock_emit_state(lock);
        goto out;
    }

    /* we're trying to lock something that's already locked
     * in another thread */
    if (item->owner != g_thread_self()) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_CANNOT_GET_LOCK,
                    "failed to obtain lock '%s' already taken by thread %p",
                    dnf_lock_type_to_string(type),
                    item->owner);
        goto out;
    }

    /* increment ref count */
    item->refcount++;

    /* emit the new locking bitfield */
    dnf_lock_emit_state(lock);

    /* success */
    id = item->id;
out:
    /* unlock other threads */
    g_mutex_unlock(&priv->mutex);
    return id;
} CATCH_TO_GERROR(0)

/**
 * dnf_lock_release:
 * @lock: a #DnfLock instance.
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
dnf_lock_release(DnfLock *lock, guint id, GError **error) try
{
    DnfLockItem *item;
    DnfLockPrivate *priv = GET_PRIVATE(lock);
    gboolean ret = FALSE;

    g_assert(DNF_IS_LOCK(lock));
    g_assert(id != 0);
    g_assert(error == NULL || *error == NULL);

    /* lock other threads */
    g_mutex_lock(&priv->mutex);

    /* never took */
    item = dnf_lock_get_item_by_id(lock, id);
    if (item == NULL) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "Lock was never taken with id %i", id);
        goto out;
    }

    /* not the same thread */
    if (item->owner != g_thread_self()) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "Lock %s was not taken by this thread",
                    dnf_lock_type_to_string(item->type));
        goto out;
    }

    /* decrement ref count */
    item->refcount--;

    /* delete file for process locks */
    if (item->refcount == 0 &&
        item->mode == DNF_LOCK_MODE_PROCESS) {
        g_autoptr(GError) error_local = NULL;
        g_autofree gchar *filename = NULL;
        g_autoptr(GFile) file = NULL;

        /* unlink */
        filename = dnf_lock_get_filename_for_type(lock, item->type);
        file = g_file_new_for_path(filename);
        ret = g_file_delete(file, NULL, &error_local);
        if (!ret) {
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_INTERNAL_ERROR,
                        "failed to write: %s",
                        error_local->message);
            goto out;
        }
    }

    /* no thread now owns this lock */
    if (item->refcount == 0)
        g_ptr_array_remove(priv->item_array, item);

    /* emit the new locking bitfield */
    dnf_lock_emit_state(lock);

    /* success */
    ret = TRUE;
out:
    /* unlock other threads */
    g_mutex_unlock(&priv->mutex);
    return ret;
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_lock_release_noerror:
 * @lock: a #DnfLock instance.
 * @id: A lock ID, as given by zif_lock_take()
 *
 * Tries to release a lock for the packaging system. This method
 * should not be used lightly as no error will be returned.
 *
 * Since: 0.1.0
 **/
void
dnf_lock_release_noerror(DnfLock *lock, guint id)
{
    g_autoptr(GError) error = NULL;
    if (!dnf_lock_release(lock, id, &error))
        g_warning("Handled locally: %s", error->message);
}
/**
 * dnf_lock_new:
 *
 * Creates a new #DnfLock.
 *
 * Returns:(transfer full): a #DnfLock
 *
 * Since: 0.1.0
 **/
DnfLock *
dnf_lock_new(void)
{
    if (dnf_lock_object != NULL) {
        g_object_ref(dnf_lock_object);
    } else {
        dnf_lock_object = g_object_new(DNF_TYPE_LOCK, NULL);
        g_object_add_weak_pointer(static_cast<GObject *>(dnf_lock_object), &dnf_lock_object);
    }
    return DNF_LOCK(dnf_lock_object);
}
