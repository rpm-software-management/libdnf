/* dnf-swdb-trans-sql.h
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

#ifndef _DNF_SWDB_TRANS_SQL
#define _DNF_SWDB_TRANS_SQL

#define S_TRANS_DATA_BY_TID "SELECT * FROM TRANS_DATA WHERE T_ID=@tid"
#define S_TRANS_WITH        "SELECT P_ID FROM TRANS_WITH WHERE T_ID=@tid"

#endif
