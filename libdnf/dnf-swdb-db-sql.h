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

#define SIMPLE_SEARCH "SELECT P_ID FROM PACKAGE WHERE name LIKE @pat"

#define SEARCH_PATTERN \
    "SELECT P_ID,name,epoch,version,release,arch," \
    "name || '.' || arch AS sql_na," \
    "name || '-' || version || '-' || release || '.' || arch AS sql_nvra," \
    "name || '-' || version AS sql_nv," \
    "name || '-' || version || '-' || release AS sql_nvr," \
    "epoch || ':' || name || '-' || version || '-' || release || '-' || arch AS sql_envra," \
    "name || '-' || epoch || '-' || version || '-' || release || '-' || arch AS sql_nevra " \
    "FROM PACKAGE WHERE sql_na LIKE @pat OR sql_nvra LIKE @pat OR sql_nv LIKE @pat OR" \
    " sql_nvr LIKE @pat OR sql_nevra LIKE @pat OR sql_envra LIKE @pat"

#define FIND_TIDS_FROM_PDID "SELECT T_ID FROM TRANS_DATA WHERE PD_ID=@pdid"
#define FIND_ALL_PDID_FOR_PID "SELECT PD_ID FROM PACKAGE_DATA WHERE P_ID=@pid"

#define CREATE_TABLES \
    "CREATE TABLE PACKAGE_DATA (PD_ID INTEGER PRIMARY KEY, P_ID INTEGER, R_ID INTEGER," \
    "   from_repo_revision TEXT, from_repo_timestamp TEXT, installed_by TEXT, changed_by TEXT," \
    "   installonly TEXT, origin_url TEXT);" \
    "CREATE TABLE PACKAGE ( P_ID INTEGER PRIMARY KEY, name TEXT, epoch INTEGER, version TEXT, " \
    "   release TEXT, arch TEXT, checksum_data TEXT, checksum_type TEXT, type INTEGER);" \
    "CREATE TABLE REPO (R_ID INTEGER PRIMARY KEY, name TEXT, last_synced TEXT, is_expired TEXT);" \
    "CREATE TABLE TRANS_DATA (TD_ID INTEGER PRIMARY KEY, T_ID INTEGER,PD_ID INTEGER, " \
    "   TG_ID INTEGER, done INTEGER, ORIGINAL_TD_ID INTEGER, reason INTEGER, state INTEGER);" \
    "CREATE TABLE TRANS (T_ID INTEGER PRIMARY KEY, beg_timestamp TEXT, end_timestamp TEXT, " \
    "   beg_RPMDB_version TEXT, end_RPMDB_version ,cmdline TEXT,loginuid TEXT, releasever TEXT, " \
    "   return_code INTEGER);" \
    "CREATE TABLE OUTPUT (O_ID INTEGER PRIMARY KEY,T_ID INTEGER, msg TEXT, type INTEGER);" \
    "CREATE TABLE STATE_TYPE (state INTEGER PRIMARY KEY, description TEXT);" \
    "CREATE TABLE REASON_TYPE (reason INTEGER PRIMARY KEY, description TEXT);" \
    "CREATE TABLE OUTPUT_TYPE (type INTEGER PRIMARY KEY, description TEXT);" \
    "CREATE TABLE PACKAGE_TYPE (type INTEGER PRIMARY KEY, description TEXT);" \
    "CREATE TABLE GROUPS (G_ID INTEGER PRIMARY KEY, name_id TEXT, name TEXT, ui_name TEXT, " \
    "   installed INTEGER, pkg_types INTEGER);" \
    "CREATE TABLE TRANS_GROUP_DATA (TG_ID INTEGER PRIMARY KEY, T_ID INTEGER, G_ID INTEGER, " \
    "   name_id TEXT, name TEXT, ui_name TEXT,installed INTEGER, pkg_types INTEGER);" \
    "CREATE TABLE GROUPS_PACKAGE (GP_ID INTEGER PRIMARY KEY, G_ID INTEGER, name TEXT);" \
    "CREATE TABLE GROUPS_EXCLUDE (GE_ID INTEGER PRIMARY KEY, G_ID INTEGER, name TEXT);" \
    "CREATE TABLE ENVIRONMENTS_GROUPS (EG_ID INTEGER PRIMARY KEY, E_ID INTEGER, G_ID INTEGER);" \
    "CREATE TABLE ENVIRONMENTS (E_ID INTEGER PRIMARY KEY, name_id TEXT, name TEXT, ui_name TEXT, " \
    "   pkg_types INTEGER, grp_types INTEGER);" \
    "CREATE TABLE ENVIRONMENTS_EXCLUDE (EE_ID INTEGER PRIMARY KEY, E_ID INTEGER, name TEXT);" \
    "CREATE TABLE RPM_DATA (RPM_ID INTEGER PRIMARY KEY, P_ID INTEGER, buildtime TEXT, buildhost " \
    "   TEXT, license TEXT, packager TEXT, size TEXT, sourcerpm TEXT, url TEXT, vendor TEXT, " \
    "   committer TEXT, committime TEXT);" \
    "CREATE TABLE TRANS_WITH (TW_ID INTEGER PRIMARY KEY, T_ID INTEGER, P_ID INTEGER);" \
    "CREATE INDEX nevra ON PACKAGE (name || '-' || epoch || ':' || version || '-' || release ||" \
    "   '.' || arch);" \
    "CREATE INDEX nvra on PACKAGE (name || '-' || version || '-' || release || '.' || arch);"

#define S_OUTPUT "SELECT msg FROM OUTPUT WHERE T_ID=@tid and type=@type"

#define I_OUTPUT "insert into OUTPUT values(null,@tid,@msg,@type)"

#endif
