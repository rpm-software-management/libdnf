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

#include "transactionitem.hpp"

TransactionItem::TransactionItem(const Transaction & trans)
  : trans{trans}
{
}

void
TransactionItem::save()
{
    getItem()->save();
    if (getId() == 0) {
        dbInsert();
    }
    else {
        dbUpdate();
    }
}

void
TransactionItem::dbInsert()
{
    const char * sql =
        "INSERT INTO "
        "  trans_item ( "
        "    id, "
        "    trans_id, "
        "    item_id, "
        "    repo_id, "
        "    replaced_by, "
        "    action, "
        "    reason, "
        "    done "
        "  ) "
        "VALUES "
        "  (null, ?, ?, ?, ?, ?, ?, ?)";
    SQLite3::Statement query(*trans.conn.get(), sql);
    query.bindv(trans.getId(),
                getItem()->getId(),
                Repo::getCached(trans.conn, getRepoid())->getId(),
                NULL, // replaced_by
                static_cast<int>(getAction()),
                static_cast<int>(getReason()),
                getDone());
    query.step();
    setId(trans.conn->lastInsertRowID());
}

void
TransactionItem::dbUpdate()
{
    const char * sql =
        "UPDATE "
        "  trans_item "
        "SET "
        "  trans_id=?, "
        "  item_id=?, "
        "  repo_id=?, "
        "  replaced_by=?, "
        "  action=?, "
        "  reason=?, "
        "  done=? "
        "WHERE "
        "  id = ?";
    SQLite3::Statement query(*trans.conn.get(), sql);
    query.bindv(trans.getId(),
                getItem()->getId(),
                Repo::getCached(trans.conn, getRepoid())->getId(),
                NULL,
                static_cast<int>(getAction()),
                static_cast<int>(getReason()),
                getDone(),
                getId());
    query.step();
}
