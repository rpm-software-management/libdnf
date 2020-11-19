/*
 * Copyright (C) 2019 Red Hat, Inc.
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

#include <algorithm>

#include "ModuleStreamItem.hpp"
#include "TransactionItem.hpp"

namespace libdnf {

ModuleStreamItem::ModuleStreamItem(SQLite3Ptr conn)
  : Item{conn}
{
}

ModuleStreamItem::ModuleStreamItem(SQLite3Ptr conn, int64_t pk)
  : Item{conn}
{
    dbSelect(pk);
}

void
ModuleStreamItem::save()
{
    if (getId() == 0) {
        //dbInsert();
        dbSelectOrInsert();
    } else {
        // dbUpdate();
    }
}

void
ModuleStreamItem::dbSelect(int64_t pk)
{
    const char *sql =
        "SELECT "
        "  name, "
        "  stream "
        "FROM "
        "  module_stream "
        "WHERE "
        "  item_id = ?";
    SQLite3::Query query(*conn.get(), sql);
    query.bindv(pk);
    query.step();

    setId(pk);
    setName(query.get< std::string >("name"));
    setStream(query.get< std::string >("stream"));
}

void
ModuleStreamItem::dbInsert()
{
    // populates this->id
    Item::save();

    const char *sql =
        "INSERT INTO "
        "  module_stream ( "
        "    item_id, "
        "    name, "
        "    stream "
        "  ) "
        "VALUES "
        "  (?, ?, ?)";
    SQLite3::Statement query(*conn.get(), sql);
    query.bindv(getId(),
                getName(),
                getStream());
    query.step();
}

void
ModuleStreamItem::dbSelectOrInsert()
{
    const char *sql =
        "SELECT "
        "  item_id "
        "FROM "
        "  module_stream "
        "WHERE "
        "  name = ? "
        "  AND stream = ? ";

    SQLite3::Statement query(*conn.get(), sql);

    query.bindv(getName(), getStream());
    SQLite3::Statement::StepResult result = query.step();

    if (result == SQLite3::Statement::StepResult::ROW) {
        setId(query.get< int >(0));
    } else {
        // insert and get the ID back
        dbInsert();
    }
}

std::string
ModuleStreamItem::toStr() const
{
    if (getStream().empty()) {
        return getName();
    }
    return getName() + ":" + getStream();
}


static TransactionItemPtr
moduleStreamTransactionItemFromQuery(SQLite3Ptr conn, SQLite3::Query &query, int64_t transID)
{

    auto trans_item = std::make_shared< TransactionItem >(conn, transID);
    auto item = std::make_shared< ModuleStreamItem >(conn);
    trans_item->setItem(item);

    trans_item->setId(query.get< int >("ti_id"));
    trans_item->setAction(static_cast< TransactionItemAction >(query.get< int >("ti_action")));
    trans_item->setReason(static_cast< TransactionItemReason >(query.get< int >("ti_reason")));
    trans_item->setState(static_cast< TransactionItemState >(query.get< int >("ti_state")));
    item->setId(query.get< int >("item_id"));
    item->setName(query.get< std::string >("name"));
    item->setStream(query.get< std::string >("stream"));

    return trans_item;
}


std::vector< TransactionItemPtr >
ModuleStreamItem::getTransactionItems(SQLite3Ptr conn, int64_t transactionId)
{
    std::vector< TransactionItemPtr > result;

    const char *sql = R"**(
        SELECT
            ti.id as ti_id,
            ti.action as ti_action,
            ti.reason as ti_reason,
            ti.state as ti_state,
            i.item_id,
            i.name,
            i.stream
        FROM
            trans_item ti
        JOIN
            module_stream i USING (item_id)
        WHERE
            ti.trans_id = ?
    )**";
    SQLite3::Query query(*conn.get(), sql);
    query.bindv(transactionId);

    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto trans_item = moduleStreamTransactionItemFromQuery(conn, query, transactionId);
        result.push_back(trans_item);
    }
    return result;
}

} // namespace libdnf
