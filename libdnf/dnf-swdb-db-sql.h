/* dnf-swdb-db-sql.h
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
 * Lesser GeneDral Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _DNF_SWDB_DB_SQL
#define _DNF_SWDB_DB_SQL

#define TRUNCATE_EXCLUSIVE "PRAGMA journal_mode = TRUNCATE; PRAGMA locking_mode = EXCLUSIVE"

#define SIMPLE_SEARCH "SELECT I_ID FROM ITEM WHERE name LIKE @pat"

#define SEARCH_PATTERN \
    "SELECT I_ID,name,epoch,version,release,arch," \
    "name || '.' || arch AS sql_na," \
    "name || '-' || version || '-' || release || '.' || arch AS sql_nvra," \
    "name || '-' || version AS sql_nv," \
    "name || '-' || version || '-' || release AS sql_nvr," \
    "epoch || ':' || name || '-' || version || '-' || release || '.' || arch AS sql_envra," \
    "name || '-' || epoch || ':' || version || '-' || release || '.' || arch AS sql_nevra " \
    "FROM ITEM WHERE sql_na LIKE @pat OR sql_nvra LIKE @pat OR sql_nv LIKE @pat OR" \
    " sql_nvr LIKE @pat OR sql_nevra LIKE @pat OR sql_envra LIKE @pat"

#define FIND_TIDS_FROM_IDID "SELECT T_ID FROM ITEM_DATA WHERE ID_ID=@idid"
#define FIND_ALL_IDID_FOR_IID "SELECT ID_ID FROM ITEM_DATA WHERE I_ID=@iid"

#define CREATE_TABLES \
    "CREATE TABLE repo(" \
    "   r_id INTEGER PRIMARY KEY," \
    "   name TEXT NOT NULL);" \
    "CREATE TABLE trans(" \
    "   t_id INTEGER PRIMARY KEY," \
    "   beg_timestamp INTEGER NOT NULL," \
    "   end_timestamp INTEGER," \
    "   beg_rpmdb_version TEXT," \
    "   end_rpmdb_version TEXT," \
    "   cmdline TEXT," \
    "   loginuid TEXT," \
    "   releasever TEXT NOT NULL," \
    "   return_code INTEGER);" \
    "CREATE TABLE groups(" \
    "   g_id INTEGER PRIMARY KEY," \
    "   name_id TEXT NOT NULL," \
    "   name TEXT," \
    "   ui_name TEXT," \
    "   installed INTEGER NOT NULL," \
    "   pkg_types INTEGER NOT NULL);" \
    "CREATE TABLE environments(" \
    "   e_id INTEGER PRIMARY KEY," \
    "   name_id TEXT NOT NULL," \
    "   name TEXT," \
    "   ui_name TEXT," \
    "   pkg_types INTEGER NOT NULL," \
    "   grp_types INTEGER NOT NULL);" \
    "CREATE TABLE trans_group_data(" \
    "   tg_id INTEGER PRIMARY KEY," \
    "   t_id INTEGER REFERENCES trans(t_id)," \
    "   g_id INTEGER REFERENCES groups(g_id)," \
    "   name_id TEXT NOT NULL," \
    "   name TEXT," \
    "   ui_name TEXT," \
    "   installed INTEGER NOT NULL," \
    "   pkg_types INTEGER NOT NULL);" \
    "CREATE TABLE state_type(" \
    "   state INTEGER PRIMARY KEY," \
    "   description TEXT NOT NULL);" \
    "CREATE TABLE item(" \
    "   i_id INTEGER PRIMARY KEY," \
    "   name TEXT NOT NULL," \
    "   epoch INTEGER NOT NULL," \
    "   version TEXT NOT NULL," \
    "   release TEXT NOT NULL," \
    "   arch TEXT NOT NULL," \
    "   checksum_data TEXT NOT NULL," \
    "   checksum_type TEXT NOT NULL," \
    "   type INTEGER NOT NULL);" \
    "CREATE TABLE item_data(" \
    "   id_id INTEGER PRIMARY KEY," \
    "   i_id INTEGER REFERENCES item(i_id)," \
    "   t_id INTEGER REFERENCES trans(t_id)," \
    "   r_id INTEGER REFERENCES repo(r_id)," \
    "   tg_id INTEGER REFERENCES trans_group_data(tg_id)," \
    "   done INTEGER NOT NULL," \
    "   obsoleting INTEGER NOT NULL," \
    "   reason INTEGER NOT NULL," \
    "   state INTEGER REFERENCES state_type(state));" \
    "CREATE TABLE output_type(" \
    "   type INTEGER PRIMARY KEY," \
    "   description TEXT NOT NULL);" \
    "CREATE TABLE output(" \
    "   o_id INTEGER PRIMARY KEY," \
    "   t_id INTEGER REFERENCES trans(t_id)," \
    "   msg TEXT," \
    "   type INTEGER REFERENCES output_type(type));" \
    "CREATE TABLE groups_package(" \
    "   gp_id INTEGER PRIMARY KEY," \
    "   g_id INTEGER REFERENCES groups(g_id)," \
    "   name TEXT NOT NULL);" \
    "CREATE TABLE groups_exclude(" \
    "   ge_id INTEGER PRIMARY KEY," \
    "   g_id INTEGER REFERENCES groups(g_id)," \
    "   name TEXT NOT NULL);" \
    "CREATE TABLE environments_groups(" \
    "   eg_id INTEGER PRIMARY KEY," \
    "   e_id INTEGER REFERENCES environments(e_id)," \
    "   g_id INTEGER REFERENCES groups(g_id));" \
    "CREATE TABLE environments_exclude(" \
    "   ee_id INTEGER PRIMARY KEY," \
    "   e_id INTEGER REFERENCES environments(e_id)," \
    "   name TEXT NOT NULL);" \
    "CREATE TABLE trans_with(" \
    "   tw_id INTEGER PRIMARY KEY," \
    "   t_id INTEGER REFERENCES trans(t_id)," \
    "   i_id INTEGER REFERENCES item(i_id));" \
    "CREATE INDEX name_index ON item(name);" \
    "CREATE INDEX i_id_index ON item_data(i_id);" \
    "CREATE TABLE config (" \
    "   key TEXT PRIMARY KEY," \
    "   value TEXT NOT NULL);" \
    "INSERT INTO config values(" \
    "   'version'," \
    "   '1.0');"

#define S_OUTPUT "SELECT msg FROM OUTPUT WHERE T_ID=@tid and type=@type"

#define I_OUTPUT "insert into OUTPUT values(null,@tid,@msg,@type)"

#endif
