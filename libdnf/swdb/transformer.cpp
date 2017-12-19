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
#include <map>

#include <iostream> // XXX debug only


static const std::unordered_map<std::string, TransactionItemAction> actions = {
    {"Install", TransactionItemAction::INSTALL},
    {"True-Install", TransactionItemAction::INSTALL},
    {"Dep-Install", TransactionItemAction::INSTALL},
    {"Downgrade", TransactionItemAction::DOWNGRADE},
    {"Downgraded", TransactionItemAction::DOWNGRADED},
    {"Obsoleting", TransactionItemAction::OBSOLETE},
    {"Obsoleted", TransactionItemAction::OBSOLETED},
    {"Update", TransactionItemAction::UPGRADE},
    {"Updated", TransactionItemAction::UPGRADED},
    {"Erase", TransactionItemAction::REMOVE},
    {"Reinstall", TransactionItemAction::REINSTALL}
};

static const std::unordered_map<std::string, TransactionItemReason> reasons = {
    {"dep", TransactionItemReason::DEPENDENCY},
    {"user", TransactionItemReason::USER},
    {"clean", TransactionItemReason::CLEAN},
    {"weak", TransactionItemReason::WEAK_DEPENDENCY},
    {"group", TransactionItemReason::GROUP}
};

static TransactionItemReason getReason(std::string reason)
{
    auto it = reasons.find(reason);
    if (it == reasons.end()) {
        return TransactionItemReason::UNKNOWN;
    }
    return it->second;
}

Transformer::Transformer(const std::string & outputFile, const std::string & inputDir)
  : inputDir(inputDir)
  , outputFile(outputFile)
{
}

void
Transformer::transform()
{
    auto swdb = std::make_shared<SQLite3>(":memory:");
    auto history = std::make_shared<SQLite3>(historyPath().c_str());

    // create a new database file
    SwdbCreateDatabase(swdb);

    // transform objects
    auto transactions = transformTrans(swdb, history);
    for (auto trans : transactions) {
        transformRPMItems(swdb, history, trans);
    }
    transformTransWith(swdb, history);
    transformOutput(swdb, history);

    // dump database to a file
    swdb->backup(outputFile);
}

std::vector<std::shared_ptr<TransformerTransaction> >
Transformer::transformTrans(std::shared_ptr<SQLite3> swdb, std::shared_ptr<SQLite3> history)
{
    std::vector<std::shared_ptr<TransformerTransaction> > result;

    // we need to left join with trans_cmdline
    // there is no cmdline for certain transactions (e.g. 1)
    const char * trans_sql = R"**(
        SELECT
            tb.tid as id,
            tb.timestamp as dt_begin,
            tb.rpmdb_version rpmdb_version_begin,
            tb.loginuid as user_id,
            te.timestamp as dt_end,
            te.rpmdb_version as rpmdb_version_end,
            te.return_code as done,
            tc.cmdline as cmdline
        FROM
            trans_beg tb
            JOIN trans_end te using(tid)
            LEFT JOIN trans_cmdline tc using(tid)
        ORDER BY
            tb.tid
    )**";

    const char * releasever_sql = R"**(
        SELECT DISTINCT
            trans_data_pkgs.tid as tid,
            yumdb_val as releasever
        FROM
            pkg_yumdb,
            trans_data_pkgs
        WHERE
            pkg_yumdb.pkgtupid = trans_data_pkgs.pkgtupid
            AND yumdb_key='releasever';
    )**";

    std::map<int64_t, std::string> releasever;
    SQLite3::Query releasever_query(*history.get(), releasever_sql);
    while (releasever_query.step() == SQLite3::Statement::StepResult::ROW) {
        releasever[releasever_query.get<int64_t>("tid")] = releasever_query.get<std::string>("releasever");
    }

    SQLite3::Query query(*history.get(), trans_sql);

    // for each transaction
    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto trans = std::make_shared<TransformerTransaction>(swdb);
        trans->setId(query.get<int>("id"));
        trans->setDtBegin(query.get<int64_t>("dt_begin"));
        trans->setDtEnd(query.get<int64_t>("dt_end"));
        trans->setRpmdbVersionBegin(query.get<std::string>("rpmdb_version_begin"));
        trans->setRpmdbVersionEnd(query.get<std::string>("rpmdb_version_end"));
        // TODO: what if releasever is not found
        trans->setReleasever(releasever.at(trans->getId()));
        trans->setUserId(query.get<int>("user_id"));
        trans->setCmdline(query.get<std::string>("cmdline"));
        trans->setDone(query.get<int>("done") == 0 ? true : false);
        trans->save();
        result.push_back(trans);
    }
    return result;
}

