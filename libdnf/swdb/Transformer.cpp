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

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "../utils/bgettext/bgettext-lib.h"

#include "RPMItem.hpp"
#include "Swdb.hpp"
#include "Transaction.hpp"
#include "TransactionItem.hpp"
#include "Transformer.hpp"

static const char *sql_create_tables =
#include "sql/create_tables.sql"
    ;

void
Transformer::createDatabase(SQLite3Ptr conn)
{
    conn->exec(sql_create_tables);
}

/**
 * Map of supported actions (originally states): string -> enum
 */
static const std::unordered_map< std::string, TransactionItemAction > actions = {
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
    {"Reinstall", TransactionItemAction::REINSTALL},
    {"Reinstalled", TransactionItemAction::REINSTALL}};

/**
 * Map of supported reasons: string -> enum
 */
static const std::unordered_map< std::string, TransactionItemReason > reasons = {
    {"dep", TransactionItemReason::DEPENDENCY},
    {"user", TransactionItemReason::USER},
    {"clean", TransactionItemReason::CLEAN},
    {"weak", TransactionItemReason::WEAK_DEPENDENCY},
    {"group", TransactionItemReason::GROUP}};

/**
 * Covert string reason into appropriate enumerated variant
 */
TransactionItemReason
Transformer::getReason(const std::string &reason)
{
    auto it = reasons.find(reason);
    if (it == reasons.end()) {
        return TransactionItemReason::UNKNOWN;
    }
    return it->second;
}

/**
 * Default constructor of the Transformer object
 * \param outputFile path to output SQLite3 database
 * \param inputDir directory to load data from (e.g. `/var/lib/dnf/`)
 */
Transformer::Transformer(const std::string &outputFile, const std::string &inputDir)
  : inputDir(inputDir)
  , outputFile(outputFile)
{
}

/**
 * Perform the database transformation routine.
 * The database is transformed in-memory.
 * Final scheme is dumped into outputFile
 */
void
Transformer::transform()
{
    auto swdb = std::make_shared< SQLite3 >(":memory:");

    // create a new database file
    createDatabase(swdb);

    // migrate history db if it exists
    try {
        auto history = std::make_shared< SQLite3 >(historyPath().c_str());

        // transform objects
        transformTrans(swdb, history);

        // transform groups
        transformGroups(swdb);
    }
    catch (Exception ex) {
        // TODO: use a different (more specific) exception
    }

    // dump database to a file
    swdb->backup(outputFile);
}

/**
 * Transform transactions from the history database
 * \param swdb pointer to swdb SQLite3 object
 * \param swdb pointer to history database SQLite3 object
 */
void
Transformer::transformTrans(SQLite3Ptr swdb, SQLite3Ptr history)
{
    std::vector< std::shared_ptr< TransformerTransaction > > result;

    // we need to left join with trans_cmdline
    // there is no cmdline for certain transactions (e.g. 1)
    const char *trans_sql = R"**(
        SELECT
            tb.tid as id,
            tb.timestamp as dt_begin,
            tb.rpmdb_version rpmdb_version_begin,
            tb.loginuid as user_id,
            te.timestamp as dt_end,
            te.rpmdb_version as rpmdb_version_end,
            te.return_code as state,
            tc.cmdline as cmdline
        FROM
            trans_beg tb
            JOIN trans_end te using(tid)
            LEFT JOIN trans_cmdline tc using(tid)
        ORDER BY
            tb.tid
    )**";

    const char *releasever_sql = R"**(
        SELECT DISTINCT
            trans_data_pkgs.tid as tid,
            yumdb_val as releasever
        FROM
            trans_data_pkgs
        JOIN
            pkg_yumdb USING (pkgtupid)
        WHERE
            yumdb_key='releasever'
    )**";

    // get release version for all the transactions
    std::map< int64_t, std::string > releasever;
    SQLite3::Query releasever_query(*history.get(), releasever_sql);
    while (releasever_query.step() == SQLite3::Statement::StepResult::ROW) {
        std::string releaseVerStr = releasever_query.get< std::string >("releasever");
        releasever[releasever_query.get< int64_t >("tid")] = releaseVerStr;
    }

    // iterate over history transactions
    SQLite3::Query query(*history.get(), trans_sql);
    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        auto trans = std::make_shared< TransformerTransaction >(swdb);
        trans->setId(query.get< int >("id"));
        trans->setDtBegin(query.get< int64_t >("dt_begin"));
        trans->setDtEnd(query.get< int64_t >("dt_end"));
        trans->setRpmdbVersionBegin(query.get< std::string >("rpmdb_version_begin"));
        trans->setRpmdbVersionEnd(query.get< std::string >("rpmdb_version_end"));

        // set release version if available
        auto it = releasever.find(trans->getId());
        if (it != releasever.end()) {
            trans->setReleasever(it->second);
        }

        trans->setUserId(query.get< int >("user_id"));
        trans->setCmdline(query.get< std::string >("cmdline"));

        TransactionState state = query.get< int >("state") == 0 ? TransactionState::DONE : TransactionState::ERROR;

        transformRPMItems(swdb, history, trans);
        transformTransWith(swdb, history, trans);

        trans->begin();

        transformOutput(history, trans);

        trans->finish(state);
    }
}

