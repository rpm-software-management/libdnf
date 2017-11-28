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
Handle::getPath ()
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
