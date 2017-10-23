/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2013-2015 Richard Hughes <richard@hughsie.com>
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

#ifndef __DNF_UTILS_H
#define __DNF_UTILS_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

gchar           *dnf_realpath                       (const gchar            *path);
gboolean         dnf_remove_recursive               (const gchar            *directory,
                                                     GError                 **error);
gboolean         dnf_ensure_file_unlinked           (const gchar            *src_path,
                                                     GError                 **error);
gboolean         dnf_delete_files_matching          (const gchar            *directory_path,
                                                     const char* const      *patterns,
                                                     GError                 **error);
gboolean         dnf_get_file_contents_allow_noent  (const gchar            *path,
                                                     gchar                  **out_contents,
                                                     gsize                  *length,
                                                     GError                 **error);

#ifdef __cplusplus
}
#endif

#endif /* __DNF_UTILS_H */
