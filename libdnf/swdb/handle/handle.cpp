/* handle.cpp
 *
 * Copyright (C) 2017 Red Hat, Inc.
 * Author: Eduard Cuba <ecuba@redhat.com>
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

#include "handle.hpp"
#include "handle-sql.hpp"
#include <cstdio>
#include <unistd.h>

// Handle *Handle::handle = nullptr;

Handle::~Handle ()
{
    if (res != nullptr) {
        sqlite3_finalize (res);
    }
    close ();
}

/*
Handle *
Handle::getInstance (const char *dbPath)
{
    if (handle == nullptr) {
        handle = new Handle (dbPath);
    }

    return handle;
}
*/

Handle::Handle (const char *dbPath)
  : path (dbPath)
{
    db = nullptr;
    res = nullptr;
}

void
Handle::createDB ()
{
    open ();
    exec (sql_create_tables);
}

void
Handle::resetDB ()
{
    close ();
    remove (path);
    createDB ();
}

bool
Handle::exists ()
{
    return access (path, F_OK) != -1;
}

void
Handle::open ()
{
    if (db == nullptr) {
        if (sqlite3_open (path, &db) != SQLITE_OK) {
            throw SQLError ("Open failed", db);
        }
    }
}

void
Handle::close ()
{
    if (db == nullptr) {
        return;
    }
    if (sqlite3_close (db) == SQLITE_BUSY) {
        sqlite3_stmt *res;
        while ((res = sqlite3_next_stmt (db, nullptr))) {
            sqlite3_finalize (res);
        }
        if (sqlite3_close (db)) {
            throw SQLError ("Close failed", db);
        }
    }
    db = nullptr;
}

const char *
Handle::getPath () const
{
    return path;
}

void
Handle::exec (const char *sql)
{
    open ();
    int result = sqlite3_exec (db, sql, nullptr, 0, nullptr);
    if (result != SQLITE_OK) {
        throw SQLError ("Exec failed", db);
    }
}

int
Handle::getInt (int pos)
{
    if (res == nullptr) {
        return 0;
    }
    return sqlite3_column_int (res, pos);
}

std::string
Handle::getString (int pos)
{
    if (res == nullptr) {
        return "";
    }
    auto result = reinterpret_cast<const char *> (sqlite3_column_text (res, pos));
    return result ? result : "";
}

bool
Handle::step ()
{
    if (res == nullptr) {
        return false;
    }
    int result = sqlite3_step (res);
    if (result == SQLITE_OK || result == SQLITE_ROW || result == SQLITE_DONE) {
        return result == SQLITE_ROW;
    }
    throw SQLError ("Step failed", db);
}

void
Handle::bind (const int pos, const std::string &val)
{
    if (res == nullptr) {
        return;
    }
    int result = sqlite3_bind_text (res, pos, val.c_str (), -1, SQLITE_TRANSIENT);
    if (result != SQLITE_OK) {
        throw SQLError ("Text bind failed", sqlite3_db_handle (res));
    }
}

void
Handle::bind (const int pos, const int val)
{
    if (res == nullptr) {
        return;
    }
    int result = sqlite3_bind_int (res, pos, val);
    if (result != SQLITE_OK) {
        throw SQLError ("Integer bind failed", sqlite3_db_handle (res));
    }
}

int64_t
Handle::lastInsertRowID ()
{
    open ();
    return sqlite3_last_insert_rowid (db);
}
