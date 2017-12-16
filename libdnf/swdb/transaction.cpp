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

#include "transaction.hpp"
#include "item_comps_environment.hpp"
#include "item_comps_group.hpp"
#include "item_rpm.hpp"
#include "transactionitem.hpp"

class RPMItem;

Transaction::Transaction(SQLite3 & conn)
  : conn{conn}
{
}

Transaction::Transaction(SQLite3 & conn, int64_t pk)
  : conn{conn}
{
    dbSelect(pk);
}

void
Transaction::save()
{
    if (id == 0) {
        dbInsert();
    }
    else {
        dbUpdate();
    }
}

void
Transaction::dbSelect(int64_t pk)
{
    const char * sql =
        "SELECT "
        "  dt_begin, "
        "  dt_end, "
        "  rpmdb_version_begin, "
        "  rpmdb_version_end, "
        "  releasever, "
        "  user_id, "
        "  cmdline, "
        "  done "
        "FROM "
        "  trans "
        "WHERE "
        "  id = ?";
    SQLite3::Query query(conn, sql);
    query.bindv(pk);
    query.step();

    setId(pk);
    setDtBegin(query.get<int>("dt_begin"));
    setDtEnd(query.get<int>("dt_end"));
    setRpmdbVersionBegin(query.get<std::string>("rpmdb_version_begin"));
    setRpmdbVersionEnd(query.get<std::string>("rpmdb_version_end"));
    setReleasever(query.get<std::string>("releasever"));
    setUserId(query.get<int>("user_id"));
    setCmdline(query.get<std::string>("cmdline"));
    setDone(query.get<bool>("done"));
}

void
Transaction::dbInsert()
{
    const std::string sql =
        "INSERT INTO "
        "  trans ("
        "    dt_begin, "
        "    dt_end, "
        "    rpmdb_version_begin, "
        "    rpmdb_version_end, "
        "    releasever, "
        "    user_id, "
        "    cmdline, "
        "    done, "
        "    id "
        "  ) "
        "VALUES "
        "  (?, ?, ?, ?, ?, ?, ?, ?, ?)";
    SQLite3::Statement query(conn, sql);
    query.bindv(getDtBegin(),
                getDtEnd(),
                getRpmdbVersionBegin(),
                getRpmdbVersionEnd(),
                getReleasever(),
                getUserId(),
                getCmdline(),
                getDone());
    if (getId() > 0) {
            query.bind(9, getId());
    }
    query.step();
    setId(conn.lastInsertRowID());
}

void
Transaction::dbUpdate()
{
    const char * sql =
        "UPDATE "
        "  trans "
        "SET "
        "  dt_begin=?, "
        "  dt_end=?, "
        "  rpmdb_version_begin=?, "
        "  rpmdb_version_end=?, "
        "  releasever=?, "
        "  user_id=?, "
        "  cmdline=?, "
        "  done=? "
        "WHERE "
        "  id = ?";
    SQLite3::Statement query(conn, sql);
    query.bindv(getDtBegin(),
                getDtEnd(),
                getRpmdbVersionBegin(),
                getRpmdbVersionEnd(),
                getReleasever(),
                getUserId(),
                getCmdline(),
                getDone(),
                getId());
    query.step();
}

std::shared_ptr<TransactionItem>
Transaction::addItem(std::shared_ptr<Item> item,
                     const std::string & repoid,
                     TransactionItemAction action,
                     TransactionItemReason reason,
                     std::shared_ptr<TransactionItem> replacedBy)
{
    auto trans_item = std::make_shared<TransactionItem>(*this);
    trans_item->setItem(item);
    trans_item->setRepoid(repoid);
    trans_item->setAction(action);
    trans_item->setReason(reason);
    trans_item->setReplacedBy(replacedBy);
    items.push_back(trans_item);
    return trans_item;
}

void
Transaction::saveItems()
{
    // TODO: remove all existing items from the database first?
    save();
    for (auto i : items) {
        i->save();
    }
}

void
Transaction::loadItems()
{
    auto rpms = RPMItem::getTransactionItems(conn, getId());
    items.insert(items.end(), rpms.begin(), rpms.end());

    auto comps_groups = CompsGroupItem::getTransactionItems(conn, getId());
    items.insert(items.end(), comps_groups.begin(), comps_groups.end());

    auto comps_environments = CompsEnvironmentItem::getTransactionItems(conn, getId());
    items.insert(items.end(), comps_environments.begin(), comps_environments.end());
}
