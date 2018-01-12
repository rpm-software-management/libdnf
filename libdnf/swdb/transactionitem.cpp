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

#include <iostream>

#include "transactionitem.hpp"

// TODO: translations
static const std::map< TransactionItemAction, std::string > transactionItemActionName = {
    {TransactionItemAction::INSTALL, "Install"},
    {TransactionItemAction::DOWNGRADE, "Downgrade"},
    {TransactionItemAction::DOWNGRADED, "Downgraded"},
    {TransactionItemAction::OBSOLETE, "Obsolete"},
    {TransactionItemAction::OBSOLETED, "Obsoleted"},
    {TransactionItemAction::UPGRADE, "Upgrade"},
    {TransactionItemAction::UPGRADED, "Upgraded"},
    {TransactionItemAction::REMOVE, "Removed"},
    {TransactionItemAction::REINSTALL, "Reinstall"},
};

static const std::map< TransactionItemAction, std::string > transactionItemActionShort = {
    {TransactionItemAction::INSTALL, "I"},
    {TransactionItemAction::DOWNGRADE, "D"},
    {TransactionItemAction::DOWNGRADED, "D"},
    {TransactionItemAction::OBSOLETE, "O"},
    {TransactionItemAction::OBSOLETED, "O"},
    {TransactionItemAction::UPGRADE, "U"},
    {TransactionItemAction::UPGRADED, "U"},
    // "R" is for Reinstall, therefore use "E" for rEmove (or Erase)
    {TransactionItemAction::REMOVE, "E"},
    {TransactionItemAction::REINSTALL, "R"},
};

/*
static const std::map<std::string, TransactionItemReason> nameTransactionItemReason = {
    {, "I"},
    {TransactionItemAction::DOWNGRADE, "D"},
    {TransactionItemAction::DOWNGRADED, "D"},
    {TransactionItemAction::OBSOLETE, "O"},
    {TransactionItemAction::OBSOLETED, "O"},
    {TransactionItemAction::UPGRADE, "U"},
    {TransactionItemAction::UPGRADED, "U"},
    // "R" is for Reinstall, therefore use "E" for rEmove (or Erase)
    {TransactionItemAction::REMOVE, "E"},
    {TransactionItemAction::REINSTALL, "R"},
};
TransactionItemReason to_TransactionItemReason(const std::string & s) {
    return
*/

const std::string &
TransactionItem::getActionName()
{
    return transactionItemActionName.at(getAction());
}

const std::string &
TransactionItem::getActionShort()
{
    return transactionItemActionShort.at(getAction());
}

TransactionItem::TransactionItem(const Transaction &trans)
  : trans{trans}
{
}

void
TransactionItem::save()
{
    getItem()->save();
    if (getId() == 0) {
        dbInsert();
    } else {
        dbUpdate();
    }
}

void
TransactionItem::dbInsert()
{
    const char *sql =
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
                static_cast< int >(getAction()),
                static_cast< int >(getReason()),
                getDone());
    query.step();
    setId(trans.conn->lastInsertRowID());
}

void
TransactionItem::dbUpdate()
{
    const char *sql =
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
                static_cast< int >(getAction()),
                static_cast< int >(getReason()),
                getDone(),
                getId());
    query.step();
}
