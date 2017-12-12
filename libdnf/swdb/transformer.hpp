/*
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

#ifndef LIBDNF_SWDB_TRANSFORMER_HPP
#define LIBDNF_SWDB_TRANSFORMER_HPP

#include "libdnf/utils/sqlite3/sqlite3.hpp"

class Transformer
{
  public:
    class Exception : public std::runtime_error
    {
      public:
        Exception (const std::string &msg)
          : runtime_error (msg)
        {
        }
        Exception (const char *msg)
          : runtime_error (msg)
        {
        }
    };

    Transformer (const std::string &outputFile, const std::string &inputDir);
    void transform ();

  protected:
    const std::string inputDir;
    const std::string outputFile;
    const std::string transformFile;

    void transformRPMItems (SQLite3 &swdb, SQLite3 &history);
    void transformTransItems (SQLite3 &swdb, SQLite3 &history);
    void transformTrans (SQLite3 &swdb, SQLite3 &history);
    void transformTransWith (SQLite3 &swdb, SQLite3 &history);
    void transformOutput (SQLite3 &swdb, SQLite3 &history);

    std::string historyPath ();
};

#endif
