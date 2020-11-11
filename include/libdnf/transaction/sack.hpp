/*
Copyright (C) 2020 Red Hat, Inc.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/


#ifndef LIBDNF_TRANSACTION_SACK_HPP
#define LIBDNF_TRANSACTION_SACK_HPP


#include "query.hpp"
#include "transaction.hpp"

#include "libdnf/common/sack/sack.hpp"
#include "libdnf/transaction/db/db.hpp"

#include <mutex>


namespace libdnf {
class Base;
}


namespace libdnf::transaction {


/// Weak pointer to Transaction. It doesn't own the object (ptr_owner = false).
/// Transactions are owned by TransactionSack.
using TransactionWeakPtr = libdnf::WeakPtr<Transaction, false>;


using TransactionSackWeakPtr = libdnf::WeakPtr<TransactionSack, false>;


/// TransactionSack holds Transaction objects.
/// Unlike in other sacks, the data is loaded on-demand.
class TransactionSack : public libdnf::sack::Sack<Transaction, TransactionQuery> {
public:
    explicit TransactionSack(libdnf::Base & base);
    ~TransactionSack();

    TransactionQuery new_query();

    /// Create a new Transaction object, store it in the sack and return a weak pointer to it.
    /// The Transaction object is owned by the TransactionSack.
    TransactionWeakPtr new_transaction();

    /// Fill TransactionSack with data.
    /// Because Transactions are loaded lazily, load (cache) only reasons.
    void fill();

    using libdnf::sack::Sack<Transaction, TransactionQuery>::get_data;

    /// Return a resolved comps environment reason from the transaction database based on 'environmentid'
    TransactionItemReason get_comps_environment_reason(const std::string & environmentid) const;

    /// Return a resolved comps group reason from the transaction database based on 'groupid'
    TransactionItemReason get_comps_group_reason(const std::string & groupid) const;

    /// Return a resolved package reason from the transaction database based on package 'name' and 'arch'
    ///
    /// @replaces libdnf:transaction/RPMItem.hpp:method:RPMItem.resolveTransactionItemReason(SQLite3Ptr conn, const std::string & name, const std::string & arch, int64_t maxTransactionId)
    TransactionItemReason get_package_reason(const std::string & name, const std::string & arch) const;

private:
    friend class Package;
    friend class Transaction;
    friend class TransactionQuery;
    libdnf::Base & base;

    // cached comps environment reasons: environmentd -> reason
    std::map<std::string, TransactionItemReason> comps_environment_reasons;

    // cached comps group reasons: groupid -> reason
    std::map<std::string, TransactionItemReason> comps_group_reasons;

    // cached package reasons: (name, arch) -> reason
    std::map<std::pair<std::string, std::string>, TransactionItemReason> package_reasons;

    // Lazy adding new items to the sack needs to be thread-safe to avoid adding duplicates.
    std::mutex mtx;
};


}  // namespace libdnf::transaction


#endif  // LIBDNF_TRANSACTION_SACK_HPP
