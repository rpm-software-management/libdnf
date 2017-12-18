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

#include "item_comps_group.hpp"

CompsGroupItem::CompsGroupItem(std::shared_ptr<SQLite3> conn)
  : Item(conn)
{
}

CompsGroupItem::CompsGroupItem(std::shared_ptr<SQLite3> conn, int64_t pk)
  : Item(conn)
{
    dbSelect(pk);
}

void
CompsGroupItem::save()
{
    if (getId() == 0) {
        dbInsert();
        // dbSelectOrInsert();
    }
    else {
        // dbUpdate();
    }
    for (auto i : getPackages()) {
        i->save();
    }
}

void
CompsGroupItem::dbSelect(int64_t pk)
{
    const char * sql =
        "SELECT "
        "  groupid, "
        "  name, "
        "  translated_name, "
        "  pkgs_type "
        "FROM "
        "  comps_group "
        "WHERE "
        "  item_id = ?";
    SQLite3::Query query(*conn.get(), sql);
    query.bindv(pk);
    query.step();

    setId(pk);
    setGroupId(query.get<std::string>("groupid"));
    setName(query.get<std::string>("name"));
    setTranslatedName(query.get<std::string>("translated_name"));
    setPackagesType(static_cast<CompsPackageType>(query.get<int>("pkgs_type")));
}

void
CompsGroupItem::dbInsert()
{
    // populates this->id
    Item::save();

    const char * sql =
        "INSERT INTO "
        "  comps_group ( "
        "    item_id, "
        "    groupid, "
        "    name, "
        "    translated_name, "
        "    pkgs_type "
        "  ) "
        "VALUES "
        "  (?, ?, ?, ?, ?)";
    SQLite3::Statement query(*conn.get(), sql);
    query.bindv(
        getId(), getGroupId(), getName(), getTranslatedName(), static_cast<int>(getPackagesType()));
    query.step();
}

std::vector<std::shared_ptr<TransactionItem> >
CompsGroupItem::getTransactionItems(std::shared_ptr<SQLite3> conn, int64_t transactionId)
{
    std::vector<std::shared_ptr<TransactionItem> > result;

    const char * sql =
        "SELECT "
        // trans_item
        "  ti.id, "
        "  ti.done, "
        // comps_group
        "  i.item_id, "
        "  i.groupid, "
        "  i.name, "
        "  i.translated_name, "
        "  i.pkgs_type "
        "FROM "
        "  trans_item ti, "
        "  comps_group i "
        "WHERE "
        "  ti.trans_id = ?"
        "  AND ti.item_id = i.item_id";
    SQLite3::Query query(*conn.get(), sql);
    query.bindv(transactionId);

    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto trans_item = std::make_shared<TransactionItem>(conn);
        auto item = std::make_shared<CompsGroupItem>(conn);
        trans_item->setItem(item);

        trans_item->setId(query.get<int>(0));
        trans_item->setDone(query.get<bool>(1));
        item->setId(query.get<int>(2));
        item->setGroupId(query.get<std::string>(3));
        item->setName(query.get<std::string>(4));
        item->setTranslatedName(query.get<std::string>(5));
        item->setPackagesType(static_cast<CompsPackageType>(query.get<int>(6)));

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
    const char * sql =
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
        auto pkg = std::make_shared<CompsGroupPackage>(*this);
        pkg->setId(query.get<int>("id"));
        pkg->setName(query.get<std::string>("name"));
        pkg->setInstalled(query.get<bool>("installed"));
        pkg->setExcluded(query.get<bool>("excluded"));
        pkg->setPackageType(static_cast<CompsPackageType>(query.get<int>("pkg_type")));
        packages.push_back(pkg);
    }
}

std::shared_ptr<CompsGroupPackage>
CompsGroupItem::addPackage(std::string name,
                           bool installed,
                           bool excluded,
                           CompsPackageType pkgType)
{
    auto pkg = std::make_shared<CompsGroupPackage>(*this);
    pkg->setName(name);
    pkg->setInstalled(installed);
    pkg->setExcluded(excluded);
    pkg->setPackageType(pkgType);
    packages.push_back(pkg);
    return pkg;
}

CompsGroupPackage::CompsGroupPackage(CompsGroupItem & group)
  : group{group}
{
}

void
CompsGroupPackage::save()
{
    if (getId() == 0) {
        dbSelectOrInsert();
    }
    else {
        // dbUpdate();
    }
}

void
CompsGroupPackage::dbInsert()
{
    const char * sql =
        "INSERT INTO "
        "  comps_group_package ( "
        "    group_id, "
        "    name, "
        "    installed, "
        "    excluded, "
        "    pkg_type "
        "  ) "
        "VALUES "
        "  (?, ?, ?, ?, ?)";
    SQLite3::Statement query(*getGroup().conn.get(), sql);
    query.bindv(getGroup().getId(),
                getName(),
                getInstalled(),
                getExcluded(),
                static_cast<int>(getPackageType()));
    query.step();
}

void
CompsGroupPackage::dbSelectOrInsert()
{
    const char * sql =
        "SELECT "
        "  id "
        "FROM "
        "  comps_group_package "
        "WHERE "
        "  name = ?";

    SQLite3::Statement query(*getGroup().conn.get(), sql);
    query.bindv(getName());
    SQLite3::Statement::StepResult result = query.step();

    if (result == SQLite3::Statement::StepResult::ROW) {
        setId(query.get<int>(0));
    }
    else {
        // insert and get the ID back
        dbInsert();
    }
}
