/*
 * Copyright (C) 2017-2018 Red Hat, Inc.
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

#ifndef LIBDNF_SWDB_SWDB_HPP
#define LIBDNF_SWDB_SWDB_HPP

#include <memory>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>
#include <solv/pooltypes.h>

class Swdb;

#include "../hy-types.h"
#include "../sack/query.hpp"
#include "../utils/sqlite3/Sqlite3.hpp"

#include "CompsGroupItem.hpp"
#include "Transaction.hpp"
#include "private/Transaction.hpp"
#include "TransactionItem.hpp"

class Swdb {
public:
    explicit Swdb(SQLite3Ptr conn);
    explicit Swdb(const std::string &path);
    ~Swdb();

    SQLite3Ptr getConn() { return conn; }

    // Database
    static constexpr const char *defaultPath =
        "/var/lib/dnf/history/sw.db"; // FIXME load this from conf
    const std::string &getPath() { return conn->getPath(); }
    void resetDatabase();
    void closeDatabase();

    // Transaction in progress
    void initTransaction();
    int64_t beginTransaction(int64_t dtBegin,
                             std::string rpmdbVersionBegin,
                             std::string cmdline,
                             uint32_t userId);
    int64_t endTransaction(int64_t dtEnd, std::string rpmdbVersionEnd, TransactionState state);
    // TODO:
    std::vector< TransactionItemPtr > getItems() { return transactionInProgress->getItems(); }

    libdnf::TransactionPtr getLastTransaction();
    std::vector< libdnf::TransactionPtr > listTransactions(); // std::vector<long long> transactionIds);

    // TransactionItems
    TransactionItemPtr addItem(ItemPtr item,
                               const std::string &repoid,
                               TransactionItemAction action,
                               TransactionItemReason reason);
    // std::shared_ptr<TransactionItem> replacedBy);

    // TODO: remove; TransactionItem states are saved on transaction save
    void setItemDone(const std::string &nevra);

    // Item: constructors
    RPMItemPtr createRPMItem();
    CompsGroupItemPtr createCompsGroupItem();
    CompsEnvironmentItemPtr createCompsEnvironmentItem();

    // Item: RPM
    TransactionItemReason resolveRPMTransactionItemReason(const std::string &name,
                                                          const std::string &arch,
                                                          int64_t maxTransactionId);
    const std::string getRPMRepo(const std::string &nevra);
    std::shared_ptr< const TransactionItem > getRPMTransactionItem(const std::string &nevra);
    std::vector< int64_t > searchTransactionsByRPM(const std::vector< std::string > &patterns);

    // Item: CompsGroup
    TransactionItemPtr getCompsGroupItem(const std::string &groupid);
    std::vector< TransactionItemPtr > getCompsGroupItemsByPattern(const std::string &pattern);
    std::vector< std::string > getPackageCompsGroups(const std::string &packageName);

    // Item: CompsEnvironment
    TransactionItemPtr getCompsEnvironmentItem(const std::string &envid);
    std::vector< TransactionItemPtr > getCompsEnvironmentItemsByPattern(const std::string &pattern);
    std::vector< std::string > getCompsGroupEnvironments(const std::string &groupId);

    // Console
    void addConsoleOutputLine(int fileDescriptor, std::string line);

    // misc
    std::vector< Id > filterUnneeded(HyQuery installed, Pool *pool) const;

protected:
    SQLite3Ptr conn;
    std::unique_ptr< libdnf::swdb_private::Transaction > transactionInProgress = nullptr;
    std::unordered_map< std::string, TransactionItemPtr > itemsInProgress;

private:
};

#endif // LIBDNF_SWDB_SWDB_HPP
