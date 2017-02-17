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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "dnf-swdb-db.h"
#include "dnf-swdb.h"
#include "dnf-swdb-db-sql.h"

gint _create_db(sqlite3 *db)
{
    gint failed = 0;
    failed += _db_exec (db, C_PKG_DATA, NULL);
    failed += _db_exec (db, C_PKG, NULL);
    failed += _db_exec (db, C_REPO, NULL);
    failed += _db_exec (db, C_TRANS_DATA, NULL);
    failed += _db_exec (db, C_TRANS, NULL);
    failed += _db_exec (db, C_OUTPUT, NULL);
    failed += _db_exec (db, C_STATE_TYPE, NULL);
    failed += _db_exec (db, C_REASON_TYPE, NULL);
    failed += _db_exec (db, C_OUTPUT_TYPE, NULL);
    failed += _db_exec (db, C_PKG_TYPE, NULL);
    failed += _db_exec (db, C_GROUPS, NULL);
    failed += _db_exec (db, C_T_GROUP_DATA, NULL);
    failed += _db_exec (db, C_GROUPS_PKG, NULL);
    failed += _db_exec (db, C_GROUPS_EX, NULL);
    failed += _db_exec (db, C_ENV_GROUPS, NULL);
    failed += _db_exec (db, C_ENV, NULL);
    failed += _db_exec (db, C_ENV_EX, NULL);
    failed += _db_exec (db, C_RPM_DATA, NULL);
    failed += _db_exec (db, C_INDEX_NVRA, NULL);
    return failed;
}

gint _db_step(sqlite3_stmt *res)
{
    if (sqlite3_step(res) != SQLITE_DONE)
    {
        fprintf(stderr, "SQL error: Statement execution failed!\n");
        sqlite3_finalize(res);
        return 0;
    }
    sqlite3_finalize(res);
    return 1; //true because of assert
}

// assumes only one parameter on output e.g "SELECT ID FROM DUMMY WHERE ..."
gint _db_find(sqlite3_stmt *res)
{
    if (sqlite3_step(res) == SQLITE_ROW ) // id for description found
    {
        gint result = sqlite3_column_int(res, 0);
        sqlite3_finalize(res);
        return result;
    }
    else
    {
        sqlite3_finalize(res);
        return 0;
    }
}

// assumes one text parameter on output e.g "SELECT DESC FROM DUMMY WHERE ..."
gchar *_db_find_str(sqlite3_stmt *res)
{
    if (sqlite3_step(res) == SQLITE_ROW ) // id for description found
    {
        gchar * result = g_strdup((gchar *)sqlite3_column_text(res, 0));
        sqlite3_finalize(res);
        return result;
    }
    else
    {
        sqlite3_finalize(res);
        return NULL;
    }
}

gchar *_db_find_str_multi(sqlite3_stmt *res)
{
    if (sqlite3_step(res) == SQLITE_ROW ) // id for description found
    {
        gchar * result = g_strdup((gchar *)sqlite3_column_text(res, 0));
        return result;
    }
    else
    {
        sqlite3_finalize(res);
        return NULL;
    }
}

gint _db_find_multi	(sqlite3_stmt *res)
{
    if (sqlite3_step(res) == SQLITE_ROW ) // id for description found
    {
        gint result = sqlite3_column_int(res, 0);
        return result;
    }
    else
    {
        sqlite3_finalize(res);
        return 0;
    }
}

gint _db_prepare(sqlite3 *db, const gchar *sql, sqlite3_stmt **res)
{
   gint rc = sqlite3_prepare_v2(db, sql, -1, res, NULL);
    if(rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
        fprintf(stderr, "SQL error: Prepare failed!\n");
        sqlite3_finalize(*res);
        return 0;
    }
    return 1; //true because of assert
}

