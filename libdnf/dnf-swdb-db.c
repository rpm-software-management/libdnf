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

#include "dnf-swdb-db.h"
#include "dnf-swdb-db-sql.h"
#include "dnf-swdb.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * _create_db:
 * @db: sqlite database handle
 *
 * Create empty database
 *
 * Returns: 0 if successful else count of tables failed to create
 **/
gint
_create_db (sqlite3 *db)
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
    failed += _db_exec (db, C_TRANS_WITH, NULL);
    failed += _db_exec (db, C_INDEX_NEVRA, NULL);
    failed += _db_exec (db, C_INDEX_NVRA, NULL);
    return failed;
}

/**
 * _db_step:
 * @res: sqlite statement
 *
 * Execute statement @res
 *
 * Returns: 1 if successful
 **/
gint
_db_step (sqlite3_stmt *res)
{
    if (sqlite3_step (res) != SQLITE_DONE) {
        fprintf (stderr, "SQL error: Statement execution failed!\n");
        sqlite3_finalize (res);
        return 0;
    }
    sqlite3_finalize (res);
    return 1; // true because of assert
}

/**
 * _db_find:
 * @res: sqlite statement
 *
 * Returns: first integral result of statement @res or 0 if error occurred
 **/
gint
_db_find (sqlite3_stmt *res)
{
    if (sqlite3_step (res) == SQLITE_ROW) // id for description found
    {
        gint result = sqlite3_column_int (res, 0);
        sqlite3_finalize (res);
        return result;
    } else {
        sqlite3_finalize (res);
        return 0;
    }
}

/**
 * _db_find_str:
 * @res: sqlite statement
 *
 * Returns: first string result of statement @res or %NULL if error occurred
 **/
gchar *
_db_find_str (sqlite3_stmt *res)
{
    if (sqlite3_step (res) == SQLITE_ROW) // id for description found
    {
        gchar *result = g_strdup ((gchar *)sqlite3_column_text (res, 0));
        sqlite3_finalize (res);
        return result;
    } else {
        sqlite3_finalize (res);
        return NULL;
    }
}

/**
 * _db_find_str_multi:
 * @res: sqlite statement
 *
 * String result iterator
 *
 * Returns: next string result or %NULL if search hit the bottom
 **/
gchar *
_db_find_str_multi (sqlite3_stmt *res)
{
    if (sqlite3_step (res) == SQLITE_ROW) // id for description found
    {
        gchar *result = g_strdup ((gchar *)sqlite3_column_text (res, 0));
        return result;
    } else {
        sqlite3_finalize (res);
        return NULL;
    }
}

/**
 * _db_find_multi:
 * @res: sqlite statement
 *
 * Integral result iterator
 *
 * Returns: next integral or 0 if search hit the bottom
 **/
gint
_db_find_multi (sqlite3_stmt *res)
{
    if (sqlite3_step (res) == SQLITE_ROW) // id for description found
    {
        gint result = sqlite3_column_int (res, 0);
        return result;
    } else {
        sqlite3_finalize (res);
        return 0;
    }
}

/**
 * _db_prepare:
 * @db: sqlite database handle
 * @sql: sql command in string
 * @res: target sql statement
 *
 * Returns: 1 if successful
 **/
gint
_db_prepare (sqlite3 *db, const gchar *sql, sqlite3_stmt **res)
{
    gint rc = sqlite3_prepare_v2 (db, sql, -1, res, NULL);
    if (rc != SQLITE_OK) {
        fprintf (stderr, "SQL error: %s\n", sqlite3_errmsg (db));
        fprintf (stderr, "SQL error: Prepare failed!\n");
        sqlite3_finalize (*res);
        return 0;
    }
    return 1; // true because of assert
}

/**
 * _db_bind:
 * @res: sqlite statement
 * @id: value placeholder
 * @source: new placeholder value
 *
 * Bind string to placeholder
 *
 * Returns: 1 if successful
 **/
