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

RPMItem::RPMItem(SQLite3 & conn)
  : Item(conn)
{
}

RPMItem::RPMItem(SQLite3 & conn, int64_t rpm_item_id)
  : Item(conn)
{
    dbSelect(rpm_item_id);
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
RPMItem::dbSelect(int64_t rpm_item_id)
{
    const char * sql =
        "SELECT "
        "  name, "
        "  epoch, "
        "  version, "
        "  release, "
        "  arch, "
        "  checksum_type, "
        "  checksum_data "
        "FROM "
        "  rpm "
        "WHERE "
        "  item_id = ?";
    SQLite3::Statement query(conn, sql);
    query.bindv(rpm_item_id);
    query.step();

    id = rpm_item_id;
    setName(query.get<std::string>(0));
    setEpoch(query.get<int>(1));
    setVersion(query.get<std::string>(2));
    setRelease(query.get<std::string>(3));
    setArch(query.get<std::string>(4));
    setChecksumType(query.get<std::string>(5));
    setChecksumData(query.get<std::string>(6));
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
        "  (?, ?, ?, ?, ?, ?, ?, ?)";
    SQLite3::Statement query(conn, sql);
    query.bindv(getId(),
                getName(),
                getEpoch(),
                getVersion(),
                getRelease(),
                getArch(),
                getChecksumType(),
                getChecksumData());
    query.step();
}

std::vector<std::shared_ptr<TransactionItem> >
RPMItem::getTransactionItems(SQLite3 & conn, int64_t transaction_id)
{
    std::vector<std::shared_ptr<TransactionItem> > result;

    const char * sql =
        "SELECT "
        // trans_item
        "  ti.id, "
        "  ti.done, "
        // rpm
        "  i.item_id, "
        "  i.name, "
        "  i.epoch, "
        "  i.version, "
        "  i.release, "
        "  i.arch, "
        "  i.checksum_type, "
        "  i.checksum_data "
        "FROM "
        "  trans_item ti, "
        "  rpm i "
        "WHERE "
        "  ti.trans_id = ?"
        "  AND ti.item_id = i.item_id";
    SQLite3::Statement query(conn, sql);
    query.bindv(transaction_id);

    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto trans_item = std::make_shared<TransactionItem>(conn);
        auto item = std::make_shared<RPMItem>(conn);
        trans_item->setItem(item);

        trans_item->setId(query.get<int>(0));
        // TODO: implement query.get<bool>
        trans_item->setDone(query.get<int>(1) ? true : false);
        item->setId(query.get<int>(2));
        item->setName(query.get<std::string>(3));
        item->setEpoch(query.get<int>(4));
        item->setVersion(query.get<std::string>(5));
        item->setRelease(query.get<std::string>(6));
        item->setArch(query.get<std::string>(7));
        item->setChecksumType(query.get<std::string>(8));
        item->setChecksumData(query.get<std::string>(9));

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
        "  AND arch = ? "
        "  AND checksum_type = ? "
        "  AND checksum_data = ? ";

    SQLite3::Statement query(conn, sql);

    query.bindv(getName(),
                getEpoch(),
                getVersion(),
                getRelease(),
                getArch(),
                getChecksumType(),
                getChecksumData());
    SQLite3::Statement::StepResult result = query.step();

    if (result == SQLite3::Statement::StepResult::ROW) {
        setId(query.get<int>(0));
    }
    else {
        // insert and get the ID back
        dbInsert();
    }
}
