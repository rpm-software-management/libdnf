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

#include "item_comps_environment.hpp"

CompsEnvironmentItem::CompsEnvironmentItem(std::shared_ptr< SQLite3 > conn)
  : Item{conn}
{
}

CompsEnvironmentItem::CompsEnvironmentItem(std::shared_ptr< SQLite3 > conn, int64_t pk)
  : Item{conn}
{
    dbSelect(pk);
}

void
CompsEnvironmentItem::save()
{
    if (getId() == 0) {
        dbInsert();
    } else {
        // dbUpdate();
    }
    for (auto i : getGroups()) {
        i->save();
    }
}

void
CompsEnvironmentItem::dbSelect(int64_t pk)
{
    const char *sql = R"**(
        SELECT
            environmentid,
            name,
            translated_name,
            pkg_types
        FROM
            comps_environment
        WHERE
            item_id = ?
    )**";
    SQLite3::Query query(*conn.get(), sql);
    query.bindv(pk);
    query.step();

    setId(pk);
    setEnvironmentId(query.get< std::string >("environmentid"));
    setName(query.get< std::string >("name"));
    setTranslatedName(query.get< std::string >("translated_name"));
    setPackageTypes(static_cast< CompsPackageType >(query.get< int >("pkg_types")));
}

void
CompsEnvironmentItem::dbInsert()
{
    // populates this->id
    Item::save();

    const char *sql = R"**(
        INSERT INTO
            comps_environment (
                item_id,
                environmentid,
                name,
                translated_name,
                pkg_types
            )
        VALUES
            (?, ?, ?, ?, ?)
    )**";
    SQLite3::Statement query(*conn.get(), sql);
    query.bindv(getId(),
                getEnvironmentId(),
                getName(),
                getTranslatedName(),
                static_cast< int >(getPackageTypes()));
    query.step();
}

static std::shared_ptr< TransactionItem >
compsEnvironmentTransactionItemFromQuery(std::shared_ptr< SQLite3 > conn, SQLite3::Query &query)
{
    auto trans_item = std::make_shared< TransactionItem >(conn);
    auto item = std::make_shared< CompsEnvironmentItem >(conn);
    trans_item->setItem(item);

    trans_item->setId(query.get< int >("ti_id"));
    trans_item->setAction(static_cast< TransactionItemAction >(query.get< int >("ti_action")));
    trans_item->setReason(static_cast< TransactionItemReason >(query.get< int >("ti_reason")));
    trans_item->setDone(query.get< bool >("ti_done"));
    item->setId(query.get< int >("item_id"));
    item->setEnvironmentId(query.get< std::string >("environmentid"));
    item->setName(query.get< std::string >("name"));
    item->setTranslatedName(query.get< std::string >("translated_name"));
    item->setPackageTypes(static_cast< CompsPackageType >(query.get< int >("pkg_types")));

    return trans_item;
}

std::shared_ptr< TransactionItem >
CompsEnvironmentItem::getTransactionItem(std::shared_ptr< SQLite3 > conn, const std::string &envid)
{
    const char *sql = R"**(
        SELECT
            ti.id as ti_id,
            ti.done as ti_done,
            ti.action as ti_action,
            ti.reason as ti_reason,
            i.item_id,
            i.environmentid,
            i.name,
            i.translated_name,
            i.pkg_types
        FROM
            trans_item ti
        JOIN
            comps_environment i USING (item_id)
        JOIN
            trans t ON ti.trans_id = t.id
        WHERE
            t.done = 1
            /* see comment in transactionitem.hpp - TransactionItemAction */
            AND ti.action not in (3, 5, 7)
            AND i.environmentid = ?
        ORDER BY
            ti.trans_id DESC
        LIMIT 1
    )**";

    SQLite3::Query query(*conn, sql);
    query.bindv(envid);
    if (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto trans_item = compsEnvironmentTransactionItemFromQuery(conn, query);
        if (trans_item->getAction() == TransactionItemAction::REMOVE) {
            return nullptr;
        }
        return trans_item;
    }
    return nullptr;
}

