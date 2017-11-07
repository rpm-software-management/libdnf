/* dnf-swdb-itemdata.h
 *
 * Copyright (C) 2016 Red Hat, Inc.
 * Author: Eduard Cuba <xcubae00@stud.fit.vutbr.cz>
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

#ifndef DNF_SWDB_OBJ_H
#define DNF_SWDB_OBJ_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DNF_TYPE_SWDB_ITEMDATA dnf_swdb_itemdata_get_type ()

G_DECLARE_FINAL_TYPE (DnfSwdbItemData, dnf_swdb_itemdata, DNF, SWDB_ITEMDATA, GObject)

#include "dnf-swdb-types.h"
#include "dnf-swdb.h"

struct _DnfSwdbItemData
{
    GObject parent_instance;
    gint idid;
    gint iid;
    gint tid;
    gint rid;
    gint tgid;
    gint done;
    gint state_code;
    gint obsoleting;
    DnfSwdbReason reason;
    gchar *state;
    gchar *from_repo;
};

DnfSwdbItemData *
dnf_swdb_itemdata_new (gint iid,
                       gint tid,
                       const gchar *from_repo,
                       gint obsoleting,
                       DnfSwdbReason reason,
                       const gchar *state);

DnfSwdbItemData *
dnf_swdb_itemdata_create (gint idid,
                          gint iid,
                          gint tid,
                          const gchar *from_repo,
                          gint tgid,
                          gint done,
                          gint obsoleting,
                          DnfSwdbReason reason,
                          const gchar *state);

DnfSwdbItemData *
_itemdata_construct (sqlite3_stmt *res);

void
_itemdata_fill (sqlite3_stmt *res, DnfSwdbItemData *obj, gint state);

G_END_DECLS

#endif
