/*
Copyright (C) 2017-2020 Red Hat, Inc.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/


#include "test_reason.hpp"

#include "libdnf/common/sack/query_cmp.hpp"
#include "libdnf/transaction/transaction.hpp"

#include <string>


using namespace libdnf::transaction;


CPPUNIT_TEST_SUITE_REGISTRATION(TransactionItemReasonTest);


void TransactionItemReasonTest::test_no_transaction() {
    auto base = new_base();
    auto & sack = base->get_transaction_sack();

    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack.get_comps_environment_reason("minimal"));
    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack.get_comps_group_reason("core"));
    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack.get_package_reason("bash", ""));
    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack.get_package_reason("bash", "x86_64"));
}


void TransactionItemReasonTest::test_empty_transaction() {
    auto base = new_base();
    auto & sack = base->get_transaction_sack();
    auto trans = sack.new_transaction();

    trans->start();
    trans->finish(TransactionState::DONE);

    // create a new Base to force reading the transaction from disk
    auto base2 = new_base();

    auto & sack2 = base2->get_transaction_sack();

    // fill the TransactionSack to load reasons from the database
    sack2.fill();

    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack2.get_comps_environment_reason("minimal"));
    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack2.get_comps_group_reason("core"));
    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack2.get_package_reason("bash", ""));
    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack2.get_package_reason("bash", "x86_64"));
}


static void add_rpm_bash_x86_64(TransactionWeakPtr & trans) {
    auto & pkg = trans->new_package();
    pkg.set_name("bash");
    pkg.set_arch("x86_64");
    pkg.set_action(TransactionItemAction::INSTALL);
    pkg.set_reason(TransactionItemReason::GROUP);
    pkg.set_state(TransactionItemState::DONE);
}


static void add_rpm_bash_i386(TransactionWeakPtr & trans) {
    auto & pkg = trans->new_package();
    pkg.set_name("bash");
    pkg.set_arch("i386");
    pkg.set_action(TransactionItemAction::INSTALL);
    pkg.set_reason(TransactionItemReason::USER);
    pkg.set_state(TransactionItemState::DONE);
}


static void add_comps_group_core(TransactionWeakPtr & trans) {
    auto & grp = trans->new_comps_group();
    grp.set_group_id("core");
    grp.set_action(TransactionItemAction::INSTALL);
    grp.set_reason(TransactionItemReason::USER);
    grp.set_state(TransactionItemState::DONE);
}


static void add_comps_environment_minimal(TransactionWeakPtr & trans) {
    auto & env = trans->new_comps_environment();
    env.set_environment_id("minimal");
    env.set_action(TransactionItemAction::INSTALL);
    env.set_reason(TransactionItemReason::USER);
    env.set_state(TransactionItemState::DONE);
}


void TransactionItemReasonTest::test_failed_transaction() {
    auto base = new_base();
    auto & sack = base->get_transaction_sack();
    auto trans = sack.new_transaction();

    add_comps_environment_minimal(trans);
    add_comps_group_core(trans);
    add_rpm_bash_x86_64(trans);

    trans->start();
    trans->finish(TransactionState::ERROR);

    // create a new Base to force reading the transaction from disk
    auto base2 = new_base();
    auto & sack2 = base2->get_transaction_sack();

    // fill the TransactionSack to load reasons from the database
    sack2.fill();

    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack2.get_comps_environment_reason("minimal"));

    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack2.get_comps_group_reason("core"));

    // if arch is not specified, return the highest reason for the given package name
    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack2.get_package_reason("bash", ""));

    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack2.get_package_reason("bash", "x86_64"));
}


void TransactionItemReasonTest::test_install() {
    auto base = new_base();
    auto & sack = base->get_transaction_sack();
    auto trans = sack.new_transaction();

    add_comps_environment_minimal(trans);
    add_comps_group_core(trans);
    add_rpm_bash_i386(trans);
    add_rpm_bash_x86_64(trans);

    trans->start();
    trans->finish(TransactionState::DONE);

    // create a new Base to force reading the transaction from disk
    auto base2 = new_base();
    auto & sack2 = base2->get_transaction_sack();

    // fill the TransactionSack to load reasons from the database
    sack2.fill();

    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::USER, sack2.get_comps_environment_reason("minimal"));

    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::USER, sack2.get_comps_group_reason("core"));

    // if arch is not specified, return the highest reason for the given package name
    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::USER, sack2.get_package_reason("bash", ""));

    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::USER, sack2.get_package_reason("bash", "i386"));
    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack2.get_package_reason("bash", "i686"));
    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::GROUP, sack2.get_package_reason("bash", "x86_64"));
}


void TransactionItemReasonTest::test_package_reason_change() {
    // create a transaction by calling another test case
    test_install();

    auto base = new_base();
    auto & sack = base->get_transaction_sack();
    auto trans = sack.new_transaction();

    auto & rpm_bash = trans->new_package();
    rpm_bash.set_name("bash");
    rpm_bash.set_arch("x86_64");
    rpm_bash.set_action(TransactionItemAction::REASON_CHANGE);
    rpm_bash.set_reason(TransactionItemReason::USER);
    rpm_bash.set_state(TransactionItemState::DONE);

    trans->start();
    trans->finish(TransactionState::DONE);

    // create a new Base to force reading the transaction from disk
    auto base2 = new_base();
    auto & sack2 = base2->get_transaction_sack();

    // fill the TransactionSack to load reasons from the database
    sack2.fill();

    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::USER, sack2.get_package_reason("bash", "x86_64"));
}


void TransactionItemReasonTest::test_remove() {
    // create a transaction by calling another test case
    test_install();

    auto base = new_base();
    auto & sack = base->get_transaction_sack();
    auto trans = sack.new_transaction();

    auto & rpm_bash = trans->new_package();
    rpm_bash.set_name("bash");
    rpm_bash.set_arch("x86_64");
    rpm_bash.set_action(TransactionItemAction::REMOVE);
    rpm_bash.set_reason(TransactionItemReason::USER);
    rpm_bash.set_state(TransactionItemState::DONE);

    auto & grp_core = trans->new_comps_group();
    grp_core.set_group_id("core");
    grp_core.set_action(TransactionItemAction::REMOVE);
    grp_core.set_reason(TransactionItemReason::USER);
    grp_core.set_state(TransactionItemState::DONE);

    auto & env_minimal = trans->new_comps_environment();
    env_minimal.set_environment_id("minimal");
    env_minimal.set_action(TransactionItemAction::REMOVE);
    env_minimal.set_reason(TransactionItemReason::USER);
    env_minimal.set_state(TransactionItemState::DONE);

    trans->start();
    trans->finish(TransactionState::DONE);

    // create a new Base to force reading the transaction from disk
    auto base2 = new_base();
    auto & sack2 = base2->get_transaction_sack();

    // fill the TransactionSack to load reasons from the database
    sack2.fill();

    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack2.get_comps_environment_reason("minimal"));

    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack2.get_comps_group_reason("core"));

    // if arch is not specified, return the highest reason for the given package name
    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::USER, sack2.get_package_reason("bash", ""));

    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::USER, sack2.get_package_reason("bash", "i386"));
    CPPUNIT_ASSERT_EQUAL(TransactionItemReason::UNKNOWN, sack2.get_package_reason("bash", "x86_64"));
}


void TransactionItemReasonTest::test_compare_operators() {
    CPPUNIT_ASSERT(TransactionItemReason::USER == TransactionItemReason::USER);
    CPPUNIT_ASSERT(TransactionItemReason::USER <= TransactionItemReason::USER);
    CPPUNIT_ASSERT(TransactionItemReason::USER >= TransactionItemReason::USER);

    CPPUNIT_ASSERT(TransactionItemReason::USER != TransactionItemReason::GROUP);
    CPPUNIT_ASSERT(TransactionItemReason::USER > TransactionItemReason::GROUP);
    CPPUNIT_ASSERT(TransactionItemReason::USER >= TransactionItemReason::GROUP);

    CPPUNIT_ASSERT(TransactionItemReason::GROUP != TransactionItemReason::USER);
    CPPUNIT_ASSERT(TransactionItemReason::GROUP < TransactionItemReason::USER);
    CPPUNIT_ASSERT(TransactionItemReason::GROUP <= TransactionItemReason::USER);
}


void TransactionItemReasonTest::test_TransactionItemReason_compare() {
    CPPUNIT_ASSERT_EQUAL(-1, TransactionItemReason_compare(TransactionItemReason::GROUP, TransactionItemReason::USER));
    CPPUNIT_ASSERT_EQUAL(0, TransactionItemReason_compare(TransactionItemReason::USER, TransactionItemReason::USER));
    CPPUNIT_ASSERT_EQUAL(1, TransactionItemReason_compare(TransactionItemReason::USER, TransactionItemReason::GROUP));
}
