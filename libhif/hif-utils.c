/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2013-2015 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
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
 * SECTION:hif-utils
 * @short_description: Helper functions for libhif
 * @include: libhif.h
 * @stability: Unstable
 *
 * These functions are used internally in libhif for various things.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <glib/gstdio.h>

#include "hif-types.h"
#include "hif-utils.h"

/**
 * hif_error_quark:
 *
 * Returns a #GQuark for the error domain used in libhif
 *
 * Returns: an error quark
 **/
GQuark
hif_error_quark(void)
{
    static GQuark quark = 0;
    if (!quark)
        quark = g_quark_from_static_string("HifError");
    return quark;
}

/**
 * hif_realpath:
 * @path: A relative path, e.g. "../../data/test"
 *
 * Converts relative paths to absolute ones.
 *
 * Returns: a new path, or %NULL
 *
 * Since: 0.1.7
 **/
gchar *
hif_realpath(const gchar *path)
{
    gchar *real = NULL;
    char *temp;

    /* don't trust realpath one little bit */
    if (path == NULL)
        return NULL;

    /* glibc allocates us a buffer to try and fix some brain damage */
    temp = realpath(path, NULL);
    if (temp == NULL)
        return NULL;
    real = g_strdup(temp);
    free(temp);
    return real;
}

/**
 * hif_remove_recursive:
 * @directory: A directory path
 * @error: A #GError, or %NULL
 *
 * Removes a directory and its contents. Use with care.
 *
 * Returns: %FALSE if an error was set
 *
 * Since: 0.1.7
 **/
gboolean
hif_remove_recursive(const gchar *directory, GError **error)
{
    const gchar *filename;
    g_autoptr(GDir) dir;
    g_autoptr(GError) error_local = NULL;

    /* try to open */
    dir = g_dir_open(directory, 0, &error_local);
    if (dir == NULL) {
        g_set_error(error,
                    HIF_ERROR,
                    HIF_ERROR_INTERNAL_ERROR,
                    "cannot open directory %s: %s",
                    directory, error_local->message);
        return FALSE;
    }

    /* find each */
    while ((filename = g_dir_read_name(dir))) {
        g_autofree gchar *src = NULL;
        src = g_build_filename(directory, filename, NULL);
        if (g_file_test(src, G_FILE_TEST_IS_DIR)) {
            if (!hif_remove_recursive(src, error))
                return FALSE;
        } else {
            g_debug("deleting file %s", src);
            if (g_unlink(src) != 0) {
                g_set_error(error,
                            HIF_ERROR,
                            HIF_ERROR_INTERNAL_ERROR,
                            "failed to unlink %s", src);
                return FALSE;
            }
        }
    }

    /* remove directory */
    g_debug("deleting directory %s", directory);
    if (g_remove(directory) != 0) {
        g_set_error(error,
                    HIF_ERROR,
                    HIF_ERROR_INTERNAL_ERROR,
                    "failed to remove %s", directory);
        return FALSE;
    }
    return TRUE;
}
