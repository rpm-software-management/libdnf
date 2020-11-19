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

#ifndef LIBDNF_TRANSACTION_MODULESTREAMITEM_HPP
#define LIBDNF_TRANSACTION_MODULESTREAMITEM_HPP

#include <memory>
#include <vector>

#include "Item.hpp"
#include "TransactionItem.hpp"


namespace libdnf {


class TransactionItem;
using TransactionItemPtr = std::shared_ptr< TransactionItem >;

class ModuleStreamItem : public Item {
public:
    explicit ModuleStreamItem(SQLite3Ptr conn);
    ModuleStreamItem(SQLite3Ptr conn, int64_t pk);
    virtual ~ModuleStreamItem() = default;

    const std::string &getName() const noexcept { return name; }
    void setName(const std::string &value) { name = value; }

    const std::string &getStream() const noexcept { return stream; }
    void setStream(const std::string &value) { stream = value; }

    virtual std::string toStr() const override;
    virtual ItemType getItemType() const noexcept { return itemType; }

    virtual void save();
    static std::vector< TransactionItemPtr > getTransactionItems(SQLite3Ptr conn,
                                                                 int64_t transactionId);

protected:
    const ItemType itemType = ItemType::MODULE_STREAM;
    std::string name;
    std::string stream;

private:
    void dbSelect(int64_t pk);
    void dbInsert();
    void dbSelectOrInsert();
};


typedef std::shared_ptr<ModuleStreamItem> ModuleStreamItemPtr;


} // namespace libdnf

#endif // LIBDNF_TRANSACTION_MODULESTREAMITEM_HPP