std::vector< std::shared_ptr< TransactionItem > >
CompsEnvironmentItem::getTransactionItemsByPattern(std::shared_ptr< SQLite3 > conn,
                                                   const std::string &pattern)
{
    const char *sql = R"**(
        SELECT DISTINCT
            environmentid
        FROM
            comps_environment
        WHERE
            environmentid LIKE ?
            OR name LIKE ?
            OR translated_name LIKE ?
    )**";

    std::vector< std::shared_ptr< TransactionItem > > result;

    SQLite3::Query query(*conn, sql);
    std::string pattern_sql = pattern;
    std::replace(pattern_sql.begin(), pattern_sql.end(), '*', '%');
    query.bindv(pattern, pattern, pattern);
    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto groupid = query.get< std::string >("environmentid");
        auto trans_item = getTransactionItem(conn, groupid);
        if (!trans_item) {
            continue;
        }
        result.push_back(trans_item);
    }
    return result;
}

std::vector< std::shared_ptr< TransactionItem > >
CompsEnvironmentItem::getTransactionItems(std::shared_ptr< SQLite3 > conn, int64_t transactionId)
{
    std::vector< std::shared_ptr< TransactionItem > > result;

    const char *sql = R"**(
        SELECT
            ti.id,
            ti.done,
            i.item_id,
            i.environmentid,
            i.name,
            i.translated_name,
            i.pkg_types
        FROM
            trans_item ti,
            comps_environment i
        WHERE
            ti.trans_id = ?
            AND ti.item_id = i.item_id
    )**";
    SQLite3::Query query(*conn.get(), sql);
    query.bindv(transactionId);

    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto trans_item = std::make_shared< TransactionItem >(conn);
        auto item = std::make_shared< CompsEnvironmentItem >(conn);
        trans_item->setItem(item);

        trans_item->setId(query.get< int >(0));
        trans_item->setDone(query.get< bool >(1));
        item->setId(query.get< int >(2));
        item->setEnvironmentId(query.get< std::string >(3));
        item->setName(query.get< std::string >(4));
        item->setTranslatedName(query.get< std::string >(5));
        item->setPackageTypes(static_cast< CompsPackageType >(query.get< int >(6)));

        result.push_back(trans_item);
    }
    return result;
}

std::string
CompsEnvironmentItem::toStr()
{
    return "@" + getEnvironmentId();
}

/**
 * Lazy loader for groups associaded with the environment.
 * \return vector of groups associated with the environment
 */
std::vector< std::shared_ptr< CompsEnvironmentGroup > >
CompsEnvironmentItem::getGroups()
{
    if (groups.empty()) {
        loadGroups();
    }
    return groups;
}

void
CompsEnvironmentItem::loadGroups()
{
    const char *sql = R"**(
        SELECT
            *
        FROM
            comps_environment_group
        WHERE
            environment_id = ?
        ORDER BY
            groupid ASC
    )**";
    SQLite3::Query query(*conn.get(), sql);
    query.bindv(getId());

    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto group = std::make_shared< CompsEnvironmentGroup >(*this);
        group->setId(query.get< int >("id"));
        group->setGroupId(query.get< std::string >("groupid"));

        group->setInstalled(query.get< bool >("installed"));
        group->setGroupType(static_cast< CompsPackageType >(query.get< int >("group_type")));
        groups.push_back(group);
    }
}

std::shared_ptr< CompsEnvironmentGroup >
CompsEnvironmentItem::addGroup(std::string groupId, bool installed, CompsPackageType groupType)
{
    auto pkg = std::make_shared< CompsEnvironmentGroup >(*this);
    pkg->setGroupId(groupId);
    pkg->setInstalled(installed);
    pkg->setGroupType(groupType);
    groups.push_back(pkg);
    return pkg;
}

CompsEnvironmentGroup::CompsEnvironmentGroup(CompsEnvironmentItem &environment)
  : environment{environment}
{
}

void
CompsEnvironmentGroup::save()
{
    if (getId() == 0) {
        dbInsert();
    } else {
        // dbUpdate();
    }
}

void
CompsEnvironmentGroup::dbInsert()
{
    const char *sql = R"**(
        INSERT INTO
            comps_environment_group (
                environment_id,
                groupid,
                installed,
                group_type
            )
        VALUES
            (?, ?, ?, ?)
    )**";
    SQLite3::Statement query(*getEnvironment().conn, sql);
    query.bindv(
        getEnvironment().getId(), getGroupId(), getInstalled(), static_cast< int >(getGroupType()));
    query.step();
    setId(getEnvironment().conn->lastInsertRowID());
}
