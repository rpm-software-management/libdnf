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
 * SECTION:dnf-utils
 * @short_description: Helper functions for libdnf
 * @include: libdnf.h
 * @stability: Unstable
 *
 * These functions are used internally in libdnf for various things.
 */


#include <stdlib.h>
#include <glib/gstdio.h>

#include "dnf-types.h"
#include "dnf-utils.h"

/**
 * dnf_error_quark:
 *
 * Returns a #GQuark for the error domain used in libdnf
 *
 * Returns: an error quark
 **/
GQuark
dnf_error_quark(void)
{
    static GQuark quark = 0;
    if (!quark)
        quark = g_quark_from_static_string("DnfError");
    return quark;
}

/**
 * dnf_realpath:
 * @path: A relative path, e.g. "../../data/test"
 *
 * Converts relative paths to absolute ones.
 *
 * Returns: a new path, or %NULL
 *
 * Since: 0.1.7
 **/
gchar *
dnf_realpath(const gchar *path)
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
 * dnf_remove_recursive:
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
dnf_remove_recursive(const gchar *directory, GError **error)
{
    const gchar *filename;
    g_autoptr(GDir) dir = NULL;
    g_autoptr(GError) error_local = NULL;

    /* try to open */
    dir = g_dir_open(directory, 0, &error_local);
    if (dir == NULL) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "cannot open directory %s: %s",
                    directory, error_local->message);
        return FALSE;
    }

    /* find each */
    while ((filename = g_dir_read_name(dir))) {
        g_autofree gchar *src = NULL;
        src = g_build_filename(directory, filename, NULL);
        if (g_file_test(src, G_FILE_TEST_IS_DIR)) {
            if (!dnf_remove_recursive(src, error))
                return FALSE;
        } else {
            g_debug("deleting file %s", src);
            if (g_unlink(src) != 0) {
                g_set_error(error,
                            DNF_ERROR,
                            DNF_ERROR_INTERNAL_ERROR,
                            "failed to unlink %s", src);
                return FALSE;
            }
        }
    }

    /* remove directory */
    g_debug("deleting directory %s", directory);
    if (g_remove(directory) != 0) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "failed to remove %s", directory);
        return FALSE;
    }
    return TRUE;
}

/**
 * dnf_get_file_contents_allow_noent:
 * @path: File to open
 * @out_contents: Output variable containing contents
 * @out_length: Output variable containing length
 * @error: A #GError, or %NULL
 *
 * Reads a file's contents. If the file does not exist, returns TRUE.
 *
 * Returns: %FALSE if an error other than G_FILE_ERROR_NOENT was set
 *
 * Since: 0.1.7
 **/
gboolean
dnf_get_file_contents_allow_noent(const gchar            *path,
                                  gchar                  **out_contents,
                                  gsize                  *out_length,
                                  GError                 **error)
{
    gsize length;
    g_autofree gchar *contents = NULL;
    g_autoptr(GError) local_error = NULL;

    if (!g_file_get_contents(path, &contents, &length, &local_error)) {
        if (g_error_matches(local_error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
            return TRUE;

        g_propagate_error(error, local_error);
        return FALSE;
    }

    if (out_contents != NULL)
        *out_contents = g_steal_pointer (&contents);
    if (out_length != NULL)
        *out_length = length;

    return TRUE;
}