gint _db_bind(sqlite3_stmt *res, const gchar *id, const gchar *source)
{
    gint idx = sqlite3_bind_parameter_index(res, id);
    gint rc = sqlite3_bind_text(res, idx, source, -1, SQLITE_STATIC);

    if (rc) //something went wrong
    {
        fprintf(stderr, "SQL error: Could not bind parameters(%d|%s|%s)\n",
                idx, id, source);
        sqlite3_finalize(res);
        return 0;
    }
    return 1; //true because of assert
}

gint _db_bind_int (sqlite3_stmt *res, const gchar *id, gint source)
{
    gint idx = sqlite3_bind_parameter_index(res, id);
    gint rc = sqlite3_bind_int(res, idx, source);

    if (rc) //something went wrong
    {
        fprintf(stderr, "SQL error: Could not bind parameters(%s|%d)\n",
                id, source);
        sqlite3_finalize(res);
        return 0;
    }
    return 1; //true because of assert
}

//use macros DB_[PREP,BIND,STEP] for parametrised queries
gint _db_exec(sqlite3 *db,
              const gchar *cmd,
              int (*callback)(void *, int, char **, char**))
{
    gchar *err_msg;
    gint result = sqlite3_exec(db, cmd, callback, 0, &err_msg);
    if ( result != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 1;
    }
    return 0;
}

// Pattern on input, transaction IDs on output
GArray *_simple_search(sqlite3* db, const gchar * pattern)
{
    GArray *tids = g_array_new(0,0,sizeof(gint));
    sqlite3_stmt *res_simple;
    const gchar *sql_simple = SIMPLE_SEARCH;
    DB_PREP(db, sql_simple, res_simple);
    DB_BIND(res_simple, "@pat", pattern);
    gint pid_simple;
    GArray *simple = g_array_new(0, 0, sizeof(gint));
    while( (pid_simple = DB_FIND_MULTI(res_simple)))
    {
        g_array_append_val(simple, pid_simple);
    }
    gint pdid;
    for(guint i = 0; i < simple->len; i++)
    {
        pid_simple = g_array_index(simple, gint, i);
        GArray *pdids = _all_pdid_for_pid(db, pid_simple);
        for( guint j = 0; j < pdids->len; j++)
        {
            pdid = g_array_index(pdids, gint, j);
            GArray *tids_for_pdid = _tids_from_pdid(db, pdid);
            tids = g_array_append_vals (tids, tids_for_pdid->data,
                                        tids_for_pdid->len);
            g_array_free(tids_for_pdid, TRUE);
        }
        g_array_free(pdids, TRUE);
    }
    g_array_free(simple, TRUE);
    return tids;
}

/*
* Provides package search with extended pattern
*/
GArray *_extended_search (sqlite3* db, const gchar *pattern)
{
    GArray *tids = g_array_new(0, 0, sizeof(gint));
    sqlite3_stmt *res;
    const gchar *sql = SEARCH_PATTERN;
    DB_PREP(db, sql, res);
    //lot of patterns...
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    GArray *pids = g_array_new(0, 0, sizeof(gint));
    gint pid;
    while(sqlite3_step(res) == SQLITE_ROW)
    {
        pid = sqlite3_column_int(res, 0);
        g_array_append_val(pids, pid);
    }
    sqlite3_finalize(res);
    gint pdid;
    for(guint j = 0; j < pids->len; ++j)
    {
        pid = g_array_index(pids, gint, j);
        GArray *pdids = _all_pdid_for_pid(db, pid);
        for(guint i = 0; i < pdids->len; ++i)
        {
            pdid = g_array_index(pdids, gint, i);
            GArray *tids_for_pdid = _tids_from_pdid(db, pdid);
            tids = g_array_append_vals (tids, tids_for_pdid->data,
                                        tids_for_pdid->len);
            g_array_free(tids_for_pdid, TRUE);
        }
        g_array_free(pdids, TRUE);
    }
    g_array_free(pids, TRUE);
    return tids;
}
