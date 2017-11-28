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

#include "sqlerror.hpp"
#include <sqlite3.h>
#include <string>

class Statement
{
  public:
    Statement (sqlite3_stmt *sqlRes)
      : res (sqlRes)
    {
    }

    ~Statement ()
    {
        if (res) {
            sqlite3_finalize (res);
        }
    }

    void getColumn (int pos, int &result) { result = sqlite3_column_int (res, pos); }
    void getColumn (int pos, std::string &result)
    {
        result = reinterpret_cast<const char *> (sqlite3_column_text (res, pos));
    }

    bool step () { return sqlite3_step (res) == SQLITE_ROW; }

    void bind (const int pos, const std::string &val)
    {
        int result = sqlite3_bind_text (res, pos, val.c_str (), -1, SQLITE_STATIC);
        if (result != SQLITE_OK) {
            throw SQLError ("Text bind failed", sqlite3_db_handle (res));
        }
    }
    void bind (const int pos, const int val)
    {
        int result = sqlite3_bind_int (res, pos, val);
        if (result != SQLITE_OK) {
            throw SQLError ("Integer bind failed", sqlite3_db_handle (res));
        }
    }

  protected:
    sqlite3_stmt *res;
};

#endif
