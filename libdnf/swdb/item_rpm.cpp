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

#include "item_rpm.hpp"

RPMItem::RPMItem(std::shared_ptr<SQLite3> conn)
  : Item(conn)
{
}

RPMItem::RPMItem(std::shared_ptr<SQLite3> conn, int64_t pk)
  : Item(conn)
{
    dbSelect(pk);
}

void
RPMItem::save()
{
    if (getId() == 0) {
        dbSelectOrInsert();
    }
    else {
        // TODO: dbUpdate() ?
    }
}

void
RPMItem::dbSelect(int64_t pk)
{
    const char * sql =
        "SELECT "
        "  name, "
        "  epoch, "
        "  version, "
        "  release, "
        "  arch "
        "FROM "
        "  rpm "
        "WHERE "
        "  item_id = ?";
    SQLite3::Statement query(*conn.get(), sql);
    query.bindv(pk);
    query.step();

    setId(pk);
    setName(query.get<std::string>(0));
    setEpoch(query.get<int>(1));
    setVersion(query.get<std::string>(2));
    setRelease(query.get<std::string>(3));
    setArch(query.get<std::string>(4));
}

void
RPMItem::dbInsert()
{
    // populates this->id
    Item::save();

    const char * sql =
        "INSERT INTO "
        "  rpm "
        "VALUES "
        "  (?, ?, ?, ?, ?, ?)";
    SQLite3::Statement query(*conn.get(), sql);
    query.bindv(getId(),
                getName(),
                getEpoch(),
                getVersion(),
                getRelease(),
                getArch());
    query.step();
}

std::vector<std::shared_ptr<TransactionItem> >
RPMItem::getTransactionItems(std::shared_ptr<SQLite3> conn, int64_t transaction_id)
{
    std::vector<std::shared_ptr<TransactionItem> > result;

    const char * sql =
        "SELECT "
        // trans_item
        "  ti.id, "
        "  ti.action, "
        "  ti.reason, "
        "  ti.done, "
        // repo
        "  r.repoid, "
        // rpm
        "  i.item_id, "
        "  i.name, "
        "  i.epoch, "
        "  i.version, "
        "  i.release, "
        "  i.arch "
        "FROM "
        "  trans_item ti, "
        "  repo r, "
        "  rpm i "
        "WHERE "
        "  ti.trans_id = ? "
        "  AND ti.repo_id = r.id "
        "  AND ti.item_id = i.item_id";
    SQLite3::Query query(*conn.get(), sql);
    query.bindv(transaction_id);

    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto trans_item = std::make_shared<TransactionItem>(conn);
        auto item = std::make_shared<RPMItem>(conn);
        trans_item->setItem(item);

        trans_item->setId(query.get<int>("id"));
        trans_item->setAction(static_cast<TransactionItemAction>(query.get<int>("action")));
        trans_item->setReason(static_cast<TransactionItemReason>(query.get<int>("reason")));
        trans_item->setRepoid(query.get<std::string>("repoid"));
        trans_item->setDone(query.get<bool>("done"));
        item->setId(query.get<int>("item_id"));
        item->setName(query.get<std::string>("name"));
        item->setEpoch(query.get<int>("epoch"));
        item->setVersion(query.get<std::string>("version"));
        item->setRelease(query.get<std::string>("release"));
        item->setArch(query.get<std::string>("arch"));

        result.push_back(trans_item);
    }
    return result;
}

std::string
RPMItem::getNEVRA()
{
    // TODO: use string formatting
    return name + "-" + std::to_string(epoch) + ":" + version + "-" + release + "." + arch;
}

std::string
RPMItem::toStr()
{
    return getNEVRA();
}

void
RPMItem::dbSelectOrInsert()
{
    const char * sql =
        "SELECT "
        "  item_id "
        "FROM "
        "  rpm "
        "WHERE "
        "  name = ? "
        "  AND epoch = ? "
        "  AND version = ? "
        "  AND release = ? "
        "  AND arch = ?";

    SQLite3::Statement query(*conn.get(), sql);

    query.bindv(getName(),
                getEpoch(),
                getVersion(),
                getRelease(),
                getArch());
    SQLite3::Statement::StepResult result = query.step();

    if (result == SQLite3::Statement::StepResult::ROW) {
        setId(query.get<int>(0));
    }
    else {
        // insert and get the ID back
        dbInsert();
    }
}