static void
fillRPMItem(std::shared_ptr< RPMItem > rpm, SQLite3::Query &query)
{
    rpm->setName(query.get< std::string >("name"));
    rpm->setEpoch(query.get< int64_t >("epoch"));
    rpm->setVersion(query.get< std::string >("version"));
    rpm->setRelease(query.get< std::string >("release"));
    rpm->setArch(query.get< std::string >("arch"));
    rpm->save();
}

/**
 * Transform binding between a Transaction and packages, which performed the transaction.
 * \param swdb pointer to swdb SQLite3 object
 * \param swdb pointer to history database SQLite3 object
 */
void
Transformer::transformTransWith(SQLite3Ptr swdb,
                                SQLite3Ptr history,
                                std::shared_ptr< TransformerTransaction > trans)
{
    const char *sql = R"**(
        SELECT
            name,
            epoch,
            version,
            release,
            arch
        FROM
            trans_with_pkgs
            JOIN pkgtups using (pkgtupid)
        WHERE
            tid=?
    )**";

    // transform stdout
    SQLite3::Query query(*history.get(), sql);
    query.bindv(trans->getId());
    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        // create RPM item object
        auto rpm = std::make_shared< RPMItem >(swdb);
        fillRPMItem(rpm, query);
        trans->addSoftwarePerformedWith(rpm);
    }
}

/**
 * Transform transaction console outputs.
 * \param swdb pointer to history database SQLite3 object
 */
void
Transformer::transformOutput(SQLite3Ptr history, std::shared_ptr< TransformerTransaction > trans)
{
    const char *sql = R"**(
        SELECT
            line
        FROM
            trans_script_stdout
        WHERE
            tid = ?
        ORDER BY
            lid
    )**";

    // transform stdout
    SQLite3::Query query(*history.get(), sql);
    query.bindv(trans->getId());
    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        trans->addConsoleOutputLine(1, query.get< std::string >("line"));
    }

    sql = R"**(
        SELECT
            msg
        FROM
            trans_error
        WHERE
            tid = ?
        ORDER BY
            mid
    )**";

    // transform stderr
    SQLite3::Query errorQuery(*history.get(), sql);
    errorQuery.bindv(trans->getId());
    while (errorQuery.step() == SQLite3::Statement::StepResult::ROW) {
        trans->addConsoleOutputLine(2, errorQuery.get< std::string >("msg"));
    }
}

static void
getYumdbData(int64_t itemId, SQLite3Ptr history, TransactionItemReason &reason, std::string &repoid)
{
    const char *sql = R"**(
        SELECT
            yumdb_key as key,
            yumdb_val as value
        FROM
            pkg_yumdb
        WHERE
            pkgtupid=?
            and key IN ('reason', 'from_repo')
    )**";

    // load reason and repoid data from yumdb
    SQLite3::Query query(*history.get(), sql);
    query.bindv(itemId);
    while (query.step() == SQLite3::Statement::StepResult::ROW) {
        std::string key = query.get< std::string >("key");
        if (key == "reason") {
            reason = Transformer::getReason(query.get< std::string >("value"));
        } else if (key == "from_repo") {
            repoid = query.get< std::string >("value");
        }
    }
}

