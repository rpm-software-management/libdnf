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

#include <memory>
#include <vector>
#include "libdnf/utils/sqlite3/sqlite3.hpp"
#include "item_rpm.hpp"
#include "transaction.hpp"
#include "transactionitem.hpp"

// following classe override default behavior with
// inserting rows with explicitly set IDs

class TransformerTransaction : public Transaction {
public:
    using Transaction::Transaction;
    void save() { dbInsert(); }
};

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
    std::vector<std::shared_ptr<TransformerTransaction> > transformTrans(std::shared_ptr<SQLite3> swdb, std::shared_ptr<SQLite3> history);
    void transformRPMItems(std::shared_ptr<SQLite3> swdb, std::shared_ptr<SQLite3> history, std::shared_ptr<TransformerTransaction> trans);
    void transformTransWith(std::shared_ptr<SQLite3> swdb, std::shared_ptr<SQLite3> history);
    void transformOutput(std::shared_ptr<SQLite3> swdb, std::shared_ptr<SQLite3> history);

  private:
    std::string historyPath ();
    const std::string inputDir;
    const std::string outputFile;
    const std::string transformFile;
};

#endif
