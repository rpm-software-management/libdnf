/* dnf-swdb-types.c
 *
 * Copyright (C) 2017 Red Hat, Inc.
 * Author: Eduard Cuba <ecuba@redhat.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "dnf-swdb-types.h"

static const gchar *const _reasons_str[] = {
    "unknown",  // 0
    "dep",      // 1
    "user",     // 2
    "clean",    // 3
    "weak",     // 4
    "group"     // 5
};

const guint _reasons_len = sizeof _reasons_str / sizeof *_reasons_str;

/**
 * dnf_convert_reason_to_id:
 * @reason: reason string;
 *
 * Convert reason string to reason ID
 * Returns: reason ID or 0 if unknown
 **/
DnfSwdbReason
dnf_convert_reason_to_id (const gchar *reason)
{
    for (guint i = 0; i < _reasons_len; ++i) {
        if (g_strcmp0(reason, _reasons_str[i]) == 0) {
            return i;
        }
    }
    return DNF_SWDB_REASON_UNKNOWN;
}
