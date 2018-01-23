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

#include <algorithm>

#include "item_comps_group.hpp"
#include "transactionitem.hpp"

CompsGroupItem::CompsGroupItem(std::shared_ptr< SQLite3 > conn)
  : Item{conn}
{
}

CompsGroupItem::CompsGroupItem(std::shared_ptr< SQLite3 > conn, int64_t pk)
  : Item{conn}
{
    dbSelect(pk);
}

void
CompsGroupItem::save()
{
    if (getId() == 0) {
        dbInsert();
    } else {
        // dbUpdate();
    }
    for (auto i : getPackages()) {
        i->save();
    }
}

void
CompsGroupItem::dbSelect(int64_t pk)
{
    const char *sql =
        "SELECT "
        "  groupid, "
        "  name, "
        "  translated_name, "
        "  pkg_types "
        "FROM "
        "  comps_group "
        "WHERE "
        "  item_id = ?";
    SQLite3::Query query(*conn.get(), sql);
    query.bindv(pk);
    query.step();

    setId(pk);
    setGroupId(query.get< std::string >("groupid"));
    setName(query.get< std::string >("name"));
    setTranslatedName(query.get< std::string >("translated_name"));
    setPackageTypes(static_cast< CompsPackageType >(query.get< int >("pkg_types")));
}

void
CompsGroupItem::dbInsert()
{
    // populates this->id
    Item::save();

    const char *sql =
        "INSERT INTO "
        "  comps_group ( "
        "    item_id, "
        "    groupid, "
        "    name, "
        "    translated_name, "
        "    pkg_types "
        "  ) "
        "VALUES "
        "  (?, ?, ?, ?, ?)";
    SQLite3::Statement query(*conn.get(), sql);
    query.bindv(getId(),
                getGroupId(),
                getName(),
                getTranslatedName(),
                static_cast< int >(getPackageTypes()));
    query.step();
}

static std::shared_ptr< TransactionItem >
compsGroupTransactionItemFromQuery(std::shared_ptr< SQLite3 > conn, SQLite3::Query &query)
{
    auto trans_item = std::make_shared< TransactionItem >(conn);
    auto item = std::make_shared< CompsGroupItem >(conn);
    trans_item->setItem(item);

    trans_item->setId(query.get< int >("ti_id"));
    trans_item->setAction(static_cast< TransactionItemAction >(query.get< int >("ti_action")));
    trans_item->setReason(static_cast< TransactionItemReason >(query.get< int >("ti_reason")));
    trans_item->setDone(query.get< bool >("ti_done"));
    item->setId(query.get< int >("item_id"));
    item->setGroupId(query.get< std::string >("groupid"));
    item->setName(query.get< std::string >("name"));
    item->setTranslatedName(query.get< std::string >("translated_name"));
    item->setPackageTypes(static_cast< CompsPackageType >(query.get< int >("pkg_types")));

    return trans_item;
}

std::shared_ptr< TransactionItem >
CompsGroupItem::getTransactionItem(std::shared_ptr< SQLite3 > conn, const std::string &groupid)
{
    const char *sql = R"**(
        SELECT
            ti.id as ti_id,
            ti.done as ti_done,
            ti.action as ti_action,
            ti.reason as ti_reason,
            i.item_id,
            i.groupid,
            i.name,
            i.translated_name,
            i.pkg_types
        FROM
            trans_item ti
        JOIN
            comps_group i USING (item_id)
        JOIN
            trans t ON ti.trans_id = t.id
        WHERE
            t.done = 1
            /* see comment in transactionitem.hpp - TransactionItemAction */
            AND ti.action not in (3, 5, 7)
            AND i.groupid = ?
        ORDER BY
            ti.trans_id DESC
    )**";

    SQLite3::Query query(*conn, sql);
    query.bindv(groupid);
    if (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto trans_item = compsGroupTransactionItemFromQuery(conn, query);
        if (trans_item->getAction() == TransactionItemAction::REMOVE) {
            return nullptr;
        }
        return trans_item;
    }
    return nullptr;
}

