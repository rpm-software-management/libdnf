/* dnf-swdb-sql.h
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

#ifndef _DNF_SWDB_SQL
#define _DNF_SWDB_SQL

#define I_DESC_STATE "INSERT INTO state_type   VALUES (null, @desc)"
#define I_DESC_OUTPUT "INSERT INTO output_type VALUES (null, @desc)"
#define I_TRANS_WITH "INSERT INTO TRANS_WITH   VALUES (null, @tid, @pid)"

#define I_ITEM_DATA \
    "INSERT INTO item_data VALUES (null, @iid, @tid, @rid, @tgid, @done, @obsoleting, @reason, " \
    "@state)"

#define INSERT_PKG \
    "insert into ITEM values(null,@name,@epoch,@version,@release,@arch,@cdata,@ctype,@type)"
#define INSERT_TRANS_BEG \
    "insert into TRANS values(null,@beg,null,@rpmdbv,null,@cmdline,@loginuid,@releasever,null)"
#define INSERT_TRANS_END \
    "UPDATE TRANS SET end_timestamp=@end,end_RPMDB_version=@rpmdbv, return_code=@rc WHERE " \
    "T_ID=@tid"
#define INSERT_REPO "insert into REPO values(null,@name)"

#define UPDATE_ITEM_DATA_IID_END \
    "UPDATE ITEM_DATA SET done=@done WHERE T_ID=@tid and ID_ID=@idid and state=@state"

#define FIND_REPO_BY_NAME "SELECT R_ID FROM REPO WHERE name=@name"
#define FIND_IDID_FROM_IID "SELECT ID_ID FROM ITEM_DATA WHERE I_ID=@iid ORDER by ID_ID DESC LIMIT 1"
#define GET_TRANS_CMDLINE "SELECT cmdline FROM TRANS WHERE T_ID=@tid"

#define IID_BY_TID "select I_ID from ITEM_DATA where T_ID=@tid"

#define RESOLVE_GROUP_TRANS \
    "SELECT TG_ID FROM ITEM join GROUPS_PACKAGE using (name) join TRANS_GROUP_DATA using " \
    "(G_ID) WHERE I_ID=@pid and T_ID=@tid"

#define S_REASON_BY_IID "SELECT reason FROM ITEM_DATA WHERE I_ID=@iid ORDER by ID_ID DESC LIMIT 1"

#define S_REASON_BY_NEVRA \
    "SELECT reason FROM ITEM join ITEM_DATA WHERE name=@n and epoch=@e and version=@v and " \
    "release=@r and arch=@a ORDER by TD_ID DESC LIMIT 1"

#define S_ITEM_DATA_BY_IID \
    "SELECT ID_ID, I_ID, T_ID, repo.name, TG_ID, done, obsoleting, reason, " \
    "state_type.description " \
    "FROM ITEM_DATA join REPO using(R_ID) join state_type using(state) WHERE I_ID=@iid ORDER BY " \
    "ID_ID DESC"

#define S_REPO_BY_RID "select name from REPO where R_ID=@rid"

#define S_TRANS "SELECT * from TRANS ORDER BY T_ID DESC"
#define S_TRANS_W_LIMIT "SELECT * from TRANS ORDER BY T_ID DESC LIMIT @limit"
#define S_TRANS_COMP \
    "SELECT * from TRANS WHERE end_timestamp is not null or end_timestamp!=0 ORDER BY T_ID DESC"
#define S_TRANS_COMP_W_LIMIT \
    "SELECT * from TRANS WHERE end_timestamp is not null or end_timestamp!=0 ORDER BY T_ID DESC " \
    "LIMIT @limit"

#define S_PACKAGE_STATE \
    "select done, state, obsoleting from ITEM_DATA where I_ID=@iid and T_ID=@tid"

#define S_REPO_FROM_PID2 "SELECT name FROM ITEM_DATA join REPO using(R_ID) where I_ID=@iid"

#define S_CHECKSUM_BY_NEVRA \
    "SELECT checksum_data, checksum_type FROM ITEM WHERE name=@n and epoch=@e and version=@v " \
    "and release=@r and arch=@a"

#define S_IID_BY_NEVRA \
    "SELECT I_ID FROM ITEM WHERE name=@n and epoch=@e and version=@v and release=@r and " \
    "arch=@a"

#define S_STATE_TYPE_ID "SELECT state  FROM state_type   WHERE description=@desc"
#define S_OUTPUT_TYPE_ID "SELECT type   FROM output_type  WHERE description=@desc"

#define S_STATE_TYPE_BY_ID "SELECT description FROM state_type   WHERE state=@id"
#define S_OUTPUT_TYPE_BY_ID "SELECT description FROM output_type  WHERE type=@id"

#define S_LATEST_PACKAGE "SELECT i_id FROM item WHERE name=@name ORDER BY i_id DESC LIMIT 1"

#define S_ERASED_REASON "SELECT reason FROM ITEM_DATA WHERE T_ID=@tid and I_ID=@iid"

#define U_REASON_BY_IDID "UPDATE ITEM_DATA SET reason=@reason where ID_ID=@idid"

#define U_REPO_BY_IID "UPDATE ITEM_DATA SET R_ID=@rid where I_ID=@iid"

#define T_OUTPUT "SELECT O_ID FROM OUTPUT WHERE T_ID=@tid and type=@type"

#endif
