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
#include <vector>

#include "../hy-query.h"
#include "../utils/sqlite3/sqlite3.hpp"

#include "item_comps_group.hpp"
#include "transaction.hpp"
#include "transactionitem.hpp"

class Swdb {
public:
    explicit Swdb(SQLite3Ptr conn);
    explicit Swdb(const std::string &path);

    const std::string &getPath() { return conn->getPath(); }

    void resetDatabase();
    void closeDatabase();

    void initTransaction();
    int64_t beginTransaction(int64_t dtBegin,
                             std::string rpmdbVersionBegin,
                             std::string cmdline,
                             uint32_t userId);
    int64_t endTransaction(int64_t dtEnd, std::string rpmdbVersionEnd, bool done);

    std::shared_ptr< const Transaction > getLastTransaction();
    std::vector< TransactionPtr > listTransactions(); // std::vector<long long> transactionIds);

    TransactionItemPtr addItem(std::shared_ptr< Item > item,
                               const std::string &repoid,
                               TransactionItemAction action,
                               TransactionItemReason reason);
    // std::shared_ptr<TransactionItem> replacedBy);
    void setItemDone(TransactionItemPtr item);
    void addConsoleOutputLine(int fileDescriptor, std::string line);

    // Item: RPM
    int resolveRPMTransactionItemReason(const std::string &name,
                                        const std::string &arch,
                                        int64_t maxTransactionId);
    const std::string getRPMRepo(const std::string &nevra);
    std::shared_ptr< const TransactionItem > getRPMTransactionItem(const std::string &nevra);

    RPMItem createRPMItem();
    CompsGroupItem createCompsGroupItem();
    CompsEnvironmentItem createCompsEnvironmentItem();

    // Item: CompsGroup
    TransactionItemPtr getCompsGroupItem(const std::string &groupid);
    std::vector< TransactionItemPtr > getCompsGroupItemsByPattern(const std::string &pattern);
    std::vector< std::string > getPackageCompsGroups(const std::string &packageName);

    // Item: CompsEnvironment
    TransactionItemPtr getCompsEnvironmentItem(const std::string &envid);
    std::vector< TransactionItemPtr > getCompsEnvironmentItemsByPattern(const std::string &pattern);
    std::vector< std::string > getCompsGroupEnvironments(const std::string &groupId);

protected:
    SQLite3Ptr conn;
    std::unique_ptr< Transaction > transactionInProgress = nullptr;

private:
};

#endif // LIBDNF_SWDB_SWDB_HPP
