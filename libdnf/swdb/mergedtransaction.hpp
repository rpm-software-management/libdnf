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

#ifndef LIBDNF_SWDB_MERGEDTRANSACTION_HPP
#define LIBDNF_SWDB_MERGEDTRANSACTION_HPP

#include <set>
#include <string>
#include <vector>

class MergedTransaction;

#include "item_rpm.hpp"
#include "transaction.hpp"

class MergedTransaction {
public:
    MergedTransaction(std::shared_ptr< Transaction > trans);
    void merge(std::shared_ptr< Transaction > trans);

    std::vector< int64_t > listIds() const noexcept;
    std::vector< int64_t > listUserIds() const noexcept;
    std::vector< std::string > listCmdlines() const noexcept;
    std::vector< bool > listDone() const noexcept;
    int64_t getDtBegin() const noexcept;
    int64_t getDtEnd() const noexcept;
    const std::string &getRpmdbVersionBegin() const noexcept;
    const std::string &getRpmdbVersionEnd() const noexcept;
    std::set< std::shared_ptr< RPMItem > > getSoftwarePerformedWith() const;
    std::vector< std::pair< int, std::string > > getConsoleOutput();

    // TODO std::vector< std::shared_ptr< TransactionItem > > getItems();

protected:
    std::vector< std::shared_ptr< Transaction > > transactions;
};

#endif // LIBDNF_SWDB_TRANSACTION_HPP
