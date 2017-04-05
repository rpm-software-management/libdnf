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

#define SEARCH_PATTERN "SELECT P_ID,name,epoch,version,release,arch,"\
    "name || '.' || arch AS sql_na,"\
    "name || '-' || version || '-' || release || '.' || arch AS sql_nvra,"\
    "name || '-' || version AS sql_nv,"\
    "name || '-' || version || '-' || release AS sql_nvr,"\
    "epoch || ':' || name || '-' || version || '-' || release || '-' || arch AS sql_envra,"\
    "name || '-' || epoch || '-' || version || '-' || release || '-' || arch AS sql_nevra "\
    "FROM PACKAGE WHERE sql_na LIKE @pat OR sql_nvra LIKE @pat OR sql_nv LIKE @pat OR"\
    " sql_nvr LIKE @pat OR sql_nevra LIKE @pat OR sql_envra LIKE @pat"

#define C_PKG_DATA  "CREATE TABLE PACKAGE_DATA ( PD_ID integer PRIMARY KEY,"\
                    "P_ID integer, R_ID integer, from_repo_revision text,"\
                    "from_repo_timestamp text, installed_by text, changed_by text,"\
                    "installonly text, origin_url text)"

#define C_PKG "CREATE TABLE PACKAGE ( P_ID integer primary key, name text,"\
              "epoch text, version text, release text, arch text,"\
              "checksum_data text, checksum_type text, type integer)"

#define C_REPO  "CREATE TABLE REPO (R_ID INTEGER PRIMARY KEY, name text,"\
                "last_synced text, is_expired text)"

#define C_TRANS_DATA "CREATE TABLE TRANS_DATA (TD_ID INTEGER PRIMARY KEY,"\
                     "T_ID integer,PD_ID integer, TG_ID integer, done INTEGER,"\
                     "ORIGINAL_TD_ID integer, reason integer, state integer)"

#define C_TRANS "CREATE TABLE TRANS (T_ID integer primary key, beg_timestamp text,"\
                "end_timestamp text, beg_RPMDB_version text, end_RPMDB_version ,cmdline text,"\
                "loginuid text, releasever text, return_code integer)"

#define C_OUTPUT "CREATE TABLE OUTPUT (O_ID integer primary key,T_ID INTEGER,"\
                 "msg text, type integer)"


#define C_STATE_TYPE  "CREATE TABLE STATE_TYPE (state INTEGER PRIMARY KEY, description text)"
#define C_REASON_TYPE "CREATE TABLE REASON_TYPE (reason INTEGER PRIMARY KEY, description text)"
#define C_OUTPUT_TYPE "CREATE TABLE OUTPUT_TYPE (type INTEGER PRIMARY KEY, description text)"
#define C_PKG_TYPE    "CREATE TABLE PACKAGE_TYPE (type INTEGER PRIMARY KEY, description text)"

#define C_GROUPS "CREATE TABLE GROUPS (G_ID INTEGER PRIMARY KEY, name_id text, name text,"\
                 "ui_name text, is_installed integer, pkg_types integer)"

#define C_T_GROUP_DATA  "CREATE TABLE TRANS_GROUP_DATA (TG_ID INTEGER PRIMARY KEY,"\
                        "T_ID integer, G_ID integer, name_id text, name text, ui_name text,"\
                        "is_installed integer, pkg_types integer)"

#define C_GROUPS_PKG "CREATE TABLE GROUPS_PACKAGE (GP_ID INTEGER PRIMARY KEY,"\
                     "G_ID integer, name text)"

#define C_GROUPS_EX  "CREATE TABLE GROUPS_EXCLUDE (GE_ID INTEGER PRIMARY KEY,"\
                     "G_ID integer, name text)"

#define C_ENV_GROUPS "CREATE TABLE ENVIRONMENTS_GROUPS (EG_ID INTEGER PRIMARY KEY,"\
                     "E_ID integer, G_ID integer)"

#define C_ENV "CREATE TABLE ENVIRONMENTS (E_ID INTEGER PRIMARY KEY, name_id text,"\
                        "name text, ui_name text, pkg_types integer, grp_types integer)"

#define C_ENV_EX "CREATE TABLE ENVIRONMENTS_EXCLUDE (EE_ID INTEGER PRIMARY KEY,"\
                 "E_ID integer, name text)"

#define C_RPM_DATA "CREATE TABLE RPM_DATA (RPM_ID INTEGER PRIMARY KEY, P_ID INTEGER,"\
                   "buildtime TEXT, buildhost TEXT, license TEXT, packager TEXT, size TEXT,"\
                   "sourcerpm TEXT, url TEXT, vendor TEXT, committer TEXT, committime TEXT)"

#define C_INDEX_NVRA    "create index nvra on PACKAGE(name || '-' || version || '-' || release || '.' || arch)"

#endif