/**
 * Transform RPM Items from a particular transaction.
 * \param swdb pointer to swdb SQLite3 object
 * \param swdb pointer to history database SQLite3 objects
 * \param trans Transaction whose items should be transformed
 */
void
Transformer::transformRPMItems(SQLite3Ptr swdb,
                               SQLite3Ptr history,
                               std::shared_ptr< TransformerTransaction > trans)
{
    // the order is important here - its Update, Updated
    const char *pkg_sql = R"**(
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

    SQLite3::Query query(*history.get(), pkg_sql);
    query.bindv(trans->getId());

    TransactionItemPtr last = nullptr;

    /*
     * Item in a single transaction can be both Obsoleted multiple times and Updated.
     * We need to keep track of all the obsoleted items,
     * so we can promote them to Updated in case.
     * Obsoleted records will be kept in item_replaced table,
     * so it's always obvious, that particular package was both Obsoleted
     * and Updated. Technically, we could replace action Obsoleted with action Erase.
     */
    std::map< int64_t, TransactionItemPtr > obsoletedItems;

    // interate over transaction packages in the history database
    while (query.step() == SQLite3::Statement::StepResult::ROW) {

        // create RPM item object
        auto rpm = std::make_shared< RPMItem >(swdb);
        fillRPMItem(rpm, query);

        // get item state/action
        std::string stateString = query.get< std::string >("state");
        TransactionItemAction action = actions.at(stateString);

        // `Obsoleting` record is duplicit with previous record (with different action)
        if (action == TransactionItemAction::OBSOLETE) {
            continue;
        }

        // find out if an item was previously obsoleted
        auto pastObsoleted = obsoletedItems.find(rpm->getId());

        TransactionItemPtr transItem = nullptr;

        if (pastObsoleted == obsoletedItems.end()) {
            // item hasn't been obsoleted yet

            // load reason and from_repo
            TransactionItemReason reason = TransactionItemReason::UNKNOWN;
            std::string repoid;
            getYumdbData(query.get< int64_t >("id"), history, reason, repoid);

            // add TransactionItem object
            transItem = trans->addItem(rpm, repoid, action, reason);
            transItem->setState(query.get< std::string >("done") == "TRUE" ? TransactionItemState::DONE : TransactionItemState::ERROR);
        } else {
            // item has been obsoleted - we just need to update the action
            transItem = pastObsoleted->second;
            transItem->setAction(action);
        }

        // resolve replaced by
        switch (action) {
            case TransactionItemAction::OBSOLETED:
                obsoletedItems[rpm->getId()] = transItem;
            case TransactionItemAction::DOWNGRADED:
            case TransactionItemAction::UPGRADED:
                transItem->addReplacedBy(last);
            default:
                break;
        }

        // keep the last item in case of obsoletes
        last = transItem;
    }
}

/**
 * Construct CompsGroupItem object from JSON
 * \param group group json object
 */
CompsGroupItemPtr
Transformer::processGroup(SQLite3Ptr swdb, const std::string &groupId, const Json::Value &group)
{
    // create group
    auto compsGroup = std::make_shared< CompsGroupItem >(swdb);
    compsGroup->setGroupId(groupId);
    compsGroup->setName(group["name"].asString());
    compsGroup->setTranslatedName(group["ui_name"].asString());

    // add installed packages - TODO pkg_types
    const Json::Value packages = group["full_list"];
    for (const auto &package : packages) {
        // XXX mandatory?
        compsGroup->addPackage(package.asString(), true, CompsPackageType::MANDATORY);
    }

    // add excluded packages
    const Json::Value excludes = group["pkg_exclude"];
    for (const auto &exclude : excludes) {
        // XXX mandatory?
        compsGroup->addPackage(exclude.asString(), false, CompsPackageType::MANDATORY);
    }

    compsGroup->save();
    return compsGroup;
}

