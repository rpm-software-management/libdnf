/*
Copyright Contributors to the libdnf project.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "goal.hpp"

#include "callbacks.hpp"
#include "dbus.hpp"
#include "package.hpp"
#include "transaction.hpp"
#include "utils.hpp"

#include <fmt/format.h>
#include <libdnf/rpm/transaction.hpp>
#include <libdnf/transaction/transaction_item.hpp>
#include <sdbus-c++/sdbus-c++.h>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

void Goal::dbus_register() {
    auto dbus_object = session.get_dbus_object();
    dbus_object->registerMethod(
        dnfdaemon::INTERFACE_GOAL, "resolve", "a{sv}", "a(ua{sv})a{sv}", [this](sdbus::MethodCall call) -> void {
            session.get_threads_manager().handle_method(*this, &Goal::resolve, call);
        });
    dbus_object->registerMethod(
        dnfdaemon::INTERFACE_GOAL, "do_transaction", "a{sv}", "", [this](sdbus::MethodCall call) -> void {
            session.get_threads_manager().handle_method(*this, &Goal::do_transaction, call);
        });
}

sdbus::MethodReply Goal::resolve(sdbus::MethodCall & call) {
    // read options from dbus call
    dnfdaemon::KeyValueMap options;
    call >> options;
    bool allow_erasing = key_value_map_get<bool>(options, "allow_erasing", false);

    session.fill_sack();

    auto & goal = session.get_goal();
    auto transaction = goal.resolve(allow_erasing);
    session.set_transaction(transaction);

    std::vector<std::string> attr{
        "name", "epoch", "version", "release", "arch", "repo", "package_size", "install_size", "evr"};

    std::vector<dnfdaemon::DbusTransactionItem> result;

    for (auto & tspkg : transaction.get_transaction_packages()) {
        result.push_back(dnfdaemon::DbusTransactionItem(
            static_cast<unsigned int>(tspkg.get_action()), package_to_map(tspkg.get_package(), attr)));
    }

    auto reply = call.createReply();
    reply << result;

    dnfdaemon::KeyValueMapList goal_resolve_log_list;

    for (const auto & log : transaction.get_resolve_logs()) {
        dnfdaemon::KeyValueMap goal_resolve_log_item;
        goal_resolve_log_item["action"] = static_cast<uint32_t>(log.get_action());
        goal_resolve_log_item["problem"] = static_cast<uint32_t>(log.get_problem());
        // TODO(nsella) better use of KeyValueMap with GoalJobSettings
        dnfdaemon::KeyValueMap goal_job_settings;
        goal_job_settings.emplace(std::make_pair("to_repo_ids", log.get_job_settings()->to_repo_ids));
        goal_resolve_log_item["goal_job_settings"] = goal_job_settings;
        goal_resolve_log_item["report"] = *log.get_spec();  // string
        goal_resolve_log_item["report_list"] =
            std::vector<std::string>{log.get_additional_data()->begin(), log.get_additional_data()->end()};
        goal_resolve_log_list.push_back(goal_resolve_log_item);
    }

    dnfdaemon::KeyValueMap goal_resolve_results;
    goal_resolve_results["transaction_problems"] = static_cast<uint32_t>(transaction.get_problems());
    goal_resolve_results["transaction_solver_problems"] = transaction.all_package_solver_problems_to_string();
    goal_resolve_results["goal_problems"] = goal_resolve_log_list;

    reply << goal_resolve_results;

    return reply;
}

libdnf::transaction::TransactionWeakPtr new_db_transaction(libdnf::Base * base, const std::string & comment) {
    auto transaction_sack = base->get_transaction_sack();
    auto transaction = transaction_sack->new_transaction();
    // TODO(mblaha): user id
    //transaction->set_user_id(get_login_uid());
    if (!comment.empty()) {
        transaction->set_comment(comment);
    }
    auto vars = base->get_vars();
    if (vars->contains("releasever")) {
        transaction->set_releasever(base->get_vars()->get_value("releasever"));
    }

    // TODO (mblaha): command line for the transaction?
    //transaction->set_cmdline(cmd_line);

    // TODO(jrohel): nevra of running microdnf?
    //transaction->add_runtime_package("microdnf");

    return transaction;
}

// TODO (mblaha) shared download_packages with microdnf / libdnf
// TODO (mblaha) callbacks to report the status
void download_packages(Session & session, libdnf::base::Transaction & transaction) {
    libdnf::repo::PackageDownloader downloader;

    std::vector<std::unique_ptr<DbusPackageCB>> download_callbacks;

    for (auto & tspkg : transaction.get_transaction_packages()) {
        if (tspkg.get_action() == libdnf::transaction::TransactionItemAction::INSTALL ||
            tspkg.get_action() == libdnf::transaction::TransactionItemAction::REINSTALL ||
            tspkg.get_action() == libdnf::transaction::TransactionItemAction::UPGRADE ||
            tspkg.get_action() == libdnf::transaction::TransactionItemAction::DOWNGRADE) {
            download_callbacks.push_back(std::make_unique<DbusPackageCB>(session, tspkg.get_package()));
            downloader.add(tspkg.get_package(), download_callbacks.back().get());
        }
    }

    downloader.download(true, true);
}

sdbus::MethodReply Goal::do_transaction(sdbus::MethodCall & call) {
    if (!session.check_authorization(dnfdaemon::POLKIT_EXECUTE_RPM_TRANSACTION, call.getSender())) {
        throw std::runtime_error("Not authorized");
    }

    // read options from dbus call
    dnfdaemon::KeyValueMap options;
    call >> options;
    std::string comment = key_value_map_get<std::string>(options, "comment", "");

    // TODO(mblaha): ensure that system repo is not loaded twice
    //session.fill_sack();

    auto base = session.get_base();
    auto * transaction = session.get_transaction();

    download_packages(session, *transaction);

    libdnf::rpm::Transaction rpm_transaction(*base);
    rpm_transaction.fill(*transaction);

    auto db_transaction = new_db_transaction(base, comment);
    db_transaction->fill_transaction_packages(transaction->get_transaction_packages());

    auto time = std::chrono::system_clock::now().time_since_epoch();
    db_transaction->set_dt_start(std::chrono::duration_cast<std::chrono::seconds>(time).count());
    db_transaction->start();

    DbusTransactionCB callback(session);
    rpm_transaction.register_cb(&callback);
    auto rpm_result = rpm_transaction.run();
    callback.finish();
    rpm_transaction.register_cb(nullptr);
    if (rpm_result != 0) {
        throw sdbus::Error(
            dnfdaemon::ERROR_TRANSACTION, fmt::format("rpm transaction failed with code {}.", rpm_result));
    }

    time = std::chrono::system_clock::now().time_since_epoch();
    db_transaction->set_dt_end(std::chrono::duration_cast<std::chrono::seconds>(time).count());
    db_transaction->finish(libdnf::transaction::TransactionState::DONE);

    // TODO(mblaha): clean up downloaded packages after successfull transaction

    auto reply = call.createReply();
    return reply;
}
