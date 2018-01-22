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

#ifndef LIBDNF_SWDB_ITEM_COMPS_GROUP_HPP
#define LIBDNF_SWDB_ITEM_COMPS_GROUP_HPP

#include <memory>
#include <vector>

#include "item.hpp"
#include "transactionitem.hpp"

class TransactionItem;

enum class CompsPackageType : int {
    MANDATORY = 0,
    DEFAULT = 1,
    OPTIONAL = 2,
    CONDITIONAL = 3,
};

class Item;
class CompsGroupItem;
class CompsGroupPackage;
class TransactionItem;

class CompsGroupItem : public Item {
public:
    CompsGroupItem(std::shared_ptr< SQLite3 > conn);
    CompsGroupItem(std::shared_ptr< SQLite3 > conn, int64_t pk);
    virtual ~CompsGroupItem() = default;

    const std::string &getGroupId() const noexcept { return groupId; }
    void setGroupId(const std::string &value) { groupId = value; }

    const std::string &getName() const noexcept { return name; }
    void setName(const std::string &value) { name = value; }

    const std::string &getTranslatedName() const noexcept { return translatedName; }
    void setTranslatedName(const std::string &value) { translatedName = value; }

    CompsPackageType getPackageTypes() const noexcept { return packageTypes; }
    void setPackageTypes(CompsPackageType value) { packageTypes = value; }

    virtual std::string toStr();
    virtual const ItemType getItemType() const noexcept { return itemType; }
    virtual void save();
    std::shared_ptr< CompsGroupPackage > addPackage(std::string name,
                                                    bool installed,
                                                    CompsPackageType pkgType);
    void loadPackages();
    std::vector< std::shared_ptr< CompsGroupPackage > > getPackages() { return packages; }
    static std::shared_ptr< TransactionItem > getTransactionItem(std::shared_ptr< SQLite3 > conn,
                                                                 const std::string &groupid);
    static std::vector< std::shared_ptr< TransactionItem > > getTransactionItemsByPattern(
        std::shared_ptr< SQLite3 > conn,
        const std::string &pattern);
    static std::vector< std::shared_ptr< TransactionItem > > getTransactionItems(
        std::shared_ptr< SQLite3 > conn,
        int64_t transactionId);

protected:
    const ItemType itemType = ItemType::GROUP;
    std::string groupId;
    std::string name;
    std::string translatedName;
    CompsPackageType packageTypes;

    std::vector< std::shared_ptr< CompsGroupPackage > > packages;

private:
    friend class CompsGroupPackage;
    void dbSelect(int64_t pk);
    void dbInsert();
};

class CompsGroupPackage {
public:
    CompsGroupPackage(CompsGroupItem &group);

    int64_t getId() const noexcept { return id; }
    void setId(int64_t value) { id = value; }

    const CompsGroupItem &getGroup() const noexcept { return group; }

    const std::string &getName() const noexcept { return name; }
    void setName(const std::string &value) { name = value; }

    bool getInstalled() const noexcept { return installed; }
    void setInstalled(bool value) { installed = value; }

    CompsPackageType getPackageType() const noexcept { return packageType; }
    void setPackageType(CompsPackageType value) { packageType = value; }

    // virtual std::string toStr();
    void save();

protected:
    int64_t id = 0;
    CompsGroupItem &group;
    std::string name;
    bool installed = false;
    CompsPackageType packageType;

private:
    void dbSelect(int64_t pk);
    void dbInsert();
    void dbSelectOrInsert();
};

#endif // LIBDNF_SWDB_ITEM_COMPS_GROUP_HPP
