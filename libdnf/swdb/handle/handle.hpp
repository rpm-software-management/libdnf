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

#include "statement.hpp"
#include <sqlite3.h>
#include <string>

class Handle
{
  public:
    ~Handle ();

    static Handle *getInstance (const char *dbPath);

    void createDB ();
    void resetDB ();

    bool exists ();

    /**
     * template function should be used in following way:
     *
     * Statement<Int, String> s = prepare<Int, String>("SELECT id, name FROM foo WHERE id=?", 1);
     *
     * const Row<Int, String> &row = s.next();
     * if (!row.empty()) {
     *     int id = row.E1;
     *     string name = row.E2;
     * }
     *
     **/
    template<typename... Types, class... Ts>
    Statement<Types...> prepare (const char *sql, Ts... args)
    {
        open ();
        sqlite3_stmt *res;
        if (sqlite3_prepare_v2 (db, sql, -1, &res, nullptr) != SQLITE_OK) {
            // TODO handle error
        }

        Statement<Types...> statement (res);

        // placeholders are indexed from 1
        int pos = 1;

        // call bind for each parameter in parameter pack (left to right)
        [](...) {}((statement.bind (pos++, std::forward<Ts> (args)), 0)...);

        return statement;
    }

    const char *getPath ();

  protected:
    Handle (const char *dbPath);

    void open ();
    void close ();

    const char *path;

    static Handle *handle;
    sqlite3 *db;
};

#endif
