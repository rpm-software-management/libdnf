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

#include "readonlytransaction.hpp"

ReadOnlyTransaction::ReadOnlyTransaction (unsigned long id,
                                          unsigned long uid,
                                          const std::string &cliCommand,
                                          const std::string &releasever)
  : id (id)
  , uid (uid)
  , cliCommand (cliCommand)
  , releasever (releasever)
  , timeOfTransactionBegin (-1)
  , timeOfTransactionEnd (-1)
  , rpmDBVersionBegin ()
  , rpmDBVersionEnd ()
{
}

ReadOnlyTransaction::ReadOnlyTransaction (unsigned long id,
                                          unsigned long uid,
                                          const std::string &cliCommand,
                                          const std::string &releasever,
                                          unsigned long timeOfTransactionBegin,
                                          unsigned long timeOfTransactionEnd,
                                          const std::string &rpmDBVersionBegin,
                                          const std::string &rpmDBVersionEnd)
  : id (id)
  , uid (uid)
  , cliCommand (cliCommand)
  , releasever (releasever)
  , timeOfTransactionBegin (timeOfTransactionBegin)
  , timeOfTransactionEnd (timeOfTransactionEnd)
  , rpmDBVersionBegin (rpmDBVersionBegin)
  , rpmDBVersionEnd (rpmDBVersionEnd)
{
}

std::vector<TransactionItem *>
ReadOnlyTransaction::listTransactionItems () const
{
    return std::vector<TransactionItem *> ();
}

std::vector<std::string>
ReadOnlyTransaction::listLogMessages (int fileDescriptor) const
{
    return std::vector<std::string> ();
}

std::vector<RPMItem>
ReadOnlyTransaction::getSoftwarePerformedWith () const
{
    return softwarePerformedWith;
}

TransactionItem *
ReadOnlyTransaction::getTransactionItem (DnfPackage *package) const
{
    return NULL;
}
