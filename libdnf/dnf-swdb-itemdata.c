/* dnf-swdb-itemdata.c
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

#include "dnf-swdb-itemdata.h"
#include "dnf-swdb-db.h"

G_DEFINE_TYPE (DnfSwdbItemData, dnf_swdb_itemdata, G_TYPE_OBJECT)

// item data destructor
static void
dnf_swdb_itemdata_finalize (GObject *object)
{
    DnfSwdbItemData *data = (DnfSwdbItemData *)object;
    g_free (data->state);
    g_free (data->from_repo);
    G_OBJECT_CLASS (dnf_swdb_itemdata_parent_class)->finalize (object);
}

// item data class initialiser
static void
dnf_swdb_itemdata_class_init (DnfSwdbItemDataClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize = dnf_swdb_itemdata_finalize;
}

// item data object initialiser
static void
dnf_swdb_itemdata_init (DnfSwdbItemData *self)
{
    self->from_repo = NULL;
}

/**
 * dnf_swdb_itemdata_new:
 *
 * Creates a new #DnfSwdbItemData with mandatory attributes
 * Used in a package manager when initializing data for package
 * in particular transaction
 *
 * Returns: a #DnfSwdbItemData
 **/
DnfSwdbItemData *
dnf_swdb_itemdata_new (gint iid,
                       gint tid,
                       const gchar *from_repo,
                       gint obsoleting,
                       DnfSwdbReason reason,
                       const gchar *state)
{
    DnfSwdbItemData *data = g_object_new (DNF_TYPE_SWDB_ITEMDATA, NULL);
    data->tid = tid;
    data->pdid = pdid;
    data->from_repo = g_strdup (from_repo);
    data->obsoleting = obsoleting;
    data->reason = reason;
    data->state = g_strdup (state);
    return data;
}

/**
 * dnf_swdb_itemdata_create:
 *
 * Creates a new #DnfSwdbItemData with all the attributes
 *  used to create full object by database
 *
 * Returns: a #DnfSwdbItemData
 **/
DnfSwdbItemData *
dnf_swdb_itemdata_create (gint idid,
                          gint iid,
                          gint tid,
                          const gchar *from_repo,
                          gint tgid,
                          gint done,
                          gint obsoleting,
                          DnfSwdbReason reason,
                          const gchar *state)
{
    DnfSwdbItemData *data = g_object_new (DNF_TYPE_SWDB_ITEMDATA, NULL);
    data->idid = idid;
    data->tid = tid;
    data->pdid = pdid;
    data->tgid = tgid;
    data->done = done;
    data->obsoleting = obsoleting;
    data->reason = reason;
    data->state = g_strdup (state);
    data->from_repo = g_strdup (from_repo);
    return data;
}

/**
 * _itemdata_construct:
 * @res: prepared sqlite3 statement
 *
 * Create #DnfSwdbItemData from prepared sqlite statement
 * Returns: full DnfSwdbItemData object from database if available, else NULL
 **/
DnfSwdbItemData *
_itemdata_construct (sqlite3_stmt *res)
{
    if (sqlite3_step (res) != SQLITE_ROW) {
        return NULL;
    }
    return dnf_swdb_itemdata_create (sqlite3_column_int (res, 0),           // idid
                                     sqlite3_column_int (res, 1),           // iid
                                     sqlite3_column_int (res, 2),           // tid
                                     (gchar *)sqlite3_column_text (res, 3), // from repo
                                     sqlite3_column_int (res, 4),           // tgid
                                     sqlite3_column_int (res, 5),           // done
                                     sqlite3_column_int (res, 6),           // obsoleting
                                     sqlite3_column_int (res, 7),           // reason
                                     (gchar *)sqlite3_column_text (res, 8)  // state
    );
}

/**
 * _itemdata_fill:
 * @res: prepared sqlite3_stmt
 * @obj: #DnfSwdbItemData object
 *
 * Fills prepared statement with @obj data - used for itemdata_add (just mandatory data)
 **/
void
_itemdata_fill (sqlite3_stmt *res, DnfSwdbItemData *obj, gint state)
{
    _db_bind_int (res, "@iid", obj->iid);
    _db_bind_int (res, "@tid", obj->tid);
    _db_bind_int (res, "@rid", obj->rid);
    _db_bind_int (res, "@tgid", obj->tgid);
    _db_bind_int (res, "@done", obj->done);
    _db_bind_int (res, "@obsoleting", obj->obsoleting);
    _db_bind_int (res, "@reason", obj->reason);
    _db_bind_int (res, "@state", state);
}
