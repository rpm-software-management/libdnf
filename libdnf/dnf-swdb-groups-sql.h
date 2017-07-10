/* dnf-swdb-groups-sql.h
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

#ifndef _DNF_SWDB_GROUPS_SQL
#define _DNF_SWDB_GROUPS_SQL

#define I_GROUP \
    "Insert into GROUPS values(null, @name_id, @name, @ui_name, @is_installed, @pkg_types)"
#define I_ENV \
    "Insert into ENVIRONMENTS values(null, @name_id, @name, @ui_name, @pkg_types, @grp_types)"
#define I_ENV_GROUP "insert into ENVIRONMENTS_GROUPS values(null, @eid, @gid)"
#define I_TRANS_GROUP_DATA \
    "insert into TRANS_GROUP_DATA values(null,@tid,@gid,@name_id,@name,@ui_name," \
    "@is_installed,@pkg_types)"

#define S_GID_BY_NAME_ID "Select G_ID from GROUPS where name_id LIKE @id"
#define S_GROUP_BY_NAME_ID "select * from GROUPS where name_id LIKE @id"
#define S_ENV_BY_NAME_ID "select * from ENVIRONMENTS where name_id LIKE @id"
#define S_GROUPS_BY_PATTERN \
    "SELECT * from GROUPS where name_id LIKE @pat or name LIKE @pat or ui_name LIKE @pat"
#define S_ENV_BY_PATTERN \
    "SELECT * from ENVIRONMENTS where name_id LIKE @pat or name LIKE @pat or ui_name LIKE @pat"
#define S_ENV_EXCLUDE_BY_ID "SELECT name FROM ENVIRONMENTS_EXCLUDE where E_ID=@eid"
#define S_IS_INSTALLED_BY_EID \
    "SELECT E_ID FROM ENVIRONMENTS_GROUPS join GROUPS using(G_ID) where E_ID=@eid and " \
    "is_installed=1"
#define S_GROUP_NAME_ID_BY_EID \
    "SELECT name_id FROM ENVIRONMENTS_GROUPS join GROUPS using(G_ID) where E_ID=@eid"

#define U_GROUP_COMMIT "UPDATE GROUPS SET is_installed=1 where name_id=@id"
#define U_GROUP \
    "UPDATE GROUPS SET " \
    "name=@name,ui_name=@ui_name,is_installed=@is_installed,pkg_types=@pkg_types where G_ID=@gid"

#define U_ENV \
    "UPDATE ENVIRONMENTS SET " \
    "name=@name,ui_name=@ui_name,pkg_types=@pkg_types,grp_types=@grp_types where E_ID=@eid"

#define R_FULL_LIST_BY_ID "DELETE FROM GROUPS_PACKAGE WHERE G_ID=@gid"

#define S_REM_REASON \
    "SELECT description from PACKAGE join PACKAGE_DATA using (P_ID) join TRANS_DATA using(PD_ID) " \
    "join REASON_TYPE using(reason) where name=@name ORDER BY TD_ID DESC"

#define S_REM \
    "SELECT G_ID from GROUPS_PACKAGE join GROUPS using(G_ID) where is_installed=1 and " \
    "GROUPS_PACKAGE.name=@name"

#endif
