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

#ifndef LIBDNF_SWDB_TRANSACTIONITEM_HPP
#define LIBDNF_SWDB_TRANSACTIONITEM_HPP

#include <memory>
#include <string>

#include "../utils/sqlite3/sqlite3.hpp"

class TransactionItem;
typedef std::shared_ptr< TransactionItem > TransactionItemPtr;

#include "item.hpp"
#include "item_comps_environment.hpp"
#include "item_comps_group.hpp"
#include "item_rpm.hpp"
#include "repo.hpp"
#include "swdb_types.hpp"
#include "transaction.hpp"

class TransactionItem {
public:
    TransactionItem(const Transaction &trans);

    int64_t getId() const noexcept { return id; }
    void setId(int64_t value) { id = value; }

    // int64_t getTransactionId() const noexcept { return trans.getId(); }

    std::shared_ptr< Item > getItem() const noexcept { return item; }
    void setItem(std::shared_ptr< Item > value) { item = value; }

    // typed items - workaround for lack of shared_ptr<> downcast support in SWIG
    std::shared_ptr< CompsEnvironmentItem > getCompsEnvironmentItem() const noexcept
    {
        return std::dynamic_pointer_cast< CompsEnvironmentItem >(item);
    }
    std::shared_ptr< CompsGroupItem > getCompsGroupItem() const noexcept
    {
        return std::dynamic_pointer_cast< CompsGroupItem >(item);
    }
    std::shared_ptr< RPMItem > getRPMItem() const noexcept
    {
        return std::dynamic_pointer_cast< RPMItem >(item);
    }

    const std::string &getRepoid() const noexcept { return repoid; }
    void setRepoid(const std::string &value) { repoid = value; }

    const std::vector< std::shared_ptr< TransactionItem > > &getReplacedBy() const noexcept
    {
        return replacedBy;
    }
    void addReplacedBy(std::shared_ptr< TransactionItem > value) { replacedBy.push_back(value); }

    TransactionItemAction getAction() const noexcept { return action; }
    void setAction(TransactionItemAction value) { action = value; }

    const std::string &getActionName();
    const std::string &getActionShort();

    TransactionItemReason getReason() const noexcept { return reason; }
    void setReason(TransactionItemReason value) { reason = value; }

    bool getDone() const noexcept { return done; }
    void setDone(bool value) { done = value; }

    void save();
    void saveReplacedBy();

protected:
    int64_t id = 0;
    const Transaction &trans;
    std::shared_ptr< Item > item;
    // TODO: replace with objects? it's just repoid, probably not necessary
    std::string repoid;
    std::vector< std::shared_ptr< TransactionItem > > replacedBy;
    TransactionItemAction action = TransactionItemAction::INSTALL;
    TransactionItemReason reason = TransactionItemReason::UNKNOWN;
    bool done = false;

    void dbInsert();
    void dbUpdate();
};

#endif // LIBDNF_SWDB_TRANSACTIONITEM_HPP
