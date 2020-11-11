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


#include "libdnf/transaction/sack.hpp"

#include "libdnf/base/base.hpp"
#include "libdnf/transaction/db/comps_environment.hpp"
#include "libdnf/transaction/db/comps_group.hpp"
#include "libdnf/transaction/db/db.hpp"
#include "libdnf/transaction/db/rpm.hpp"


namespace libdnf::transaction {


TransactionSack::TransactionSack(libdnf::Base & base) : base{base} {}


TransactionSack::~TransactionSack() = default;


void TransactionSack::fill() {
    auto conn = transaction_db_connect(base);
    comps_environment_reasons = comps_environment_select_reasons(*conn);
    comps_group_reasons = comps_group_select_reasons(*conn);
    package_reasons = rpm_select_reasons(*conn);
}


TransactionQuery TransactionSack::new_query() {
    // create an *empty* query
    // the content is lazily loaded/cached while running the queries
    TransactionQuery q;
    q.sack = this;
    return q;
}


TransactionWeakPtr TransactionSack::new_transaction() {
    return add_item_with_return(std::make_unique<Transaction>(*this));
}


TransactionItemReason TransactionSack::get_comps_environment_reason(const std::string & environmentid) const {
    TransactionItemReason result{TransactionItemReason::UNKNOWN};

    auto it = comps_environment_reasons.find(environmentid);
    if (it != comps_environment_reasons.end()) {
        result = it->second;
    }

    // TODO(dmach): inspect also transaction(s) in progress?
    return result;
}


TransactionItemReason TransactionSack::get_comps_group_reason(const std::string & groupid) const {
    TransactionItemReason result{TransactionItemReason::UNKNOWN};

    auto it = comps_group_reasons.find(groupid);
    if (it != comps_group_reasons.end()) {
        result = it->second;
    }

    // TODO(dmach): inspect also transaction(s) in progress?
    return result;
}


TransactionItemReason TransactionSack::get_package_reason(const std::string & name, const std::string & arch) const {
    TransactionItemReason result{TransactionItemReason::UNKNOWN};

    auto it = package_reasons.find(std::make_pair(name, arch));
    if (it != package_reasons.end()) {
        result = it->second;
    }

    // TODO(dmach): inspect also transaction(s) in progress?
    return result;
}


}  // namespace libdnf::transaction