/**
 * Construct CompsEnvironmentItem object from JSON
 * \param env environment json object
 */
std::shared_ptr< CompsEnvironmentItem >
Transformer::processEnvironment(SQLite3Ptr swdb, const std::string &envId, const Json::Value &env)
{
    // create environment
    auto compsEnv = std::make_shared< CompsEnvironmentItem >(swdb);
    compsEnv->setEnvironmentId(envId);
    compsEnv->setName(env["name"].asString());
    compsEnv->setTranslatedName(env["ui_name"].asString());

    // add installed groups - TODO pkg_types, grp_types
    const Json::Value groups = env["full_list"];
    for (const auto &group : groups) {
        // XXX mandatory?
        compsEnv->addGroup(group.asString(), true, CompsPackageType::MANDATORY);
    }

    // add excluded groups/packages
    const Json::Value excludes = env["pkg_exclude"];
    for (const auto &exclude : excludes) {
        // XXX mandatory?
        compsEnv->addGroup(exclude.asString(), false, CompsPackageType::MANDATORY);
    }

    compsEnv->save();

    return compsEnv;
}

/**
 * Create fake transaction for groups in persistor
 * \param swdb pointer to swdb SQLite3 object
 * \param root group persistor root node
 */
void
Transformer::processGroupPersistor(SQLite3Ptr swdb, const Json::Value &root)
{
    auto trans = libdnf::swdb_private::Transaction(swdb);

    // load sequences
    const Json::Value groups = root["GROUPS"];
    const Json::Value environments = root["ENVIRONMENTS"];

    // add groups
    for (const auto &groupId : groups.getMemberNames()) {
        trans.addItem(processGroup(swdb, groupId, groups[groupId]),
                      {}, // repoid
                      TransactionItemAction::INSTALL,
                      TransactionItemReason::USER);
    }

    // add environments
    for (const auto &envId : environments.getMemberNames()) {
        trans.addItem(processEnvironment(swdb, envId, environments[envId]),
                      {}, // repoid
                      TransactionItemAction::INSTALL,
                      TransactionItemReason::USER);
    }

    trans.begin();

    for (auto i : trans.getItems()) {
        i->setState(TransactionItemState::DONE);
        i->save();
    }

    trans.finish(TransactionState::DONE);
}

/**
 * Load group persistor into JSON object and perform transformation
 * \param swdb pointer to swdb SQLite3 object
 */
void
Transformer::transformGroups(SQLite3Ptr swdb)
{
    std::string groupsFile(inputDir);

    // create the groups.json path
    if (groupsFile.back() != '/') {
        groupsFile += '/';
    }
    groupsFile += "groups.json";

    std::ifstream groupsJsonFile(groupsFile, std::ifstream::binary);

    if (!groupsJsonFile.is_open()) {
        return;
    }

    // load the JSON file
    Json::Value root;
    groupsJsonFile >> root;

    processGroupPersistor(swdb, root);
}

/**
 * Try to find the history database in the inputDir
 * \return path to the latest history database in the inputDir
 */
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
    std::vector< std::string > possibleFiles;

    // open history directory
    struct dirent *dp;
    DIR *dirp = opendir(historyDir.c_str());

    if (!dirp) {
        throw Exception(_("Transformer: can't open history persist dir"));
    }

    // iterate over history directory
    while ((dp = readdir(dirp)) != nullptr) {
        std::string fileName(dp->d_name);

        // push the possible history DB files into vector
        if (fileName.find("history") != std::string::npos) {
            possibleFiles.push_back(fileName);
        }
    }

    if (possibleFiles.empty()) {
        throw Exception(_("Couldn't find a history database"));
    }

    // find the latest DB file
    std::sort(possibleFiles.begin(), possibleFiles.end());

    // return the path
    return historyDir + "/" + possibleFiles.back();
}