void
Transformer::transformTransWith(std::shared_ptr<SQLite3> swdb, std::shared_ptr<SQLite3> history)
{
    // history.trans_with_pkgs
    // TODO support in a Transaction
}

void
Transformer::transformOutput(std::shared_ptr<SQLite3> swdb, std::shared_ptr<SQLite3> history)
{
    // history.trans_script_stdout
    // history.trans_error
    // TODO support in a Transaction
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

void
Transformer::transformRPMItems(std::shared_ptr<SQLite3> swdb, std::shared_ptr<SQLite3> history, std::shared_ptr<TransformerTransaction> trans)
{
    // the order is important here - its Update, Updated
    const char * pkg_sql = R"**(
        SELECT
            t.state,
            t.done,
            r.pkgtupid as id,
            r.name,
            r.epoch,
            r.version,
            r.release,
            r.arch
        FROM
            trans_data_pkgs t
            JOIN pkgtups r using(pkgtupid)
        WHERE
            t.tid=?
    )**";

    const char * yumdb_sql = R"**(
        SELECT
            yumdb_key as key,
            yumdb_val as value
        FROM
            pkg_yumdb
        WHERE
            pkgtupid=?
    )**";

    SQLite3::Query query(*history.get(), pkg_sql);
    query.bindv(trans->getId());

    std::map<int64_t, std::shared_ptr<RPMItem> > seen;
    std::shared_ptr<TransactionItem> last = nullptr;

    // for every package in the history database
    while (query.step() == SQLite3::Statement::StepResult::ROW) {

        // create RPM item object
        auto rpm = std::make_shared<RPMItem>(swdb);
        rpm->setName(query.get<std::string>("name"));
        rpm->setEpoch(query.get<int64_t>("epoch"));
        rpm->setVersion(query.get<std::string>("version"));
        rpm->setRelease(query.get<std::string>("release"));
        rpm->setArch(query.get<std::string>("arch"));
        rpm->save();

        // get item state/action
        std::string stateString = query.get<std::string>("state");
        TransactionItemAction action = actions.at(stateString);

        if (action == TransactionItemAction::OBSOLETE) {
            continue;
        }

        auto it = seen.find(rpm->getId());
        if (it != seen.end()) {
            // without this, import failed on unique constraint
            // due to multiple RPMs with the same NEVRA in one transaction
            // FIXME: we probably need many-to-many relation
            /* Imagine
                tid |name               |version|release    |state
                53  |grub2-tools-efi    |2.02   |18.fc27    |Install
                53  |grub2-tools-efi    |2.02   |18.fc27    |Obsoleting
                53  |grub2-tools        |2.02   |0.40.fc26  |Obsoleted
                53  |grub2-tools-extra  |2.02   |18.fc27    |Install
                53  |grub2-tools-extra  |2.02   |18.fc27    |Obsoleting
                53  |grub2-tools        |2.02   |0.40.fc26  |Obsoleted
                53  |grub2-tools-minimal|2.02   |18.fc27    |Install
                53  |grub2-tools-minimal|2.02   |18.fc27    |Obsoleting
                53  |grub2-tools        |2.02   |0.40.fc26  |Obsoleted
                53  |grub2-tools        |2.02   |18.fc27    |Update
                53  |grub2-tools        |2.02   |0.40.fc26  |Updated
              Expected output:
                ???
            */
            std::cout << rpm->getName() << " " << stateString << std::endl;
            continue;
        }
        seen[rpm->getId()] = rpm;


        // get data from yumdb
        std::map<std::string, std::string> yumdb;
        SQLite3::Query yumdb_query(*history.get(), yumdb_sql);
        yumdb_query.bindv(query.get<int64_t>("id"));
        while (yumdb_query.step() == SQLite3::Statement::StepResult::ROW) {
            yumdb[yumdb_query.get<std::string>("key")] = yumdb_query.get<std::string>("value");
        }

        std::string repoid = yumdb["from_repo"];
        TransactionItemReason reason = getReason(yumdb["reason"]);

        // add TransactionItem object
        auto transItem = trans->addItem(rpm, repoid, action, reason, nullptr);
        transItem->setDone(query.get<std::string>("done") == "TRUE");

        // resolve replaced by
        switch (action) {
            case TransactionItemAction::DOWNGRADED:
            case TransactionItemAction::UPGRADED:
            case TransactionItemAction::OBSOLETED:
                transItem->setReplacedBy(last);
                break;
            default:
                break;
        }

        last = transItem;
    }
    trans->saveItems();
}
