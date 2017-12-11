#include <cstdio>
#include <iostream>
#include <string>

#include "WorkflowTest.hpp"
#include "libdnf/swdb/item_rpm.hpp"
#include "libdnf/swdb/swdb.hpp"
#include "libdnf/swdb/transaction.hpp"
#include "libdnf/swdb/transactionitem.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(WorkflowTest);

void
WorkflowTest::setUp()
{
    conn = std::unique_ptr<SQLite3>(new SQLite3(":memory:"));
    SwdbCreateDatabase(*conn.get());
}

void
WorkflowTest::tearDown()
{
}

void
WorkflowTest::testDefaultWorkflow()
{
    // STEP 1: create transaction object
    auto trans = Transaction(*conn.get());
    CPPUNIT_ASSERT(trans.getDone() == false);

    // STEP 2: set vars
    trans.setReleasever("26");

    // populate goal
    // resolve dependencies
    // prepare RPM transaction

    // STEP 3: associate RPMs to the transaction
    // bash-4.4.12-5.fc26.x86_64
    auto rpm_bash = std::make_shared<RPMItem>(*conn.get());
    rpm_bash->setName("bash");
    rpm_bash->setEpoch(0);
    rpm_bash->setVersion("4.4.12");
    rpm_bash->setRelease("5.fc26");
    rpm_bash->setArch("x86_64");
    rpm_bash->setChecksumType("sha256");
    rpm_bash->setChecksumData("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    std::string repoid = "base";
    TransactionItemAction action = TransactionItemAction::INSTALL;
    TransactionItemReason reason = TransactionItemReason::USER;
    trans.addItem(rpm_bash, repoid, action, reason, NULL);

    // systemd-233-6.fc26
    auto rpm_systemd = std::make_shared<RPMItem>(*conn.get());
    rpm_systemd->setName("systemd");
    rpm_systemd->setEpoch(0);
    rpm_systemd->setVersion("233");
    rpm_systemd->setRelease("6.fc26");
    rpm_systemd->setArch("x86_64");
    rpm_systemd->setChecksumType("sha256");
    rpm_systemd->setChecksumData(
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    repoid = "base";
    action = TransactionItemAction::OBSOLETE;
    reason = TransactionItemReason::USER;
    auto ti_rpm_systemd = trans.addItem(rpm_systemd, repoid, action, reason, NULL);

    // sysvinit-2.88-14.dsf.fc20
    auto rpm_sysvinit = std::make_shared<RPMItem>(*conn.get());
    rpm_sysvinit->setName("sysvinit");
    rpm_sysvinit->setEpoch(0);
    rpm_sysvinit->setVersion("2.88");
    rpm_sysvinit->setRelease("14.dsf.fc20");
    rpm_sysvinit->setArch("x86_64");
    rpm_sysvinit->setChecksumType("sha256");
    rpm_sysvinit->setChecksumData(
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    repoid = "f20";
    action = TransactionItemAction::OBSOLETED;
    reason = TransactionItemReason::USER;
    trans.addItem(rpm_sysvinit, repoid, action, reason, ti_rpm_systemd);

    // STEP 4: save transaction and all associated items
    trans.save();
    trans.saveItems();

    // STEP 5: run RPM transaction; callback: mark completed items
    for (auto i : trans.getItems()) {
        i->setDone(true);
        i->save();
    }

    // STEP 6
    // mark completed transaction
    trans.setDone(true);
    trans.save();
    CPPUNIT_ASSERT(trans.getDone() == true);

    // VERIFY
    // verify that data is available via public API
    auto trans2 = Transaction(*conn.get(), trans.getId());
    CPPUNIT_ASSERT(trans2.getDone() == true);

    trans2.loadItems();
    CPPUNIT_ASSERT(trans2.getItems().size() == 3);

    for (auto i : trans2.getItems()) {
        CPPUNIT_ASSERT(i->getItem()->getItemType() == "rpm");
        CPPUNIT_ASSERT(i->getDone() == true);
        // std::cout << "TransactionItem: " << i->getItem()->toStr() << std::endl;
    }
}
