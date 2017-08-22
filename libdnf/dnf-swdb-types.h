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

#ifndef DNF_SWDB_TYPES_H
#define DNF_SWDB_TYPES_H

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
    DNF_SWDB_REASON_UNKNOWN = 0,
    DNF_SWDB_REASON_DEP = 1,
    DNF_SWDB_REASON_USER = 2,
    DNF_SWDB_REASON_CLEAN = 3, // hawkey compatibility
    DNF_SWDB_REASON_WEAK = 4,
    DNF_SWDB_REASON_GROUP = 5
} DnfSwdbReason;

DnfSwdbReason dnf_convert_reason_to_id (const gchar  *reason);
gchar        *dnf_convert_id_to_reason (DnfSwdbReason reason);

G_END_DECLS

#endif
