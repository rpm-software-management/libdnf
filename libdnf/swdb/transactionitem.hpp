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

#ifndef LIBDNF_SWDB_TRANSACTIONITEM_HPP
#define LIBDNF_SWDB_TRANSACTIONITEM_HPP

#include <memory>
#include <string>

#include "item.hpp"
#include "libdnf/utils/sqlite3/sqlite3.hpp"
#include "repo.hpp"
#include "transaction.hpp"

class Transaction;

enum class TransactionItemReason : int
{
    UNKNOWN = 0,
    DEPENDENCY = 1,
    USER = 2,
    CLEAN = 3, // hawkey compatibility
    WEAK_DEPENDENCY = 4,
    GROUP = 5
};

enum class TransactionItemAction : int
{
    INSTALL = 1,    // a new package that was installed on the system
    DOWNGRADE = 2,  // an older package version that replaced previously installed version
    DOWNGRADED = 3, // an original package version that was replaced
    OBSOLETE = 4,   //
    OBSOLETED = 5,  //
    UPGRADE = 6,    //
    UPGRADED = 7,   //
    REMOVE = 8,     // a package that was removed from the system
    REINSTALL = 9   // a package that was reinstalled with the identical version
};

/*
Install
-------
* Command example: dnf install bash
* Old package(s): (none)
* New package(s): bash-4.4.12-5
* -> new TransactionItem: item="bash-4.4.12-5", action=INSTALL, reason=<new>, replaced_by=NULL

Downgrade
---------
* Command example: dnf downgrade bash
* Old package(s): bash-4.4.12-5
* New package(s): bash-4.4.12-4
* -> new TransactionItem: item="bash-4.4.12-5", action=DOWNGRADE, reason=<inherited>,
replaced_by=NULL
* -> new TransactionItem: item="bash-4.4.12-4", action=DOWNGRADED, reason=<inherited>,
replaced_by=<id of bash-4.4.12-5 transaction item>

Obsolete
--------
* Command example: dnf upgrade
* Old package(s): sysvinit-2.88-9
* New package(s): systemd-233-6
* -> new TransactionItem: item="systemd-233-6", action=OBSOLETE, reason=<inherited>,
replaced_by=NULL
* -> new TransactionItem: item="sysvinit-2.88-9", action=OBSOLETED, reason=<inherited>,
replaced_by=<id of systemd-233-6 transaction item>

Obsolete & Upgrade
------------------
* Command example: dnf upgrade
* Old package(s): systemd-233-5, sysvinit-2.88-9
* New package(s): systemd-233-6 (introducing Obsoletes: sysvinit)
* -> new TransactionItem: item="systemd-233-6", action=UPGRADE, reason=<inherited>, replaced_by=NULL
* -> new TransactionItem: item="systemd-233-5", action=UPGRADED, reason=<inherited>, replaced_by=<id
of systemd-233-6 transaction item>
* -> new TransactionItem: item="sysvinit-2.88-9", action=OBSOLETED, reason=<inherited>,
replaced_by=<id of systemd-233-6 transaction item>

Upgrade
-------
* Command example: dnf upgrade
* Old package(s): bash-4.4.12-4
* New package(s): bash-4.4.12-5
* -> new TransactionItem: item="bash-4.4.12-5", action=UPGRADE, reason=<inherited>, replaced_by=NULL
* -> new TransactionItem: item="bash-4.4.12-4", action=UPGRADED, reason=<inherited>, replaced_by=<id
of bash-4.4.12-4 transaction item>

Remove
------
* Command example: dnf remove bash
* Old package(s): bash-4.4.12-5
* New package(s): (none)
* -> new TransactionItem: item="bash-4.4.12-5", action=REMOVED, reason=<new>, replaced_by=NULL

Reinstall
---------
* Command example: dnf reinstall bash
* Old package(s): bash-4.4.12-5
* New package(s): bash-4.4.12-5
* -> new TransactionItem: item="bash-4.4.12-5", action=REINSTALL, reason=<inherited>,
replaced_by=NULL

Reasons:
* new = a brand new reason why a package was installed or removed
* inherited = a package was installed in the past, re-use it's reason in existing transaction
*/

class TransactionItem {
public:
    TransactionItem(const Transaction & trans);

    int64_t getId() const noexcept { return id; }
    void setId(int64_t value) { id = value; }

    // TODO: make read-only, set in constructor?
    // int64_t getTransactionId() const noexcept { return trans.getId(); }
    // void setTransactionId(int64_t value) { transactionId = value; }

    std::shared_ptr<Item> getItem() const noexcept { return item; }
    void setItem(std::shared_ptr<Item> value) { item = value; }

    const std::string & getRepoid() const noexcept { return repoid; }
    void setRepoid(const std::string & value) { repoid = value; }

    std::shared_ptr<TransactionItem> getReplacedBy() const noexcept { return replacedBy; }
    void setReplacedBy(std::shared_ptr<TransactionItem> value) { replacedBy = value; }

    TransactionItemAction getAction() const noexcept { return action; }
    void setAction(TransactionItemAction value) { action = value; }

    TransactionItemReason getReason() const noexcept { return reason; }
    void setReason(TransactionItemReason value) { reason = value; }

    bool getDone() const noexcept { return done; }
    void setDone(bool value) { done = value; }

    void save();

protected:
    int64_t id = 0;
    const Transaction & trans;
    std::shared_ptr<Item> item;
    // TODO: replace with objects? it's just repoid, probably not necessary
    std::string repoid;
    std::shared_ptr<TransactionItem> replacedBy = NULL;
    TransactionItemAction action = TransactionItemAction::INSTALL;
    TransactionItemReason reason = TransactionItemReason::UNKNOWN;
    bool done = false;

private:
    void dbInsert();
    void dbUpdate();
};

#endif // LIBDNF_SWDB_TRANSACTIONITEM_HPP
