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

#ifndef LIBDNF_SWDB_TRANSACTION_HPP
#define LIBDNF_SWDB_TRANSACTION_HPP

#include <memory>
#include <string>

#include "item.hpp"
#include "libdnf/utils/sqlite3/sqlite3.hpp"
#include "transactionitem.hpp"

class TransactionItem;
enum class TransactionItemAction;
enum class TransactionItemReason;

class Transaction {
public:
    // create an empty object, don't read from db
    Transaction(std::shared_ptr<SQLite3> conn);
    // load from db
    Transaction(std::shared_ptr<SQLite3> conn, int64_t pk);

    int64_t getId() const noexcept { return id; }
    void setId(int64_t value) { id = value; }

    int64_t getDtBegin() const noexcept { return dtBegin; }
    void setDtBegin(int64_t value) { dtBegin = value; }

    int64_t getDtEnd() const noexcept { return dtEnd; }
    void setDtEnd(int64_t value) { dtEnd = value; }

    const std::string & getRpmdbVersionBegin() const noexcept { return rpmdbVersionBegin; }
    void setRpmdbVersionBegin(const std::string & value) { rpmdbVersionBegin = value; }

    const std::string & getRpmdbVersionEnd() const noexcept { return rpmdbVersionEnd; }
    void setRpmdbVersionEnd(const std::string & value) { rpmdbVersionEnd = value; }

    const std::string & getReleasever() const noexcept { return releasever; }
    void setReleasever(const std::string & value) { releasever = value; }

    int32_t getUserId() const noexcept { return userId; }
    void setUserId(int32_t value) { userId = value; }

    const std::string & getCmdline() const noexcept { return cmdline; }
    void setCmdline(const std::string & value) { cmdline = value; }

    bool getDone() const noexcept { return done; }
    void setDone(bool value) { done = value; }

    void save();
    std::shared_ptr<TransactionItem> addItem(std::shared_ptr<Item> item,
                                             const std::string & repoid,
                                             TransactionItemAction action,
                                             TransactionItemReason reason,
                                             std::shared_ptr<TransactionItem> replacedBy);
    std::vector<std::shared_ptr<TransactionItem> > getItems() { return items; }
    void loadItems();
    void saveItems();
    std::shared_ptr<SQLite3> conn;

protected:
    int64_t id = 0;
    int64_t dtBegin = 0;
    int64_t dtEnd = 0;
    std::string rpmdbVersionBegin;
    std::string rpmdbVersionEnd;
    // TODO: move to a new "vars" table?
    std::string releasever;
    int32_t userId = 0;
    std::string cmdline;
    bool done = false;

    std::vector<std::shared_ptr<TransactionItem> > items;
    // std::vector<RPMItem> softwarePerformedWith;

    void dbSelect(int64_t transaction_id);
    void dbInsert();
    void dbUpdate();
};

#endif // LIBDNF_SWDB_TRANSACTION_HPP