gint
_db_bind (sqlite3_stmt *res, const gchar *id, const gchar *source)
{
    gint idx = sqlite3_bind_parameter_index (res, id);
    gint rc = sqlite3_bind_text (res, idx, source, -1, SQLITE_STATIC);

    if (rc) // something went wrong
    {
        fprintf (stderr, "SQL error: Could not bind parameters(%d|%s|%s)\n", idx, id, source);
        sqlite3_finalize (res);
        return 0;
    }
    return 1; // true because of assert
}

/**
 * _db_bind_int:
 * @res: sqlite statement
 * @id: value placeholder
 * @source: new placeholder value
 *
 * Bind integer to placeholder
 *
 * Returns: 1 if successful
 **/
gint
_db_bind_int (sqlite3_stmt *res, const gchar *id, gint source)
{
    gint idx = sqlite3_bind_parameter_index (res, id);
    gint rc = sqlite3_bind_int (res, idx, source);

    if (rc) // something went wrong
    {
        fprintf (stderr, "SQL error: Could not bind parameters(%s|%d)\n", id, source);
        sqlite3_finalize (res);
        return 0;
    }
    return 1; // true because of assert
}

/**
 * _db_exec:
 * @db: sqlite database handle
 * @cmd: sql command
 * @callback: callback function
 *
 * Direct execution of sql command
 *
 * Returns: 0 if successful
 **/
gint
_db_exec (sqlite3 *db, const gchar *cmd, int (*callback) (void *, int, char **, char **))
{
    gchar *err_msg;
    gint result = sqlite3_exec (db, cmd, callback, 0, &err_msg);
    if (result != SQLITE_OK) {
        fprintf (stderr, "SQL error: %s\n", err_msg);
        sqlite3_free (err_msg);
        return 1;
    }
    return 0;
}

/**
 * _tids_from_pdid:
 * @db: sqlite database handle
 * @pdid: package data ID
 *
 * Get transaction data IDs for selected package data ID
 *
 * Returns: list of transaction data IDs
 **/
static GArray *
_tids_from_pdid (sqlite3 *db, gint pdid)
{
    sqlite3_stmt *res;
    GArray *tids = g_array_new (0, 0, sizeof (gint));
    const gchar *sql = FIND_TIDS_FROM_PDID;
    DB_PREP (db, sql, res);
    DB_BIND_INT (res, "@pdid", pdid);
    gint tid = 0;
    while (sqlite3_step (res) == SQLITE_ROW) {
        tid = sqlite3_column_int (res, 0);
        g_array_append_val (tids, tid);
    }
    sqlite3_finalize (res);
    return tids;
}

/**
 * _all_pdid_for_pid:
 * @db: sqlite database handle
 * @pid: package ID
 *
 * Get all package data IDs connected with package with ID @pid.
 *
 * Returns: list of package data IDs
 **/
static GArray *
_all_pdid_for_pid (sqlite3 *db, gint pid)
{
    GArray *pdids = g_array_new (0, 0, sizeof (gint));
    sqlite3_stmt *res;
    const gchar *sql = FIND_ALL_PDID_FOR_PID;
    DB_PREP (db, sql, res);
    DB_BIND_INT (res, "@pid", pid);
    gint pdid;
    while ((pdid = DB_FIND_MULTI (res))) {
        g_array_append_val (pdids, pdid);
    }
    return pdids;
}

/**
 * _simple_search:
 * @db: sqlite database handle
 * @pattern: package name
 *
 * Find transactions which operated with package matched by name
 *
 * Returns: list of transaction IDs
 **/
