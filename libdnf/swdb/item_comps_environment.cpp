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

#include "item_comps_environment.hpp"

CompsEnvironmentItem::CompsEnvironmentItem(std::shared_ptr<SQLite3> conn)
  : Item(conn)
{
}

CompsEnvironmentItem::CompsEnvironmentItem(std::shared_ptr<SQLite3> conn, int64_t pk)
  : Item(conn)
{
    dbSelect(pk);
}

void
CompsEnvironmentItem::save()
{
    if (getId() == 0) {
        dbInsert();
    }
    else {
        // dbUpdate();
    }
    for (auto i : getGroups()) {
        i->save();
    }
}

void
CompsEnvironmentItem::dbSelect(int64_t pk)
{
    const char * sql =
        "SELECT "
        "  environmentid, "
        "  name, "
        "  translated_name, "
        "  groups_type "
        "FROM "
        "  comps_environment "
        "WHERE "
        "  item_id = ?";
    SQLite3::Query query(*conn.get(), sql);
    query.bindv(pk);
    query.step();

    setId(pk);
    setEnvironmentId(query.get<std::string>("groupid"));
    setName(query.get<std::string>("name"));
    setTranslatedName(query.get<std::string>("translated_name"));
    setGroupsType(static_cast<CompsPackageType>(query.get<int>("groups_type")));
}

void
CompsEnvironmentItem::dbInsert()
{
    // populates this->id
    Item::save();

    const char * sql =
        "INSERT INTO "
        "  comps_environment ( "
        "    item_id, "
        "    environmentid, "
        "    name, "
        "    translated_name, "
        "    groups_type "
        "  ) "
        "VALUES "
        "  (?, ?, ?, ?, ?)";
    SQLite3::Statement query(*conn.get(), sql);
    query.bindv(getId(),
                getEnvironmentId(),
                getName(),
                getTranslatedName(),
                static_cast<int>(getGroupsType()));
    query.step();
}

std::vector<std::shared_ptr<TransactionItem> >
CompsEnvironmentItem::getTransactionItems(std::shared_ptr<SQLite3> conn, int64_t transactionId)
{
    std::vector<std::shared_ptr<TransactionItem> > result;

    const char * sql =
        "SELECT "
        // trans_item
        "  ti.id, "
        "  ti.done, "
        // comps_environment
        "  i.item_id, "
        "  i.environmentid, "
        "  i.name, "
        "  i.translated_name, "
        "  i.groups_type "
        "FROM "
        "  trans_item ti, "
        "  comps_environment i "
        "WHERE "
        "  ti.trans_id = ?"
        "  AND ti.item_id = i.item_id";
    SQLite3::Query query(*conn.get(), sql);
    query.bindv(transactionId);

    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto trans_item = std::make_shared<TransactionItem>(conn);
        auto item = std::make_shared<CompsEnvironmentItem>(conn);
        trans_item->setItem(item);

        trans_item->setId(query.get<int>(0));
        trans_item->setDone(query.get<bool>(1));
        item->setId(query.get<int>(2));
        item->setEnvironmentId(query.get<std::string>(3));
        item->setName(query.get<std::string>(4));
        item->setTranslatedName(query.get<std::string>(5));
        item->setGroupsType(static_cast<CompsPackageType>(query.get<int>(6)));

        result.push_back(trans_item);
    }
    return result;
}

std::string
CompsEnvironmentItem::toStr()
{
    return "@" + getEnvironmentId();
}

void
CompsEnvironmentItem::loadGroups()
{
    const char * sql =
        "SELECT "
        "  * "
        "FROM "
        "  comps_evironment_group "
        "WHERE "
        "  environment_id = ?";
    SQLite3::Query query(*conn.get(), sql);
    query.bindv(getId());

    groups.clear();
    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto pkg = std::make_shared<CompsEnvironmentGroup>(*this);
        pkg->setId(query.get<int>("id"));
        pkg->setGroupId(query.get<std::string>("groupid"));
        pkg->setInstalled(query.get<bool>("installed"));
        pkg->setExcluded(query.get<bool>("excluded"));
        pkg->setGroupType(static_cast<CompsPackageType>(query.get<int>("group_type")));
        groups.push_back(pkg);
    }
}

std::shared_ptr<CompsEnvironmentGroup>
CompsEnvironmentItem::addGroup(std::string groupId,
                               bool installed,
                               bool excluded,
                               CompsPackageType groupType)
{
    auto pkg = std::make_shared<CompsEnvironmentGroup>(*this);
    pkg->setGroupId(groupId);
    pkg->setInstalled(installed);
    pkg->setExcluded(excluded);
    pkg->setGroupType(groupType);
    groups.push_back(pkg);
    return pkg;
}

CompsEnvironmentGroup::CompsEnvironmentGroup(CompsEnvironmentItem & environment)
  : environment{environment}
{
}

void
CompsEnvironmentGroup::save()
{
    if (getId() == 0) {
        dbSelectOrInsert();
    }
    else {
        // dbUpdate();
    }
}

void
CompsEnvironmentGroup::dbInsert()
{
    const char * sql =
        "INSERT INTO "
        "  comps_evironment_group ( "
        "    environment_id, "
        "    groupid, "
        "    installed, "
        "    excluded, "
        "    group_type "
        "  ) "
        "VALUES "
        "  (?, ?, ?, ?, ?)";
    SQLite3::Statement query(*getEnvironment().conn.get(), sql);
    query.bindv(getEnvironment().getId(),
                getGroupId(),
                getInstalled(),
                getExcluded(),
                static_cast<int>(getGroupType()));
    query.step();
}

void
CompsEnvironmentGroup::dbSelectOrInsert()
{
    const char * sql =
        "SELECT "
        "  id "
        "FROM "
        "  comps_evironment_group "
        "WHERE "
        "  groupid=?";
    SQLite3::Query query(*getEnvironment().conn.get(), sql);
    query.bindv(getGroupId());
    SQLite3::Statement::StepResult result = query.step();

    if (result == SQLite3::Statement::StepResult::ROW) {
        setId(query.get<int>(0));
    }
    else {
        // insert and get the ID back
        dbInsert();
    }
}
