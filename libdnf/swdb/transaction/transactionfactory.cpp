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

#include "transactionfactory.hpp"
#include "transaction.hpp"

ReadOnlyTransaction *
TransactionFactory::getTransaction (unsigned long id, bool readOnly)
{
    // TODO get data from db
    unsigned long uid;
    std::string cliCommand;
    std::string releasever;
    // ...

    if (readOnly) {
        return new ReadOnlyTransaction (id, uid, cliCommand, releasever /*, ... */);
    }

    return new Transaction (id, uid, cliCommand, releasever /*, ... */);
}

ReadOnlyTransaction *
TransactionFactory::createTransaction (unsigned long uid,
                                       const std::string &cliCommand,
                                       const std::string &releasever,
                                       unsigned long id,
                                       bool readOnly)
{
    if (id == 0)
        id = getNextTransactionID ();

    if (readOnly) {
        return new ReadOnlyTransaction (id, uid, cliCommand, releasever);
    }

    return new Transaction (id, uid, cliCommand, releasever);
}

unsigned long
TransactionFactory::getNextTransactionID ()
{
    // TODO return id of next new transaction
    return -1;
}
