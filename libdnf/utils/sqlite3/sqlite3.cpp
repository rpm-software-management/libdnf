/* sqlite3.cpp
 *
 * Copyright (C) 2017 Red Hat, Inc.
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

#include "sqlite3.hpp"

void
SQLite3::open()
{
    if (db == nullptr) {
        auto result = sqlite3_open(path.c_str(), &db);
        if (result != SQLITE_OK) {
            sqlite3_close(db);
            throw LibException(result, "Open failed");
        }
        // sqlite doesn't behave correctly in chroots without following line:
        exec("PRAGMA journal_mode = TRUNCATE; PRAGMA locking_mode = EXCLUSIVE");
    }
}

void
SQLite3::close()
{
    if (db == nullptr)
        return;
    auto result = sqlite3_close(db);
    if (result == SQLITE_BUSY) {
        sqlite3_stmt *res;
        while ((res = sqlite3_next_stmt(db, nullptr))) {
            sqlite3_finalize(res);
        }
        result = sqlite3_close(db);
    }
    if (result != SQLITE_OK)
        throw LibException(result, "Close failed");
    db = nullptr;
}
