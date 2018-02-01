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

#ifndef LIBDNF_SWDB_MERGEDTRANSACTION_HPP
#define LIBDNF_SWDB_MERGEDTRANSACTION_HPP

#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class MergedTransaction;
class MergedTransactionItem;
typedef std::shared_ptr< MergedTransaction > MergedTransactionPtr;
typedef std::shared_ptr< MergedTransactionItem > MergedTransactionItemPtr;

#include "item_rpm.hpp"
#include "transaction.hpp"

class MergedTransactionItem {
public:
    MergedTransactionItem(TransactionItemPtr item)
      : item{item->getItem()}
      , repoid{item->getRepoid()}
      , action{item->getAction()}
    {
    }

    ItemPtr getItem() const noexcept { return item; }
    void setItem(ItemPtr value) { item = value; }

    const std::string &getRepoid() const noexcept { return repoid; }
    void setRepoid(const std::string &value) { repoid = value; }

    TransactionItemAction getAction() const noexcept { return action; }
    void setAction(TransactionItemAction value) { action = value; }

protected:
    ItemPtr item;
    std::string repoid;
    TransactionItemAction action = TransactionItemAction::INSTALL;
};

class MergedTransaction {
public:
    MergedTransaction(TransactionPtr trans);
    void merge(TransactionPtr trans);

    std::vector< int64_t > listIds() const noexcept;
    std::vector< int64_t > listUserIds() const noexcept;
    std::vector< std::string > listCmdlines() const noexcept;
    std::vector< bool > listDone() const noexcept;
    int64_t getDtBegin() const noexcept;
    int64_t getDtEnd() const noexcept;
    const std::string &getRpmdbVersionBegin() const noexcept;
    const std::string &getRpmdbVersionEnd() const noexcept;
    std::set< RPMItemPtr > getSoftwarePerformedWith() const;
    std::vector< std::pair< int, std::string > > getConsoleOutput();

    std::vector< MergedTransactionItemPtr > getItems();

protected:
    std::vector< TransactionPtr > transactions;

    struct ItemPair {
        ItemPair(MergedTransactionItemPtr first, MergedTransactionItemPtr second)
          : first{first}
          , second{second}
        {
        }
        ItemPair(){};
        MergedTransactionItemPtr first = nullptr;
        MergedTransactionItemPtr second = nullptr;
    };
    typedef std::unordered_map< std::string, ItemPair > ItemPairMap;

    void mergeItem(ItemPairMap &itemPairMap, MergedTransactionItemPtr transItem);
    void resolveRPMDifference(ItemPair &previousItemPair, MergedTransactionItemPtr mTransItem);
    void resolveErase(ItemPair &previousItemPair, MergedTransactionItemPtr mTransItem);
    void resolveAltered(ItemPair &previousItemPair, MergedTransactionItemPtr mTransItem);
};

#endif // LIBDNF_SWDB_TRANSACTION_HPP
