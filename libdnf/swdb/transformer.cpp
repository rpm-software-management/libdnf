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

#include "transformer.hpp"
#include "item_rpm.hpp"
#include "swdb.hpp"
#include "transaction.hpp"
#include "transactionitem.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

Transformer::Transformer(const std::string & outputFile, const std::string & inputDir)
  : inputDir(inputDir)
  , outputFile(outputFile)
  , transformFile(outputFile + ".transform")
{
}

void
Transformer::transform()
{
    // perform transformation
    // block to limit lifetime of database objects
    {
        SQLite3 swdb(transformFile.c_str());
        SQLite3 history(historyPath().c_str());
        // create a new database file
        SwdbCreateDatabase(swdb);

        // transform objects
        transformRPMItems(swdb, history);
        transformTrans(swdb, history);
        transformTransWith(swdb, history);
        transformOutput(swdb, history);
        transformTransItems(swdb, history);
    }

    // rename temporary file to the final one
    int result = rename(transformFile.c_str(), outputFile.c_str());
    if (result != 0) {
        throw Exception(std::strerror(errno));
    }
}

void
Transformer::transformRPMItems(SQLite3 & swdb, SQLite3 & history)
{
    const char * sql = R"**(
        SELECT pkgtupid,
               name, epoch, version, release, arch, checksum FROM pkgtups;
    )**";

    SQLite3::Statement query(history, sql);

    // for every package in the history database
    while (query.step() == SQLite3::Statement::StepResult::ROW) {

        RPMItem pkg(swdb);

        pkg.setId(query.get<int>(0));
        pkg.setName(query.get<std::string>(1));
        pkg.setEpoch(query.get<int>(2));
        pkg.setVersion(query.get<std::string>(3));
        pkg.setRelease(query.get<std::string>(4));
        pkg.setArch(query.get<std::string>(5));

        // checksum in format checksumType:checksumData
        std::string checksum = query.get<std::string>(6);
        size_t delimiter = checksum.find(':');

        // pkg.setChecksumType(checksum.substr(0, delimiter));
        // pkg.setChecksumData(checksum.substr(delimiter + 1));

        pkg.save();
    }
}

void
Transformer::transformTrans(SQLite3 & swdb, SQLite3 & history)
{
    // history.(trans_beg, trans_end, trans_cmdline)

    const char * sql = R"**(
        SELECT
            tid, trans_beg.timestamp, trans_end.timestamp, trans_beg.rpmdb_version,
            trans_end.rpmdb_version, loginuid, cmdline, return_code
        FROM
            trans_beg
            JOIN trans_end using(tid)
            JOIN trans_cmdline using(tid)
    )**";

    SQLite3::Statement query(history, sql);

    // for each transaction
    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        Transaction trans(swdb);

        trans.setId(query.get<int>(0));
        trans.setDtBegin(query.get<int64_t>(1));
        trans.setDtEnd(query.get<int64_t>(2));
        trans.setRpmdbVersionBegin(query.get<std::string>(3));
        trans.setRpmdbVersionEnd(query.get<std::string>(4));
        trans.setReleasever("unknown"); // TODO
        trans.setUserId(query.get<int>(5));
        trans.setCmdline(query.get<std::string>(6));
        trans.setDone(query.get<int>(7) == 0 ? true : false);

        trans.save();
    }
}

void
Transformer::transformTransWith(SQLite3 & swdb, SQLite3 & history)
{
    // history.trans_with_pkgs
    // TODO
}

void
Transformer::transformOutput(SQLite3 & swdb, SQLite3 & history)
{
    // history.trans_script_stdout
    // history.trans_error
    // TODO
}

void
Transformer::transformTransItems(SQLite3 & swdb, SQLite3 & history)
{
    const char * sql = R"**(
        SELECT
            tid, pkgtupid, done, state
        FROM
            trans_data_pkgs
    )**";

    SQLite3::Statement query(history, sql);

    std::unordered_map<std::string, TransactionItemAction> actions = {
        {"Install", TransactionItemAction::INSTALL},
        {"Downgrade", TransactionItemAction::DOWNGRADE},
        {"Downgraded", TransactionItemAction::DOWNGRADED},
        {"Obsolete", TransactionItemAction::OBSOLETE},
        {"Obsoleted", TransactionItemAction::OBSOLETED},
        {"Upgrade", TransactionItemAction::UPGRADED},
        {"Remove", TransactionItemAction::REMOVE},
        {"Reinstall", TransactionItemAction::REINSTALL}};

    std::unordered_map<std::string, TransactionItemReason> reasons = {
        {"dep", TransactionItemReason::DEPENDENCY},
        {"user", TransactionItemReason::USER},
        {"clean", TransactionItemReason::CLEAN},
        {"weak", TransactionItemReason::WEAK_DEPENDENCY},
        {"group", TransactionItemReason::GROUP}};

    // for each transaction
    while (query.step() == SQLite3::Statement::StepResult::ROW) {

        Transaction trans(swdb, query.get<int64_t>(0));

        // TODO we will definitely need a (swdb) constructor
        TransactionItem transItem(trans);

        // TODO set item by ID
        RPMItem rpm(swdb, query.get<int64_t>(1));
        transItem.setItem(std::make_shared<Item>(static_cast<Item>(rpm)));

        transItem.setRepoid("unknown"); // TODO
        // item.setReplacedBy() // TODO

        transItem.setDone(query.get<std::string>(2) == "TRUE");

        transItem.setAction(actions[query.get<std::string>((3))]);

        transItem.setReason(TransactionItemReason::UNKNOWN); // TODO find reason in pkg_yumdb

        transItem.save();
    }
}

std::string
Transformer::historyPath()
{
    std::string historyDir(inputDir);

    // construct the history directory path
    if (historyDir.back() != '/') {
        historyDir += '/';
    }
    historyDir += "history";

    // vector for possible history DB files
    std::vector<std::string> possibleFiles;

    // open history directory
    struct dirent * dp;
    DIR * dirp = opendir(historyDir.c_str());

    // iterate over history directory
    while ((dp = readdir(dirp)) != nullptr) {
        std::string fileName(dp->d_name);

        // push the possible history DB files into vector
        if (fileName.find("history") != std::string::npos) {
            possibleFiles.push_back(fileName);
        }
    }

    if (possibleFiles.empty()) {
        throw Exception("Couldn't find a history database");
    }

    // find the latest DB file
    std::sort(possibleFiles.begin(), possibleFiles.end());

    // return the path
    return historyDir + "/" + possibleFiles.back();
}
