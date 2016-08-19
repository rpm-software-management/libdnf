/* hif-swdb-sql.h
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

#ifndef _HIF_SWDB_SQL
#define _HIF_SWDB_SQL

#define INSERT_PKG "insert into PACKAGE values(null,@name,@epoch,@version,@release,@arch,@cdata,@ctype,@type)"
#define INSERT_OUTPUT "insert into OUTPUT values(null,@tid,@msg,@type)"
#define INSERT_TRANS_BEG "insert into TRANS values(null,@beg,null,@rpmdbv,@cmdline,@loginuid,@releasever,null)"
#define INSERT_TRANS_END "UPDATE TRANS SET end_timestamp=@end,return_code=@rc WHERE T_ID=@tid"
#define INSERT_REPO "insert into REPO values(null,@name,null,null)"

#define UPDATE_PKG_DATA "UPDATE PACKAGE_DATA SET R_ID=@rid,from_repo_revision=@repo_r,from_repo_timestamp=@repo_t,"\
                        "installed_by=@installed_by,changed_by=@changed_by,installonly=@installonly,"\
                        "origin_url=@origin_url where P_ID=@pid"

#define INSERT_RPM_DATA "INSERT into RPM_DATA values(null,@pid,@buildtime,@buildhost,@license,@packager,@size,@sourcerpm,@url,"\
                        "@vendor,@committer,@committime)"

#define INSERT_TRANS_DATA_BEG "insert into TRANS_DATA values(null,@tid,@pdid,null,@done,null,@reason,@state)"
#define UPDATE_TRANS_DATA_END "UPDATE TRANS_DATA SET done=@done WHERE T_ID=@tid"
#define UPDATE_TRANS_DATA_PID_END "UPDATE TRANS_DATA SET done=@done WHERE T_ID=@tid and PD_ID=@pdid and state=@state"

#define FIND_REPO_BY_NAME "SELECT R_ID FROM REPO WHERE name=@name"
#define FIND_PDID_FROM_PID "SELECT PD_ID FROM PACKAGE_DATA WHERE P_ID=@pid ORDER by PD_ID DESC"
#define FIND_ALL_PDID_FOR_PID "SELECT PD_ID FROM PACKAGE_DATA WHERE P_ID=@pid"
#define GET_TRANS_CMDLINE "SELECT cmdline FROM TRANS WHERE T_ID=@tid"

#define INSERT_PDID "insert into PACKAGE_DATA values(null,@pid,null,null,null,null,null,null,null)"
#define FIND_TID_FROM_PDID "SELECT T_ID FROM TRANS_DATA WHERE PD_ID=@pdid"
#define LOAD_OUTPUT "SELECT msg FROM OUTPUT WHERE T_ID=@tid and type=@type"
#define PKG_DATA_ATTR_BY_PID "FROM PACKAGE_DATA WHERE P_ID=@pid"
#define TRANS_DATA_ATTR_BY_PDID "FROM TRANS_DATA WHERE PD_ID=@pdid"
#define TRANS_ATTR_BY_TID "FROM TRANS WHERE T_ID=@tid"
#define RPM_ATTR_BY_PID    "FROM RPM_DATA WHERE P_ID=@pid"
#define PID_BY_TID  "select P_ID from TRANS_DATA join PACKAGE_DATA using(PD_ID) where T_ID=@tid"

#define FIND_PKG_BY_NEVRA   "SELECT P_ID FROM PACKAGE WHERE name=@name and epoch=@epoch and version=@version and release=@release"\
                            " and arch=@arch and @type=type"
#define FIND_PKG_BY_NEVRACHT "SELECT P_ID FROM PACKAGE WHERE name=@name and epoch=@epoch and version=@version and release=@release"\
                            " and arch=@arch and @type=type and checksum_data=@cdata and checksum_type=@ctype"

#define FIND_PIDS_BY_NAME "SELECT P_ID FROM PACKAGE WHERE NAME LIKE @pattern"

#define SIMPLE_SEARCH "SELECT P_ID FROM PACKAGE WHERE name LIKE @pat"
#define SEARCH_PATTERN "SELECT P_ID,name,epoch,version,release,arch,"\
  "name || '.' || arch AS sql_nameArch,"\
  "name || '-' || version || '-' || release || '.' || arch AS sql_nameVerRelArch,"\
  "name || '-' || version AS sql_nameVer,"\
  "name || '-' || version || '-' || release AS sql_nameVerRel,"\
  "epoch || ':' || name || '-' || version || '-' || release || '-' || arch AS sql_envra,"\
  "name || '-' || epoch || '-' || version || '-' || release || '-' || arch AS sql_nevra "\
  "FROM PACKAGE WHERE name LIKE @sub AND (sql_nameArch LIKE @pat OR sql_nameVerRelArch LIKE @pat OR sql_nameVer LIKE @pat OR"\
  " sql_nameVerRel LIKE @pat OR sql_nevra LIKE @pat OR sql_envra LIKE @pat)"


#define S_PACKAGE_BY_PID "SELECT name,epoch,version,release,arch,checksum_data,checksum_type,type FROM PACKAGE WHERE P_ID=@pid"

#define S_TRANS "SELECT * from TRANS ORDER BY T_ID DESC"
#define S_TRANS_W_LIMIT "SELECT * from TRANS ORDER BY T_ID DESC LIMIT @limit"
#define S_TRANS_COMP "SELECT * from TRANS WHERE end_timestamp is not null or end_timestamp!='' ORDER BY T_ID DESC"
#define S_TRANS_COMP_W_LIMIT "SELECT * from TRANS WHERE end_timestamp is not null or end_timestamp!='' ORDER BY T_ID DESC LIMIT @limit"

#define S_TRANS_DATA_BY_TID "SELECT * FROM TRANS_DATA WHERE T_ID=@tid"
//CREATION OF tables

#define C_PKG_DATA 		"CREATE TABLE PACKAGE_DATA ( PD_ID integer PRIMARY KEY,"\
                    	"P_ID integer, R_ID integer, from_repo_revision text,"\
                    	"from_repo_timestamp text, installed_by text, changed_by text,"\
                    	"installonly text, origin_url text)"

#define C_PKG 			"CREATE TABLE PACKAGE ( P_ID integer primary key, name text,"\
						"epoch text, version text, release text, arch text,"\
						"checksum_data text, checksum_type text, type integer)"

#define C_REPO			"CREATE TABLE REPO (R_ID INTEGER PRIMARY KEY, name text,"\
                    	"last_synced text, is_expired text)"

#define C_TRANS_DATA	"CREATE TABLE TRANS_DATA (TD_ID INTEGER PRIMARY KEY,"\
                        "T_ID integer,PD_ID integer, G_ID integer, done INTEGER,"\
                        "ORIGINAL_TD_ID integer, reason integer, state integer)"

#define C_TRANS 		"CREATE TABLE TRANS (T_ID integer primary key, beg_timestamp text,"\
                        "end_timestamp text, RPMDB_version text, cmdline text,"\
                        "loginuid text, releasever text, return_code integer)"

#define C_OUTPUT		"CREATE TABLE OUTPUT (O_ID integer primary key,T_ID INTEGER,"\
						"msg text, type integer)"


#define C_STATE_TYPE	"CREATE TABLE STATE_TYPE (ID INTEGER PRIMARY KEY, description text)"
#define C_REASON_TYPE	"CREATE TABLE REASON_TYPE (ID INTEGER PRIMARY KEY, description text)"
#define C_OUTPUT_TYPE	"CREATE TABLE OUTPUT_TYPE (ID INTEGER PRIMARY KEY, description text)"
#define C_PKG_TYPE		"CREATE TABLE PACKAGE_TYPE (ID INTEGER PRIMARY KEY, description text)"

#define C_GROUPS		"CREATE TABLE GROUPS (G_ID INTEGER PRIMARY KEY, name_id text, name text,"\
                        "ui_name text, is_installed integer, pkg_types integer, grp_types integer)"

#define C_T_GROUP_DATA	"CREATE TABLE TRANS_GROUP_DATA (TG_ID INTEGER PRIMARY KEY,"\
						"T_ID integer, G_ID integer, name_id text, name text, ui_name text,"\
                        "is_installed integer, pkg_types integer, grp_types integer)"

#define C_GROUPS_PKG	"CREATE TABLE GROUPS_PACKAGE (GP_ID INTEGER PRIMARY KEY,"\
                        "G_ID integer, name text)"

#define C_GROUPS_EX		"CREATE TABLE GROUPS_EXCLUDE (GE_ID INTEGER PRIMARY KEY,"\
                        "G_ID integer, name text)"

#define C_ENV_GROUPS	"CREATE TABLE ENVIRONMENTS_GROUPS (EG_ID INTEGER PRIMARY KEY,"\
                        "E_ID integer, G_ID integer)"

#define C_ENV			"CREATE TABLE ENVIRONMENTS (E_ID INTEGER PRIMARY KEY, name_id text,"\
                        "name text, ui_name text, pkg_types integer, grp_types integer)"

#define C_ENV_EX		"CREATE TABLE ENVIRONMENTS_EXCLUDE (EE_ID INTEGER PRIMARY KEY,"\
                        "E_ID integer, name text)"
#define C_RPM_DATA      "CREATE TABLE RPM_DATA (RPM_ID INTEGER PRIMARY KEY, P_ID INTEGER,"\
                        "buildtime TEXT, buildhost TEXT, license TEXT, packager TEXT, size TEXT,"\
                        "sourcerpm TEXT, url TEXT, vendor TEXT, committer TEXT, committime TEXT)"

#endif