GArray *
_simple_search (sqlite3 *db, const gchar *pattern)
{
    GArray *tids = g_array_new (0, 0, sizeof (gint));
    sqlite3_stmt *res_simple;
    const gchar *sql_simple = SIMPLE_SEARCH;
    DB_PREP (db, sql_simple, res_simple);
    DB_BIND (res_simple, "@pat", pattern);
    gint pid_simple;
    GArray *simple = g_array_new (0, 0, sizeof (gint));
    while ((pid_simple = DB_FIND_MULTI (res_simple))) {
        g_array_append_val (simple, pid_simple);
    }
    gint pdid;
    for (guint i = 0; i < simple->len; i++) {
        pid_simple = g_array_index (simple, gint, i);
        GArray *pdids = _all_pdid_for_pid (db, pid_simple);
        for (guint j = 0; j < pdids->len; j++) {
            pdid = g_array_index (pdids, gint, j);
            GArray *tids_for_pdid = _tids_from_pdid (db, pdid);
            tids = g_array_append_vals (tids, tids_for_pdid->data, tids_for_pdid->len);
            g_array_free (tids_for_pdid, TRUE);
        }
        g_array_free (pdids, TRUE);
    }
    g_array_free (simple, TRUE);
    return tids;
}

/**
 * _extended_search:
 * @db: sqlite database handle
 * @pattern: package pattern
 *
 * Find transactions containing packages matched by @pattern
 * Accepted pattern formats:
 *   name.arch
 *   name-version-release.arch
 *   name-version
 *   epoch:name-version-release.arch
 *   name-epoch-version-release.arch*
 *
 * Returns: list of transaction IDs
 **/
GArray *
_extended_search (sqlite3 *db, const gchar *pattern)
{
    GArray *tids = g_array_new (0, 0, sizeof (gint));
    sqlite3_stmt *res;
    const gchar *sql = SEARCH_PATTERN;
    DB_PREP (db, sql, res);
    // lot of patterns...
    DB_BIND (res, "@pat", pattern);
    DB_BIND (res, "@pat", pattern);
    DB_BIND (res, "@pat", pattern);
    DB_BIND (res, "@pat", pattern);
    DB_BIND (res, "@pat", pattern);
    DB_BIND (res, "@pat", pattern);
    GArray *pids = g_array_new (0, 0, sizeof (gint));
    gint pid;
    while (sqlite3_step (res) == SQLITE_ROW) {
        pid = sqlite3_column_int (res, 0);
        g_array_append_val (pids, pid);
    }
    sqlite3_finalize (res);
    gint pdid;
    for (guint j = 0; j < pids->len; ++j) {
        pid = g_array_index (pids, gint, j);
        GArray *pdids = _all_pdid_for_pid (db, pid);
        for (guint i = 0; i < pdids->len; ++i) {
            pdid = g_array_index (pdids, gint, i);
            GArray *tids_for_pdid = _tids_from_pdid (db, pdid);
            tids = g_array_append_vals (tids, tids_for_pdid->data, tids_for_pdid->len);
            g_array_free (tids_for_pdid, TRUE);
        }
        g_array_free (pdids, TRUE);
    }
    g_array_free (pids, TRUE);
    return tids;
}

/**
 * _load_output:
 * @db: sqlite database handle
 * @tid: transaction ID
 * @type: output type
 *
 * Load output for transaction @tid
 *
 * Returns: list of transaction outputs in string
 **/
GPtrArray *
_load_output (sqlite3 *db, gint tid, gint type)
{
    sqlite3_stmt *res;
    const gchar *sql = S_OUTPUT;
    DB_PREP (db, sql, res);
    DB_BIND_INT (res, "@tid", tid);
    DB_BIND_INT (res, "@type", type);
    GPtrArray *l = g_ptr_array_new ();
    gchar *row;
    while ((row = (gchar *)DB_FIND_STR_MULTI (res))) {
        g_ptr_array_add (l, row);
    }
    return l;
}

/**
 * _output_insert:
 * @db: sqlite database handle
 * @tid: transaction ID
 * @msg: output text
 * @type: type of @msg (stdout or stderr)
 **/
void
_output_insert (sqlite3 *db, gint tid, const gchar *msg, gint type)
{
    sqlite3_stmt *res;
    const gchar *sql = I_OUTPUT;
    DB_PREP (db, sql, res);
    DB_BIND_INT (res, "@tid", tid);
    DB_BIND (res, "@msg", msg);
    DB_BIND_INT (res, "@type", type);
    DB_STEP (res);
}
