/* dnf-swdb-groups.h
 *
 * Copyright (C) 2017 Red Hat, Inc.
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

#ifndef DNF_SWDB_GROUPS_H
#define DNF_SWDB_GROUPS_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DNF_TYPE_SWDB_GROUP dnf_swdb_group_get_type()
G_DECLARE_FINAL_TYPE ( DnfSwdbGroup, dnf_swdb_group, DNF, SWDB_GROUP, GObject)
struct _DnfSwdbGroup
{
    GObject parent_instance;
    gint gid;
    const gchar* name_id;
    gchar* name;
    gchar* ui_name;
    gint is_installed;
    gint pkg_types;
    gint grp_types;
    DnfSwdb *swdb;
};

DnfSwdbGroup* dnf_swdb_group_new(const gchar* name_id,
                                 gchar* name,
                                 gchar* ui_name,
                                 gint is_installed,
                                 gint pkg_types,
                                 gint grp_types,
                                 DnfSwdb *swdb);

GPtrArray * dnf_swdb_group_get_exclude(DnfSwdbGroup *self);

GPtrArray * dnf_swdb_group_get_full_list(DnfSwdbGroup *self);

gint dnf_swdb_group_update_full_list(DnfSwdbGroup *group,
                                     GPtrArray *full_list);

gint dnf_swdb_group_add_package(DnfSwdbGroup *group,
                                GPtrArray *packages);

gint dnf_swdb_group_add_exclude(DnfSwdbGroup *group,
                                GPtrArray *exclude);

void _insert_group_additional(sqlite3 *db,
                              int gid,
                              GPtrArray *data,
                              const gchar *table);

void _update_group(sqlite3 *db, DnfSwdbGroup *group);

gint _insert_id_name(sqlite3 *db, const gchar *table, gint id, const gchar *name);

gint _group_id_to_gid(sqlite3 *db, const gchar *group_id);

void _add_group(sqlite3 *db, DnfSwdbGroup *group);

DnfSwdbGroup *_get_group(sqlite3 *db, const gchar *name_id);

#define DNF_TYPE_SWDB_ENV dnf_swdb_env_get_type()
G_DECLARE_FINAL_TYPE ( DnfSwdbEnv, dnf_swdb_env, DNF, SWDB_ENV, GObject)
struct _DnfSwdbEnv
{
    GObject parent_instance;
    gint eid;
    const gchar* name_id;
    gchar* name;
    gchar* ui_name;
    gint pkg_types;
    gint grp_types;
    DnfSwdb *swdb;
};

DnfSwdbEnv* dnf_swdb_env_new(const gchar* name_id,
                             gchar* name,
                             gchar* ui_name,
                             gint pkg_types,
                             gint grp_types,
                             DnfSwdb *swdb);

GPtrArray *dnf_swdb_env_get_group_list(DnfSwdbEnv* env);

GPtrArray *dnf_swdb_env_get_exclude    (DnfSwdbEnv* self);

gboolean dnf_swdb_env_is_installed  (DnfSwdbEnv *env );

gint dnf_swdb_env_add_exclude(DnfSwdbEnv *env, GPtrArray *exclude);

gint dnf_swdb_env_add_group(DnfSwdbEnv *env, GPtrArray *groups);

DnfSwdbEnv *_get_env(sqlite3 *db, const gchar *name_id);

GPtrArray *_get_list_from_table(sqlite3_stmt *res);

GPtrArray *_get_data_list(sqlite3 *db, int gid, const gchar *table);

GPtrArray *_env_get_group_list(sqlite3 *db, gint eid);

void _log_group_trans (sqlite3 *db,
                       const gint tid,
                       GPtrArray *groups,
                       const gint is_installed);

void _update_env(sqlite3 *db, DnfSwdbEnv *env);

void _add_env(sqlite3 *db, DnfSwdbEnv *env);

G_END_DECLS

#endif
