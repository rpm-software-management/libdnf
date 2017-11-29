/* handle.hpp
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

#ifndef __HANDLE_HPP
#define __HANDLE_HPP

#include "sqlerror.hpp"
#include <sqlite3.h>
#include <string>

class Handle
{
  public:
    Handle (const char *dbPath); // XXX this should be protected if we want handle as singleton
    ~Handle ();

    // static Handle *getInstance (const char *dbPath);

    void createDB ();
    void resetDB ();

    bool exists ();

    /**
     * template function should be used in following way:
     *
     * prepare("SELECT id, name FROM foo WHERE id=?", 1);
     * while(step()) {
     *     int id = getInt(0);
     *     std::string id = getString(1);
     * }
     *
     **/
    template<class... Ts>
    void prepare (const char *sql, Ts... args)
    {
        open ();

        if (res != nullptr) {
            sqlite3_finalize (res);
            res = nullptr;
        }

        if (sqlite3_prepare_v2 (db, sql, -1, &res, nullptr) != SQLITE_OK) {
            throw SQLError ("Prepare failed", db);
        }

        // placeholders are indexed from 1
        int pos = sizeof...(args);

        // call bind for each parameter in parameter pack (right to left)
        [](...) {}((bind (pos--, std::forward<Ts> (args)), 0)...);
    }

    const char *getPath () const;

    bool step ();
    void exec (const char *sql);

    int getInt (int pos);
    std::string getString (int pos);

    void bind (const int pos, const std::string &val);
    void bind (const int pos, const int val);

    int64_t lastInsertRowID ();

  protected:
    void open ();
    void close ();

    const char *path;

    static Handle *handle;
    sqlite3 *db;
    sqlite3_stmt *res;
};

#endif