std::vector< std::shared_ptr< TransactionItem > >
CompsGroupItem::getTransactionItemsByPattern(std::shared_ptr< SQLite3 > conn,
                                             const std::string &pattern)
{
    const char *sql = R"**(
        SELECT DISTINCT
            groupid
        FROM
            comps_group
        WHERE
            groupid LIKE ?
            OR name LIKE ?
            OR translated_name LIKE ?
    )**";

    std::vector< std::shared_ptr< TransactionItem > > result;

    SQLite3::Query query(*conn, sql);
    std::string pattern_sql = pattern;
    std::replace(pattern_sql.begin(), pattern_sql.end(), '*', '%');
    query.bindv(pattern, pattern, pattern);
    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto groupid = query.get< std::string >("groupid");
        auto trans_item = getTransactionItem(conn, groupid);
        if (!trans_item) {
            continue;
        }
        result.push_back(trans_item);
    }
    return result;
}

std::vector< std::shared_ptr< TransactionItem > >
CompsGroupItem::getTransactionItems(std::shared_ptr< SQLite3 > conn, int64_t transactionId)
{
    std::vector< std::shared_ptr< TransactionItem > > result;

    const char *sql = R"**(
        SELECT
            ti.id as ti_id,
            ti.action as ti_action,
            ti.reason as ti_reason,
            ti.done as ti_done,
            i.item_id,
            i.groupid,
            i.name,
            i.translated_name,
            i.pkg_types
        FROM
            trans_item ti
        JOIN
            comps_group i USING (item_id)
        WHERE
            ti.trans_id = ?
    )**";
    SQLite3::Query query(*conn.get(), sql);
    query.bindv(transactionId);

    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto trans_item = compsGroupTransactionItemFromQuery(conn, query);
        result.push_back(trans_item);
    }
    return result;
}

std::string
CompsGroupItem::toStr()
{
    return "@" + getGroupId();
}

void
CompsGroupItem::loadPackages()
{
    const char *sql =
        "SELECT "
        "  * "
        "FROM "
        "  comps_group_package "
        "WHERE "
        "  group_id = ?";
    SQLite3::Query query(*conn.get(), sql);
    query.bindv(getId());

    packages.clear();
    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto pkg = std::make_shared< CompsGroupPackage >(*this);
        pkg->setId(query.get< int >("id"));
        pkg->setName(query.get< std::string >("name"));
        pkg->setInstalled(query.get< bool >("installed"));
        pkg->setPackageType(static_cast< CompsPackageType >(query.get< int >("pkg_type")));
        packages.push_back(pkg);
    }
}

std::shared_ptr< CompsGroupPackage >
CompsGroupItem::addPackage(std::string name, bool installed, CompsPackageType pkgType)
{
    auto pkg = std::make_shared< CompsGroupPackage >(*this);
    pkg->setName(name);
    pkg->setInstalled(installed);
    pkg->setPackageType(pkgType);
    packages.push_back(pkg);
    return pkg;
}

CompsGroupPackage::CompsGroupPackage(CompsGroupItem &group)
  : group{group}
{
}

void
CompsGroupPackage::save()
{
    if (getId() == 0) {
        dbSelectOrInsert();
    } else {
        // dbUpdate();
    }
}

void
CompsGroupPackage::dbInsert()
{
    const char *sql = R"**(
        INSERT INTO
            comps_group_package (
                group_id,
                name,
                installed,
                pkg_type
            )
        VALUES
            (?, ?, ?, ?)
    )**";
    SQLite3::Statement query(*getGroup().conn.get(), sql);
    query.bindv(
        getGroup().getId(), getName(), getInstalled(), static_cast< int >(getPackageType()));
    query.step();
}

void
CompsGroupPackage::dbSelectOrInsert()
{
    const char *sql = R"**(
        SELECT
            id
        FROM
          comps_group_package
        WHERE
            name = ?
            AND group_id = ?
    )**";

    SQLite3::Statement query(*getGroup().conn.get(), sql);
    query.bindv(getName(), getGroup().getId());
    SQLite3::Statement::StepResult result = query.step();

    if (result == SQLite3::Statement::StepResult::ROW) {
        setId(query.get< int >(0));
    } else {
        // insert and get the ID back
        dbInsert();
    }
}
