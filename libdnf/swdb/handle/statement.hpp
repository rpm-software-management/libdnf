/* statement.hpp
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

#ifndef LIBDNF_SWDB_STATEMENT_HPP
#define LIBDNF_SWDB_STATEMENT_HPP

#include <sqlite3.h>
#include <string>

template<typename... Types>
struct Row
{
    // access attributes as E1, E2, E3...
    bool empty ();
};

template<typename... Types>
class Statement
{
  public:
    Statement (sqlite3_stmt *sqlRes)
      : res (sqlRes)
    {
    }

    Row<Types...> next ();
    void exec ()
    {
        // TODO handle error
        sqlite3_step (res);
    }

    void bind (const int pos, const std::string &val)
    {
        // TODO handle error
        sqlite3_bind_text (res, pos, val.c_str (), -1, SQLITE_STATIC);
    }
    void bind (const int pos, const int val)
    {
        // TODO handle error
        sqlite3_bind_int (res, pos, val);
    }

  protected:
    sqlite3_stmt *res;
};

#endif
