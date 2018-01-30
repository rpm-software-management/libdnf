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

#include "mergedtransaction.hpp"

MergedTransaction::MergedTransaction(std::shared_ptr< Transaction > trans)
  : transactions{trans}
{
}

void
MergedTransaction::merge(std::shared_ptr< Transaction > trans)
{
    bool inserted = false;
    for (auto it = transactions.begin(); it < transactions.end(); ++it) {
        if ((*it)->getId() > trans->getId()) {
            transactions.insert(it, trans);
            inserted = true;
            break;
        }
    }
    if (!inserted) {
        transactions.push_back(trans);
    }
}

std::vector< int64_t >
MergedTransaction::listIds() const noexcept
{
    std::vector< int64_t > ids;
    for (auto t : transactions) {
        ids.push_back(t->getId());
    }
    return ids;
}
std::vector< int64_t >
MergedTransaction::listUserIds() const noexcept
{
    std::vector< int64_t > users;
    for (auto t : transactions) {
        users.push_back(t->getUserId());
    }
    return users;
}

std::vector< std::string >
MergedTransaction::listCmdlines() const noexcept
{
    std::vector< std::string > cmdLines;
    for (auto t : transactions) {
        cmdLines.push_back(t->getCmdline());
    }
    return cmdLines;
}

std::vector< bool >
MergedTransaction::listDone() const noexcept
{
    std::vector< bool > done;
    for (auto t : transactions) {
        done.push_back(t->getDone());
    }
    return done;
}

int64_t
MergedTransaction::getDtBegin() const noexcept
{
    return transactions.front()->getDtBegin();
}
int64_t
MergedTransaction::getDtEnd() const noexcept
{
    return transactions.back()->getDtEnd();
}
const std::string &
MergedTransaction::getRpmdbVersionBegin() const noexcept
{
    return transactions.front()->getRpmdbVersionBegin();
}

const std::string &
MergedTransaction::getRpmdbVersionEnd() const noexcept
{
    return transactions.back()->getRpmdbVersionEnd();
}

std::set< std::shared_ptr< RPMItem > >
MergedTransaction::getSoftwarePerformedWith() const
{
    std::set< std::shared_ptr< RPMItem > > software;
    for (auto t : transactions) {
        auto tranSoft = t->getSoftwarePerformedWith();
        software.insert(tranSoft.begin(), tranSoft.end());
    }
    return software;
}

std::vector< std::pair< int, std::string > >
MergedTransaction::getConsoleOutput()
{
    std::vector< std::pair< int, std::string > > output;
    for (auto t : transactions) {
        auto tranOutput = t->getConsoleOutput();
        output.insert(output.end(), tranOutput.begin(), tranOutput.end());
    }
    return output;
}
