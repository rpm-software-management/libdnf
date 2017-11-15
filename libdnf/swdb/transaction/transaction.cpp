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

#include "transaction.hpp"

Transaction::Transaction (unsigned long id,
                          unsigned long uid,
                          const std::string &cliCommand,
                          const std::string &releasever)
  : ReadOnlyTransaction (id, uid, cliCommand, releasever)
{
}

Transaction::Transaction (unsigned long id,
                          unsigned long uid,
                          const std::string &cliCommand,
                          const std::string &releasever,
                          unsigned long timeOfTransactionBegin,
                          unsigned long timeOfTransactionEnd,
                          const std::string &rpmDBVersionBegin,
                          const std::string &rpmDBVersionEnd)
  : ReadOnlyTransaction (id,
                         uid,
                         cliCommand,
                         releasever,
                         timeOfTransactionBegin,
                         timeOfTransactionEnd,
                         rpmDBVersionBegin,
                         rpmDBVersionEnd)
{
}

void
Transaction::addTransactionItem (TransactionItem *transactionItem)
{
}

void Transaction::setSoftwarePerformedWith (std::vector<RPMItem>)
{
}

void
Transaction::begin (const std::string &rpmDBVersion)
{
}
void
Transaction::logOutput (std::string *message, int fileDescriptor)
{
}
void
Transaction::end (std::string &rpmDBVersion)
{
}
