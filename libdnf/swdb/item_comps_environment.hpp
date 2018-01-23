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

#ifndef LIBDNF_SWDB_ITEM_COMPS_ENVIRONMENT_HPP
#define LIBDNF_SWDB_ITEM_COMPS_ENVIRONMENT_HPP

#include <memory>
#include <vector>

#include "item.hpp"
#include "item_comps_group.hpp"
#include "transactionitem.hpp"

class CompsEnvironmentGroup;
enum class CompsPackageType;
class TransactionItem;

class CompsEnvironmentItem : public Item {
public:
    CompsEnvironmentItem(std::shared_ptr< SQLite3 > conn);
    CompsEnvironmentItem(std::shared_ptr< SQLite3 > conn, int64_t pk);
    virtual ~CompsEnvironmentItem() = default;

    const std::string &getEnvironmentId() const noexcept { return environmentId; }
    void setEnvironmentId(const std::string &value) { environmentId = value; }

    const std::string &getName() const noexcept { return name; }
    void setName(const std::string &value) { name = value; }

    const std::string &getTranslatedName() const noexcept { return translatedName; }
    void setTranslatedName(const std::string &value) { translatedName = value; }

    CompsPackageType getPackageTypes() const noexcept { return packageTypes; }
    void setPackageTypes(CompsPackageType value) { packageTypes = value; }

    virtual std::string toStr();
    virtual const std::string &getItemType() const noexcept { return itemType; }
    virtual void save();
    std::shared_ptr< CompsEnvironmentGroup > addGroup(std::string groupId,
                                                      bool installed,
                                                      CompsPackageType groupType);
    void loadGroups();
    std::vector< std::shared_ptr< CompsEnvironmentGroup > > getGroups() { return groups; }
    static std::shared_ptr< TransactionItem > getTransactionItem(std::shared_ptr< SQLite3 > conn,
                                                                 const std::string &envid);
    static std::vector< std::shared_ptr< TransactionItem > > getTransactionItemsByPattern(
        std::shared_ptr< SQLite3 > conn,
        const std::string &pattern);
    static std::vector< std::shared_ptr< TransactionItem > > getTransactionItems(
        std::shared_ptr< SQLite3 > conn,
        int64_t transactionId);

protected:
    const std::string itemType = "comps-environment";
    std::string environmentId;
    std::string name;
    std::string translatedName;
    CompsPackageType packageTypes;

    std::vector< std::shared_ptr< CompsEnvironmentGroup > > groups;

private:
    friend class CompsEnvironmentGroup;
    void dbSelect(int64_t pk);
    void dbInsert();
};

class CompsEnvironmentGroup {
public:
    CompsEnvironmentGroup(CompsEnvironmentItem &environment);

    int64_t getId() const noexcept { return id; }
    void setId(int64_t value) { id = value; }

    const CompsEnvironmentItem &getEnvironment() const noexcept { return environment; }

    const std::string &getGroupId() const noexcept { return groupId; }
    void setGroupId(const std::string &value) { groupId = value; }

    bool getInstalled() const noexcept { return installed; }
    void setInstalled(bool value) { installed = value; }

    CompsPackageType getGroupType() const noexcept { return groupType; }
    void setGroupType(CompsPackageType value) { groupType = value; }

    // virtual std::string toStr();
    void save();

protected:
    int64_t id = 0;
    CompsEnvironmentItem &environment;
    std::string groupId;
    bool installed = false;
    CompsPackageType groupType;

private:
    void dbSelect(int64_t pk);
    void dbInsert();
    void dbSelectOrInsert();
};

#endif // LIBDNF_SWDB_ITEM_COMPS_ENVIRONMENT_HPP
