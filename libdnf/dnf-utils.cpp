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


#include <errno.h>
#include <stdlib.h>
#include <glib/gstdio.h>

#include "dnf-types.h"
#include "dnf-utils.h"

#include "utils/bgettext/bgettext-lib.h"

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
                    _("cannot open directory %1$s: %2$s"),
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
            if (!dnf_ensure_file_unlinked(src, error))
                return FALSE;
        }
    }

    /* remove directory */
    g_debug("deleting directory %s", directory);
    if (g_remove(directory) != 0) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    _("failed to remove %s"), directory);
        return FALSE;
    }
    return TRUE;
}


/**
 * dnf_ensure_file_unlinked:
 * @src_path: the path to the file
 * @error: A #GError, or %NULL
 *
 * Remove a file based on the file path,
 * refactored from dnf_remove_recursive() function
 *
 * Returns: %FALSE if an error was set
 *
 * Since 0.9.4
 **/
gboolean
dnf_ensure_file_unlinked(const gchar *src_path, GError **error)
{
    if ((unlink(src_path) != 0) && errno != ENOENT) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "failed to unlink %s", src_path);
        return FALSE;
    }

    return TRUE;
}

/**
 * dnf_delete_files_matching:
 * @directory_path: the top level directory path to look at
 * @patterns: the patterns that we are expecting from file/directory's name
 * @error: a #GError instance, to track error
 *
 * Remove recursively all the files/directories that have names
 * which match the patterns. Use with care
 *
 * There are several assumptions that are made in this function:
 *
 * 1: We assume the top level path( the path passed in initially)
 * does not satisfy the criteria of matching the pattern
 *
 * 2: We assume the top level path itself is a directory initially
 *
 * Returns: %FALSE if failed to delete a file/directory
 *
 * Since: 0.9.4
 **/
gboolean
dnf_delete_files_matching(const gchar* directory_path,
                          const char* const* patterns,
                          GError **error)
{
    const gchar *filename;
    g_autoptr(GDir) dir = NULL;

    /* try to open the directory*/
    dir = g_dir_open(directory_path, 0, error);
    if (dir == NULL) {
        g_prefix_error(error, "Cannot open directory %s: ", directory_path);
        return FALSE;
    }
    /* In the directory, we read each file and check if their name matches one of the patterns */
    while ((filename = g_dir_read_name(dir))) {
        g_autofree gchar *src = g_build_filename(directory_path, filename, NULL);
        if (g_file_test(src, G_FILE_TEST_IS_DIR)) {
            gboolean matching = FALSE;
            for (char **iter = (char **) patterns; iter && *iter; iter++) {
                const char* pattern = *iter;
                if (g_str_has_suffix(filename, pattern)) {
                    if (!dnf_remove_recursive(src, error))
                        return FALSE;
                    matching = TRUE;
                    break;
                }
            }
            if (!matching) {
                /* If the directory does not match pattern, keep looking into it */
                if (!dnf_delete_files_matching(src, patterns, error))
                   return FALSE;
            }
        }
        else {
            /* This is for files in the directory, if matches, we directly delete it */
            for (char **iter = (char **)patterns; iter && *iter; iter++) {
                const char* pattern = *iter;
                if (g_str_has_suffix(filename, pattern)) {
                    if (!dnf_ensure_file_unlinked(src, error))
                        return FALSE;
                    break;
                }
            }
        }
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
        *out_contents = static_cast<gchar *>(g_steal_pointer (&contents));
    if (out_length != NULL)
        *out_length = length;

    return TRUE;
}
