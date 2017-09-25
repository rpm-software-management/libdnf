/* dnf-swdb-groups.c
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

#include "dnf-swdb-groups.h"
#include "dnf-swdb-groups-sql.h"
#include "dnf-swdb.h"

G_DEFINE_TYPE (DnfSwdbGroup, dnf_swdb_group, G_TYPE_OBJECT)
G_DEFINE_TYPE (DnfSwdbEnv, dnf_swdb_env, G_TYPE_OBJECT)

// Group destructor
static void
dnf_swdb_group_finalize (GObject *object)
{
    DnfSwdbGroup *group = (DnfSwdbGroup *)object;
    g_free (group->name_id);
    g_free (group->name);
    g_free (group->ui_name);
    G_OBJECT_CLASS (dnf_swdb_group_parent_class)->finalize (object);
}

// Group Class initialiser
static void
dnf_swdb_group_class_init (DnfSwdbGroupClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize = dnf_swdb_group_finalize;
}

// Group Object initialiser
static void
dnf_swdb_group_init (DnfSwdbGroup *self)
{
    self->gid = 0;
    self->swdb = NULL;
    self->installed = FALSE;
}

/**
 * dnf_swdb_group_new:
 *
 * Creates a new #DnfSwdbGroup.
 *
 * Returns: a #DnfSwdbGroup
 **/
DnfSwdbGroup *
dnf_swdb_group_new (const gchar *name_id,
                    const gchar *name,
                    const gchar *ui_name,
                    gboolean installed,
                    gint pkg_types,
                    DnfSwdb *swdb)
{
    DnfSwdbGroup *group = g_object_new (DNF_TYPE_SWDB_GROUP, NULL);
    group->name_id = g_strdup (name_id);
    group->name = g_strdup (name);
    group->ui_name = g_strdup (ui_name);
    group->installed = installed;
    group->pkg_types = pkg_types;
    group->swdb = swdb;
    return group;
}

// environment destructor
static void
dnf_swdb_env_finalize (GObject *object)
{
    DnfSwdbEnv *env = (DnfSwdbEnv *)object;
    g_free (env->name_id);
    g_free (env->name);
    g_free (env->ui_name);
    G_OBJECT_CLASS (dnf_swdb_env_parent_class)->finalize (object);
}

// environment Class initialiser
static void
dnf_swdb_env_class_init (DnfSwdbEnvClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize = dnf_swdb_env_finalize;
}

// environment Object initialiser
static void
dnf_swdb_env_init (DnfSwdbEnv *self)
{
    self->eid = 0;
    self->swdb = NULL;
    self->installed = FALSE;
}

/**
 * dnf_swdb_env_new:
 *
 * Creates a new #DnfSwdbEnv.
 *
 * Returns: a #DnfSwdbEnv
 **/
DnfSwdbEnv *
dnf_swdb_env_new (const gchar *name_id,
                  const gchar *name,
                  const gchar *ui_name,
                  gint pkg_types,
                  gint grp_types,
                  DnfSwdb *swdb)
{
    DnfSwdbEnv *env = g_object_new (DNF_TYPE_SWDB_ENV, NULL);
    env->name_id = g_strdup (name_id);
    env->name = g_strdup (name);
    env->ui_name = g_strdup (ui_name);
    env->pkg_types = pkg_types;
    env->grp_types = grp_types;
    env->swdb = swdb;
    return env;
}

/**
 * _insert_id_name:
 * @db: sqlite database handle
 * @table: table name
 * @id: group/env ID
 * @name: package name
 *
 * Create group-package or environment-group binding
 *
 * Returns: 0 if successful
 *
 * TODO: remove dynamic sql command construction
 **/
gint
_insert_id_name (sqlite3 *db, const gchar *table, gint id, const gchar *name)
{
    sqlite3_stmt *res;
    g_autofree gchar *sql = g_strjoin (" ", "insert into", table, "values(null, @id, @name)", NULL);

    _db_prepare (db, sql, &res);

    // entity id
    _db_bind_int (res, "@id", id);

    // package name
    _db_bind_str (res, "@name", name);

    _db_step (res);
    return 0;
}

