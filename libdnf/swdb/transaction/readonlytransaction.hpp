/*
 * Copyright (C) 2017 Red Hat, Inc.
 * Author: Eduard Cuba <ecuba@redhat.com>
 *         Martin Hatina <mhatina@redhat.com>
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

#ifndef LIBDNF_READONLYTRANSACTION_HPP
#define LIBDNF_READONLYTRANSACTION_HPP

#include <string>
#include <vector>

#include "../hy-package.h"
#include "../item/rpmitem.hpp"
#include "../item/transactionitem.hpp"

class ReadOnlyTransaction
{
  public:
    ReadOnlyTransaction (unsigned long id,
                         unsigned long uid,
                         const std::string &cliCommand,
                         const std::string &releasever);
    ReadOnlyTransaction (unsigned long id,
                         unsigned long uid,
                         const std::string &cliCommand,
                         const std::string &releasever,
                         unsigned long timeOfTransactionBegin,
                         unsigned long timeOfTransactionEnd,
                         const std::string &rpmDBVersionBegin,
                         const std::string &rpmDBVersionEnd);
    virtual ~ReadOnlyTransaction () = default;

    virtual std::vector<TransactionItem *> listTransactionItems () const;
    virtual std::vector<std::string> listLogMessages (int fileDescriptor = -1) const;
    virtual std::vector<RPMItem> getSoftwarePerformedWith () const { return softwarePerformedWith; }
    virtual TransactionItem *getTransactionItem (DnfPackage *package) const;

  protected:
    unsigned long id;
    unsigned long uid;
    std::string cliCommand;
    std::string releasever;

    unsigned long timeOfTransactionBegin;
    unsigned long timeOfTransactionEnd;

    std::string rpmDBVersionBegin;
    std::string rpmDBVersionEnd;

    std::vector<RPMItem> softwarePerformedWith;
};

#endif // LIBDNF_READONLYTRANSACTION_HPP
