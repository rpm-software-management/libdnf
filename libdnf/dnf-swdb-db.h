/* dnf-swdb-db.c
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

#ifndef DNF_SWDB_DB_H
#define DNF_SWDB_DB_H

#include <glib-object.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <sqlite3.h>

gint _create_db(sqlite3 *db);

gint _db_step(sqlite3_stmt *res);

gint _db_find(sqlite3_stmt *res);

gchar *_db_find_str(sqlite3_stmt *res);

gchar *_db_find_str_multi(sqlite3_stmt *res);

gint _db_find_multi(sqlite3_stmt *res);

gint _db_prepare(sqlite3 *db, const gchar *sql, sqlite3_stmt **res);

gint _db_bind(sqlite3_stmt *res, const gchar *id, const gchar *source);

gint _db_bind_int(sqlite3_stmt *res, const gchar *id, gint source);

gint _db_exec(sqlite3 *db,
              const gchar *cmd,
              int (*callback)(void *, int, char **, char**));

const gchar *_table_by_attribute(const gchar *attr);

GArray *_simple_search(sqlite3* db, const gchar * pattern);

GArray *_extended_search (sqlite3* db, const gchar *pattern);

#endif