/**
 * _find_match:
 * @str: needle
 * @arr: haystack
 *
 * Returns: %TRUE if @str found in @arr
 **/
static gboolean
_find_match (const gchar *str, GPtrArray *arr)
{
    for (guint i = 0; i < arr->len; ++i) {
        if (!g_strcmp0 (str, (gchar *)g_ptr_array_index (arr, i))) {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * _insert_group_additional:
 * @db: sqlite database handle
 * @gid: group ID
 * @data: list of package/group names
 * @table: target table
 *
 * Bind list of package/group names with group/environment ID in @table
 **/
static void
_insert_group_additional (sqlite3 *db, int gid, GPtrArray *data, const gchar *table)
{
    GPtrArray *actualList = _get_data_list (db, gid, table);
    for (guint i = 0; i < data->len; ++i) {
        const gchar *name = (gchar *)g_ptr_array_index (data, i);
        if (!_find_match (name, actualList)) {
            _insert_id_name (db, table, gid, name);
        }
    }
}

/**
 * _env_installed:
 * @env: environment object
 *
 * Resolve if environment is installed by looking at list of installed groups
 **/
static void
_env_installed (sqlite3 *db, DnfSwdbEnv *env)
{
    if (!env->eid)
        return;
    sqlite3_stmt *res;
    const gchar *sql = S_installed_BY_EID;
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@eid", env->eid);
    env->installed = sqlite3_step (res) == SQLITE_ROW;
    sqlite3_finalize (res);
}

/**
 * dnf_swdb_group_add_package:
 * @packages: (element-type utf8)(transfer container): list of package name
 *
 * Add list of package names into group full list
 * Does nothing when group allready contains particular package.
 *
 * Returns: 0 if successful
 **/
gint
dnf_swdb_group_add_package (DnfSwdbGroup *group, GPtrArray *packages)
{

    if (!group->gid || dnf_swdb_open (group->swdb))
        return 1;
    _insert_group_additional (group->swdb->db, group->gid, packages, "GROUPS_PACKAGE");
    return 0;
}

/**
 * dnf_swdb_group_add_exclude:
 * @exclude: (element-type utf8)(transfer container): list of package names
 *
 * Add list of package names into group exclude list.
 * Does nothing if particular package is allready in exclude list.
 *
 * Returns: 0 if successful
 **/
gint
dnf_swdb_group_add_exclude (DnfSwdbGroup *group, GPtrArray *exclude)
{
    if (!group->gid || dnf_swdb_open (group->swdb))
        return 1;
    _insert_group_additional (group->swdb->db, group->gid, exclude, "GROUPS_EXCLUDE");
    return 0;
}

/**
 * dnf_swdb_env_add_exclude:
 * @exclude: (element-type utf8)(transfer container): list of group names
 *
 * Add list of groups into environment exclude list
 * Does nothing when particular group is allready in lexclude list
 *
 * Returns: 0 if successful
 **/
gint
dnf_swdb_env_add_exclude (DnfSwdbEnv *env, GPtrArray *exclude)
{
    if (!env->eid || dnf_swdb_open (env->swdb))
        return 1;
    sqlite3 *db = env->swdb->db;
    GPtrArray *actualList = _env_get_exclude (db, env->eid);
    for (guint i = 0; i < exclude->len; ++i) {
        const gchar *name = (gchar *)g_ptr_array_index (exclude, i);
        if (!_find_match (name, actualList)) {
            _insert_id_name (db, "ENVIRONMENTS_EXCLUDE", env->eid, name);
        }
    }
    return 0;
}

/**
 * _group_id_to_gid:
 * @db: sqlite database handle
 * @group_id: group unique identifier string
 *
 * Returns: group ID
 **/
gint
_group_id_to_gid (sqlite3 *db, const gchar *group_id)
{
    sqlite3_stmt *res;
    const gchar *sql = S_GID_BY_NAME_ID;
    _db_prepare (db, sql, &res);
    _db_bind_str (res, "@id", group_id);
    return _db_find_int (res);
}

/**
 * dnf_swdb_env_add_group:
 * @groups: (element-type utf8)(transfer container): list of group names
 *
 * Add list of groups into environment full list.
 * When environment allready contains particular group, does nothing.
 *
 * Returns: 0 if successful
 **/
gint
dnf_swdb_env_add_group (DnfSwdbEnv *env, GPtrArray *groups)
{
    if (!env->eid || dnf_swdb_open (env->swdb))
        return 1;
    const gchar *sql = I_ENV_GROUP;
    gint gid;
    GPtrArray *actualList = _env_get_group_list (env->swdb->db, env->eid);
    for (guint i = 0; i < groups->len; i++) {
        const gchar *name = (gchar *)g_ptr_array_index (groups, i);

        // test for existing binding
        if (_find_match (name, actualList)) {
            continue;
        }

        // bind group_id to gid
        gid = _group_id_to_gid (env->swdb->db, name);
        if (!gid)
            continue;
        sqlite3_stmt *res;
        _db_prepare (env->swdb->db, sql, &res);
        _db_bind_int (res, "@eid", env->eid);
        _db_bind_int (res, "@gid", gid);
        _db_step (res);
    }
    return 0;
}

/**
 * _add_group:
 * @db: sqlite database handle
 * @group: group object
 *
 * Insert @group into SWDB
 **/
void
_add_group (sqlite3 *db, DnfSwdbGroup *group)
{
    sqlite3_stmt *res;
    const gchar *sql = I_GROUP;
    _db_prepare (db, sql, &res);
    _db_bind_str (res, "@name_id", group->name_id);
    _db_bind_str (res, "@name", group->name);
    _db_bind_str (res, "@ui_name", group->ui_name);
    _db_bind_int (res, "@installed", group->installed);
    _db_bind_int (res, "@pkg_types", group->pkg_types);
    _db_step (res);
    group->gid = sqlite3_last_insert_rowid (db);
}

/**
 * dnf_swdb_add_group:
 * @self: SWDB object
 * @group: group object
 *
 * Insert or update @group
 *
 * Returns: 0 if successful
 **/
gint
dnf_swdb_add_group (DnfSwdb *self, DnfSwdbGroup *group)
{
    if (dnf_swdb_open (self))
        return 1;
    g_autoptr (DnfSwdbGroup) old = _get_group (self->db, group->name_id);
    if (!old) {
        _add_group (self->db, group);
    } else {
        group->gid = old->gid;
        _update_group (self->db, group);
    }
    return 0;
}

/**
 * _update_env:
 * @db: sqlite database handle
 * @env: environment object
 *
 * Update environment attributes
 **/
void
_update_env (sqlite3 *db, DnfSwdbEnv *env)
{
    sqlite3_stmt *res;
    const gchar *sql = U_ENV;
    _db_prepare (db, sql, &res);
    _db_bind_str (res, "@name", env->name);
    _db_bind_str (res, "@ui_name", env->ui_name);
    _db_bind_int (res, "@pkg_types", env->pkg_types);
    _db_bind_int (res, "@grp_types", env->grp_types);
    _db_bind_int (res, "@eid", env->eid);
    _db_step (res);
}

/**
 * _add_env:
 * @db: sqlite database holder
 * @env: environment object
 *
 * Insert @env into SWDB
 **/
void
_add_env (sqlite3 *db, DnfSwdbEnv *env)
{
    sqlite3_stmt *res;
    const gchar *sql = I_ENV;
    _db_prepare (db, sql, &res);
    _db_bind_str (res, "@name_id", env->name_id);
    _db_bind_str (res, "@name", env->name);
    _db_bind_str (res, "@ui_name", env->ui_name);
    _db_bind_int (res, "@pkg_types", env->pkg_types);
    _db_bind_int (res, "@grp_types", env->grp_types);
    _db_step (res);
    env->eid = sqlite3_last_insert_rowid (db);
}

/**
 * dnf_swdb_add_env:
 * @self: SWDB object
 * @env: environment object
 *
 * Insert or update environment @env
 *
 * Returns: 0 if successful
 **/
gint
dnf_swdb_add_env (DnfSwdb *self, DnfSwdbEnv *env)
{
    if (dnf_swdb_open (self))
        return 1;
    g_autoptr (DnfSwdbEnv) old = _get_env (self->db, env->name_id);
    if (!old) {
        _add_env (self->db, env);
    } else {
        env->eid = old->eid;
        _update_env (self->db, env);
    }
    return 0;
}

/**
 * _get_group:
 * @db: sqlite database handle
 * @name_id: uniqie group identifier
 *
 * Get group object by @name_id
 *
 * Returns: #DnfSwdbGroup matched by @name_id
 **/
DnfSwdbGroup *
_get_group (sqlite3 *db, const gchar *name_id)
{
    sqlite3_stmt *res;
    const gchar *sql = S_GROUP_BY_NAME_ID;
    _db_prepare (db, sql, &res);
    _db_bind_str (res, "@id", name_id);
    if (sqlite3_step (res) == SQLITE_ROW) {
        DnfSwdbGroup *group = dnf_swdb_group_new (name_id,                               // name_id
                                                  (gchar *)sqlite3_column_text (res, 2), // name
                                                  (gchar *)sqlite3_column_text (res, 3), // ui_name
                                                  sqlite3_column_int (res, 4), // installed
                                                  sqlite3_column_int (res, 5), // pkg_types
                                                  NULL);                       // swdb
        group->gid = sqlite3_column_int (res, 0);
        sqlite3_finalize (res);
        return group;
    }
    return NULL;
}

/**
 * dnf_swdb_get_group:
 * @self: SWDB object
 * @name_id: uniqie group identifier
 *
 * Get group object by @name_id
 *
 * Returns: (transfer full): #DnfSwdbGroup matched by name_id
 */
DnfSwdbGroup *
dnf_swdb_get_group (DnfSwdb *self, const gchar *name_id)
{
    if (dnf_swdb_open (self))
        return NULL;

    DnfSwdbGroup *group = _get_group (self->db, name_id);
    if (group)
        group->swdb = self;
    return group;
}

/**
 * _get_env:
 * @db: sqlite database handle
 * @name_id: uniqie environment identifier
 *
 * Get environment object by @name_id
 *
 * Returns: #DnfSwdbEnv matched by @name_id
 **/

DnfSwdbEnv *
_get_env (sqlite3 *db, const gchar *name_id)
{
    sqlite3_stmt *res;
    const gchar *sql = S_ENV_BY_NAME_ID;
    _db_prepare (db, sql, &res);
    _db_bind_str (res, "@id", name_id);
    if (sqlite3_step (res) == SQLITE_ROW) {
        DnfSwdbEnv *env = dnf_swdb_env_new (name_id,                               // name_id
                                            (gchar *)sqlite3_column_text (res, 2), // name
                                            (gchar *)sqlite3_column_text (res, 3), // ui_name
                                            sqlite3_column_int (res, 4),           // pkg_types
                                            sqlite3_column_int (res, 5),           // grp_types
                                            NULL);                                 // swdb
        env->eid = sqlite3_column_int (res, 0);
        sqlite3_finalize (res);
        _env_installed (db, env);
        return env;
    }
    return NULL;
}

/**
 * dnf_swdb_get_env:
 * @self: SWDB object
 * @name_id: uniqie environment identifier
 *
 * Get environment object by @name_id
 *
 * Returns: (transfer full): #DnfSwdbEnv matched by @name_id
 **/
DnfSwdbEnv *
dnf_swdb_get_env (DnfSwdb *self, const gchar *name_id)
{
    if (dnf_swdb_open (self))
        return NULL;

    DnfSwdbEnv *env = _get_env (self->db, name_id);
    if (env)
        env->swdb = self;
    return env;
}

/**
 * dnf_swdb_groups_by_pattern:
 * @self: SWDB object
 * @pattern: string pattern
 *
 * Get list of groups matched by @pattern
 * @pattern can match name_id, name or ui_name
 *
 * Returns: (element-type DnfSwdbGroup)(array)(transfer container): list of #DnfSwdbGroup
 **/
GPtrArray *
dnf_swdb_groups_by_pattern (DnfSwdb *self, const gchar *pattern)
{
    if (dnf_swdb_open (self))
        return NULL;
    GPtrArray *node = g_ptr_array_new ();
    sqlite3_stmt *res;
    const gchar *sql = S_GROUPS_BY_PATTERN;
    _db_prepare (self->db, sql, &res);
    _db_bind_str (res, "@pat", pattern);
    _db_bind_str (res, "@pat", pattern);
    _db_bind_str (res, "@pat", pattern);
    while (sqlite3_step (res) == SQLITE_ROW) {
        DnfSwdbGroup *group =
          dnf_swdb_group_new ((const gchar *)sqlite3_column_text (res, 1), // name_id
                              (gchar *)sqlite3_column_text (res, 2),       // name
                              (gchar *)sqlite3_column_text (res, 3),       // ui_name
                              sqlite3_column_int (res, 4),                 // installed
                              sqlite3_column_int (res, 5),                 // pkg_types
                              self);                                       // swdb
        group->gid = sqlite3_column_int (res, 0);
        g_ptr_array_add (node, (gpointer)group);
    }
    sqlite3_finalize (res);
    return node;
}

/**
 * dnf_swdb_env_by_pattern:
 * @self: SWDB object
 * @pattern: string pattern
 *
 * Get list of environment matched by @pattern
 * @pattern can match name_id, name or ui_name
 *
 * Returns: (element-type DnfSwdbEnv)(array)(transfer container): list of #DnfSwdbEnv
 **/
GPtrArray *
dnf_swdb_env_by_pattern (DnfSwdb *self, const gchar *pattern)
{
    if (dnf_swdb_open (self))
        return NULL;
    GPtrArray *node = g_ptr_array_new ();
    sqlite3_stmt *res;
    const gchar *sql = S_ENV_BY_PATTERN;
    _db_prepare (self->db, sql, &res);
    _db_bind_str (res, "@pat", pattern);
    _db_bind_str (res, "@pat", pattern);
    _db_bind_str (res, "@pat", pattern);
    while (sqlite3_step (res) == SQLITE_ROW) {
        DnfSwdbEnv *env = dnf_swdb_env_new ((const gchar *)sqlite3_column_text (res, 1), // name_id
                                            (gchar *)sqlite3_column_text (res, 2),       // name
                                            (gchar *)sqlite3_column_text (res, 3),       // ui_name
                                            sqlite3_column_int (res, 4), // pkg_types
                                            sqlite3_column_int (res, 5), // grp_types
                                            self);                       // swdb
        env->eid = sqlite3_column_int (res, 0);
        g_ptr_array_add (node, (gpointer) env);
    }
    sqlite3_finalize (res);
    for (guint i = 0; i < node->len; ++i) {
        DnfSwdbEnv *env = g_ptr_array_index (node, i);
        _env_installed (self->db, env);
    }
    return node;
}

/**
 * _get_list_from_table:
 * @res: sqlite3_stmt returning list of strings
 *
 * Returns: list of string matched by @res
 **/
GPtrArray *
_get_list_from_table (sqlite3_stmt *res)
{
    GPtrArray *node = g_ptr_array_new ();
    while (sqlite3_step (res) == SQLITE_ROW) {
        gchar *str = g_strdup ((const gchar *)sqlite3_column_text (res, 0));
        g_ptr_array_add (node, (gpointer)str);
    }
    sqlite3_finalize (res);
    return node;
}

/**
 * _get_data_list:
 * @db: sqlite database handle
 * @gid: group id
 * @table: target table
 *
 * Get list of names connected with @gid from @table
 *
 * Returns: list of names
 *
 * TODO: don't use dynamic sql command construction
 **/
GPtrArray *
_get_data_list (sqlite3 *db, int gid, const gchar *table)
{
    sqlite3_stmt *res;
    g_autofree gchar *sql = g_strjoin (" ", "SELECT name FROM", table, "where G_ID=@gid", NULL);
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@gid", gid);
    GPtrArray *node = _get_list_from_table (res);
    return node;
}

/**
 * dnf_swdb_group_get_exclude:
 * @self: group object
 *
 * Get list of excludes for @self
 *
 * Returns: (element-type utf8)(array)(transfer container): list of excludes
 **/
GPtrArray *
dnf_swdb_group_get_exclude (DnfSwdbGroup *self)
{
    if (!self->gid)
        return NULL;
    if (dnf_swdb_open (self->swdb))
        return NULL;
    GPtrArray *node = _get_data_list (self->swdb->db, self->gid, "GROUPS_EXCLUDE");
    return node;
}

/**
 * dnf_swdb_group_get_full_list:
 * @self: group object
 *
 * Get full package list of @self
 *
 * Returns: (element-type utf8)(array)(transfer container): list of package names
 **/
GPtrArray *
dnf_swdb_group_get_full_list (DnfSwdbGroup *self)
{
    if (!self->gid)
        return NULL;
    if (dnf_swdb_open (self->swdb))
        return NULL;
    GPtrArray *node = _get_data_list (self->swdb->db, self->gid, "GROUPS_PACKAGE");
    return node;
}

/**
 * dnf_swdb_group_update_full_list:
 * @group: group object
 * @full_list: (element-type utf8)(transfer container): list of package names
 *
 * Update list of packages installed with group
 *
 * Returns: 0 if successful
 */
gint
dnf_swdb_group_update_full_list (DnfSwdbGroup *group, GPtrArray *full_list)
{
    if (!group->gid)
        return 1;
    if (dnf_swdb_open (group->swdb))
        return 1;
    sqlite3_stmt *res;
    const gchar *sql = R_FULL_LIST_BY_ID;
    _db_prepare (group->swdb->db, sql, &res);
    _db_bind_int (res, "@gid", group->gid);
    _db_step (res);
    _insert_group_additional (group->swdb->db, group->gid, full_list, "GROUPS_PACKAGE");
    return 0;
}

/**
 * _update_group:
 * @db: sqlite database handle
 * @group: group object
 *
 * Update @group data in SWDB by properties from @group object
 **/
void
_update_group (sqlite3 *db, DnfSwdbGroup *group)
{
    sqlite3_stmt *res;
    const gchar *sql = U_GROUP;
    _db_prepare (db, sql, &res);
    _db_bind_str (res, "@name", group->name);
    _db_bind_str (res, "@ui_name", group->ui_name);
    _db_bind_int (res, "@installed", group->installed);
    _db_bind_int (res, "@pkg_types", group->pkg_types);
    _db_bind_int (res, "@gid", group->gid);
    _db_step (res);
}

/**
 * dnf_swdb_uninstall_group:
 * @self: SWDB object
 * @group: group object
 *
 * Set group installed to %false
 *
 * Returns: 0 if successful
 **/
gint
dnf_swdb_uninstall_group (DnfSwdb *self, DnfSwdbGroup *group)
{
    if (!group->gid)
        return 1;
    if (dnf_swdb_open (self))
        return 1;
    group->installed = FALSE;
    _update_group (self->db, group);
    return 0;
}

/**
 * _env_get_group_list:
 * @db: sqlite database handle
 * @eid: environment ID
 *
 * Get full list of groups for environment @eid
 *
 * Returns: list of group names
 **/
GPtrArray *
_env_get_group_list (sqlite3 *db, gint eid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_GROUP_NAME_ID_BY_EID;
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@eid", eid);
    return _get_list_from_table (res);
}

/**
 * dnf_swdb_env_get_group_list:
 * @env: environment object
 *
 * Get full list of groups for environment @env
 *
 * Returns: (element-type utf8)(array)(transfer container): list of group names
 */
GPtrArray *
dnf_swdb_env_get_group_list (DnfSwdbEnv *env)
{
    if (!env->eid || dnf_swdb_open (env->swdb))
        return NULL;
    GPtrArray *node = _env_get_group_list (env->swdb->db, env->eid);
    return node;
}

/**
 * _env_get_exclude:
 * @db: sqlite database handle
 * @eid: environment ID
 *
 * Get environment exclude list
 *
 * Returns: list of excluded groups
 **/
GPtrArray *
_env_get_exclude (sqlite3 *db, int eid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_ENV_EXCLUDE_BY_ID;
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@eid", eid);
    return _get_list_from_table (res);
}

/**
 * dnf_swdb_env_get_exclude:
 * @self: environment object
 *
 * Get environment exclude list
 *
 * Returns: (element-type utf8)(array)(transfer container): list of excluded group names
 **/
GPtrArray *
dnf_swdb_env_get_exclude (DnfSwdbEnv *self)
{
    if (!self->eid)
        return NULL;
    if (dnf_swdb_open (self->swdb))
        return NULL;
    GPtrArray *node = _env_get_exclude (self->swdb->db, self->eid);
    return node;
}

/**
 * dnf_swdb_groups_commit:
 * @self: SWDB object
 * @groups: (element-type DnfSwdbGroup)(array)(transfer container): list of #DnfSwdbGroup
 *
 * Tag list of @groups as installed
 *
 * Returns: 0 if successful
 **/
gint
dnf_swdb_groups_commit (DnfSwdb *self, GPtrArray *groups)
{
    if (dnf_swdb_open (self))
        return 1;
    const gchar *sql = U_GROUP_COMMIT;
    for (guint i = 0; i < groups->len; ++i) {
        DnfSwdbGroup *group = (DnfSwdbGroup *)g_ptr_array_index (groups, i);
        sqlite3_stmt *res;
        _db_prepare (self->db, sql, &res);
        _db_bind_str (res, "@id", group->name_id);
        _db_step (res);
    }
    return 0;
}

/**
 * _log_group_trans:
 * @db: sqlite database handle
 * @tid: transaction ID
 * @groups: list of #DnfSwdbGroup to be installed
 * @installed: tag whether groups should be tagged as installed
 **/
void
_log_group_trans (sqlite3 *db, gint tid, GPtrArray *groups, gboolean installed)
{
    const gchar *sql = I_TRANS_GROUP_DATA;
    for (guint i = 0; i < groups->len; ++i) {
        sqlite3_stmt *res;
        DnfSwdbGroup *group = g_ptr_array_index (groups, i);
        _db_prepare (db, sql, &res);
        _db_bind_int (res, "@tid", tid);
        _db_bind_int (res, "@gid", group->gid);
        _db_bind_str (res, "@name_id", group->name_id);
        _db_bind_str (res, "@name", group->name);
        _db_bind_str (res, "@ui_name", group->ui_name);
        _db_bind_int (res, "@installed", installed);
        _db_bind_int (res, "@pkg_types", group->pkg_types);
        _db_step (res);
    }
}

/**
 * dnf_swdb_log_group_trans:
 * @self: SWDB object
 * @tid: transaction ID
 * @installing: (element-type DnfSwdbGroup)(array)(transfer container): list of #DnfSwdbGroup
 * @removing: (element-type DnfSwdbGroup)(array)(transfer container): list of #DnfSwdbGroup
 *
 * Log group transaction
 *
 * Returns: 0 if successful
 **/
gint
dnf_swdb_log_group_trans (DnfSwdb *self, gint tid, GPtrArray *installing, GPtrArray *removing)
{
    if (dnf_swdb_open (self))
        return 1;
    _log_group_trans (self->db, tid, installing, 1);
    _log_group_trans (self->db, tid, removing, 0);
    return 0;
}

/**
 * dnf_swdb_removable_pkg:
 * @self: SWDB object
 * @pkg_name: name of the package
 *
 * Test if package can be removed with group
 *   - package was installed with group
 *   - package doesn't appear in any other installed group
 *
 * Returns: %TRUE if package can be removed
 **/
gboolean
dnf_swdb_removable_pkg (DnfSwdb *self, const gchar *pkg_name)
{
    if (dnf_swdb_open (self))
        return FALSE;
    gboolean removable = TRUE;
    // check if package was installed with group

    sqlite3_stmt *res;
    const gchar *sql = S_REM_REASON;
    _db_prepare (self->db, sql, &res);
    _db_bind_str (res, "@name", pkg_name);
    gint reason = _db_find_int (res);
    if (reason != DNF_SWDB_REASON_GROUP) {
        removable = FALSE;
    } else {
        sql = S_REM;
        _db_prepare (self->db, sql, &res);
        _db_bind_str (res, "@name", pkg_name);
        gint count = 0;
        while (sqlite3_step (res) == SQLITE_ROW && count < 2) {
            count++;
        }
        sqlite3_finalize (res);
        removable = count < 2;
    }
    return removable;
}
