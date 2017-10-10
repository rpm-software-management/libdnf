/* dnf-swdb-obj.h
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

#define DNF_TYPE_SWDB_PKGDATA   dnf_swdb_pkgdata_get_type()
#define DNF_TYPE_SWDB_TRANSDATA dnf_swdb_transdata_get_type()

G_DECLARE_FINAL_TYPE (DnfSwdbPkgData, dnf_swdb_pkgdata, DNF, SWDB_PKGDATA, GObject)
G_DECLARE_FINAL_TYPE (DnfSwdbTransData, dnf_swdb_transdata, DNF, SWDB_TRANSDATA, GObject)

#include "dnf-swdb.h"
#include "dnf-swdb-types.h"

struct _DnfSwdbPkgData
{
    GObject parent_instance;
    gchar *from_repo;
    gchar *from_repo_revision;
    gint64 from_repo_timestamp;
    gchar *installed_by;
    gchar *changed_by;
    gint pdid;
    gint pid;
};

struct _DnfSwdbTransData
{
    GObject parent_instance;
    gint tdid;
    gint tid;
    gint pdid;
    gint tgid;
    gint done;
    gint obsoleting;
    DnfSwdbReason reason;
    gchar *state;
};

DnfSwdbPkgData     *dnf_swdb_pkgdata_new            (const gchar   *from_repo_revision,
                                                     gint64         from_repo_timestamp,
                                                     const gchar   *installed_by,
                                                     const gchar   *changed_by,
                                                     const gchar   *from_repo);
DnfSwdbTransData   *dnf_swdb_transdata_new          (gint           tdid,
                                                     gint           tid,
                                                     gint           pdid,
                                                     gint           tgid,
                                                     gint           done,
                                                     gint           obsoleting,
                                                     DnfSwdbReason  reason,
                                                     gchar         *state);
G_END_DECLS

#endif
