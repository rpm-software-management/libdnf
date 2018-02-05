#include <set>
#include <string>

#include "MergedTransactionTest.hpp"
#include "libdnf/swdb/item_rpm.hpp"
#include "libdnf/swdb/mergedtransaction.hpp"
#include "libdnf/swdb/swdb.hpp"
#include "libdnf/swdb/transaction.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(MergedTransactionTest);

void
MergedTransactionTest::setUp()
{
    conn = std::make_shared< SQLite3 >(":memory:");
    Swdb swdb(conn);
    swdb.createDatabase();
}

static TransactionPtr
initTransFirst(SQLite3Ptr conn)
{
    // create the first transaction
    auto first = std::make_shared< Transaction >(conn);
    first->setDtBegin(1);
    first->setDtEnd(2);
    first->setRpmdbVersionBegin("begin 1");
    first->setRpmdbVersionEnd("end 1");
    first->setUserId(1000);
    first->setCmdline("dnf install foo");

    auto dnfRpm = std::make_shared< RPMItem >(conn);
    dnfRpm->setName("dnf");
    dnfRpm->setEpoch(0);
    dnfRpm->setVersion("3.0.0");
    dnfRpm->setRelease("2.fc26");
    dnfRpm->setArch("x86_64");
    dnfRpm->save();

    first->addSoftwarePerformedWith(dnfRpm);
    first->begin();

    return first;
}

static TransactionPtr
initTransSecond(SQLite3Ptr conn)
{
    // create the second transaction
    auto second = std::make_shared< Transaction >(conn);
    second->setDtBegin(3);
    second->setDtEnd(4);
    second->setRpmdbVersionBegin("begin 2");
    second->setRpmdbVersionEnd("end 2");
    second->setUserId(1001);
    second->setCmdline("dnf install bar");

    auto rpmRpm = std::make_shared< RPMItem >(conn);
    rpmRpm->setName("rpm");
    rpmRpm->setEpoch(0);
    rpmRpm->setVersion("4.14.0");
    rpmRpm->setRelease("2.fc26");
    rpmRpm->setArch("x86_64");
    rpmRpm->save();

    second->addSoftwarePerformedWith(rpmRpm);
    second->begin();

    return second;
}

void
MergedTransactionTest::tearDown()
{
}

void
MergedTransactionTest::testMerge()
{
    auto first = initTransFirst(conn);
    first->addConsoleOutputLine(1, "Foo");
    first->finish(true);

    auto second = initTransSecond(conn);
    second->addConsoleOutputLine(1, "Bar");
    second->finish(false);

    MergedTransaction merged(first);
    merged.merge(second);

    CPPUNIT_ASSERT_EQUAL((int64_t)1, merged.listIds().at(0));
    CPPUNIT_ASSERT_EQUAL((int64_t)2, merged.listIds().at(1));

    CPPUNIT_ASSERT_EQUAL((int64_t)1000, merged.listUserIds().at(0));
    CPPUNIT_ASSERT_EQUAL((int64_t)1001, merged.listUserIds().at(1));

    CPPUNIT_ASSERT_EQUAL(std::string("dnf install foo"), merged.listCmdlines().at(0));
    CPPUNIT_ASSERT_EQUAL(std::string("dnf install bar"), merged.listCmdlines().at(1));

    CPPUNIT_ASSERT_EQUAL(true, (bool)merged.listDone().at(0));
    CPPUNIT_ASSERT_EQUAL(false, (bool)merged.listDone().at(1));

    CPPUNIT_ASSERT_EQUAL((int64_t)1, merged.getDtBegin());
    CPPUNIT_ASSERT_EQUAL((int64_t)4, merged.getDtEnd());

    CPPUNIT_ASSERT_EQUAL(std::string("begin 1"), merged.getRpmdbVersionBegin());
    CPPUNIT_ASSERT_EQUAL(std::string("end 2"), merged.getRpmdbVersionEnd());

    auto output = merged.getConsoleOutput();
    CPPUNIT_ASSERT_EQUAL(1, output.at(0).first);
    CPPUNIT_ASSERT_EQUAL(std::string("Foo"), output.at(0).second);

    CPPUNIT_ASSERT_EQUAL(1, output.at(1).first);
    CPPUNIT_ASSERT_EQUAL(std::string("Bar"), output.at(1).second);

    auto software = merged.getSoftwarePerformedWith();
    std::set< std::string > names = {"rpm", "dnf"};

    CPPUNIT_ASSERT(names.size() == 2);

    for (auto s : software) {
        const std::string &name = s->getName();
        CPPUNIT_ASSERT_MESSAGE("Name: " + name, names.find(name) != names.end());
        names.erase(name);
    }
}

static MergedTransactionPtr
prepareMergedTransaction(SQLite3Ptr conn,
                         TransactionItemAction actionFirst,
                         TransactionItemAction actionSecond,
                         const std::string &versionFirst,
                         const std::string &versionSecond,
                         TransactionItemAction oldFirstAction = TransactionItemAction::INSTALL,
                         const std::string &oldFirstVersion = {}

)
{
    auto firstRPM = std::make_shared< RPMItem >(conn);
    firstRPM->setName("foo");
    firstRPM->setEpoch(0);
    firstRPM->setVersion(versionFirst);
    firstRPM->setRelease("2.fc26");
    firstRPM->setArch("x86_64");
    firstRPM->save();

    auto secondRPM = std::make_shared< RPMItem >(conn);
    secondRPM->setName("foo");
    secondRPM->setEpoch(0);
    secondRPM->setVersion(versionSecond);
    secondRPM->setRelease("2.fc26");
    secondRPM->setArch("x86_64");
    secondRPM->save();

    auto first = initTransFirst(conn);
    if (oldFirstAction != TransactionItemAction::INSTALL) {
        auto oldFirst = std::make_shared< RPMItem >(conn);
        oldFirst->setName("foo");
        oldFirst->setEpoch(0);
        oldFirst->setVersion(oldFirstVersion);
        oldFirst->setRelease("2.fc26");
        oldFirst->setArch("x86_64");
        oldFirst->save();
        first->addItem(oldFirst, "base", oldFirstAction, TransactionItemReason::USER);
    }

    first->addItem(firstRPM, "base", actionFirst, TransactionItemReason::USER);
    first->finish(true);

    auto second = initTransSecond(conn);
    second->addItem(secondRPM, "base", actionSecond, TransactionItemReason::USER);
    second->finish(true);

    auto merged = std::make_shared< MergedTransaction >(first);
    merged->merge(second);
    return merged;
}

/// Erase -> Install = Reinstall
void
MergedTransactionTest::testMergeEraseInstallReinstall()
{
    auto merged = prepareMergedTransaction(
        conn, TransactionItemAction::REMOVE, TransactionItemAction::INSTALL, "1.0.0", "1.0.0");

    auto items = merged->getItems();
    CPPUNIT_ASSERT_EQUAL(1, (int)items.size());
    auto item = items.at(0);
    CPPUNIT_ASSERT_EQUAL(TransactionItemAction::REINSTALL, item->getAction());
}

/// Erase -> Install = Downgrade
void
MergedTransactionTest::testMergeEraseInstallDowngrade()
{
    auto merged = prepareMergedTransaction(
        conn, TransactionItemAction::REMOVE, TransactionItemAction::INSTALL, "0.11.0", "0.10.9");

    auto items = merged->getItems();

    CPPUNIT_ASSERT_EQUAL(1, (int)items.size());
    auto item = items.at(0);
    CPPUNIT_ASSERT_EQUAL(TransactionItemAction::DOWNGRADE, item->getAction());
}

/// Erase -> Install = Upgrade
void
MergedTransactionTest::testMergeEraseInstallUpgrade()
{
    auto merged = prepareMergedTransaction(
        conn, TransactionItemAction::OBSOLETED, TransactionItemAction::INSTALL, "0.11.0", "0.12.0");

    auto items = merged->getItems();
    CPPUNIT_ASSERT_EQUAL(1, (int)items.size());
    auto item = items.at(0);
    CPPUNIT_ASSERT_EQUAL(TransactionItemAction::UPGRADE, item->getAction());
}

/// Reinstall/Reason change -> (new action) = (new action)
void
MergedTransactionTest::testMergeReinstallAny()
{
    auto merged = prepareMergedTransaction(conn,
                                           TransactionItemAction::REINSTALL,
                                           TransactionItemAction::UPGRADE,
                                           "1.0.0",
                                           "1.0.1",
                                           TransactionItemAction::UPGRADED,
                                           "0.9.9");

    auto items = merged->getItems();
    CPPUNIT_ASSERT_EQUAL(2, (int)items.size());
    auto item = items.at(1);
    CPPUNIT_ASSERT_EQUAL(TransactionItemAction::UPGRADE, item->getAction());
    auto oldItem = items.at(0);
    CPPUNIT_ASSERT_EQUAL(TransactionItemAction::UPGRADED, oldItem->getAction());
}

/// Install -> Erase = (nothing)
void
MergedTransactionTest::testMergeInstallErase()
{
    auto merged = prepareMergedTransaction(
        conn, TransactionItemAction::INSTALL, TransactionItemAction::REMOVE, "1.0.0", "1.0.0");

    auto items = merged->getItems();

    // nothing
    CPPUNIT_ASSERT_EQUAL(0, (int)items.size());
}

/// Install -> Upgrade/Downgrade = Install (with Upgrade version)

void
MergedTransactionTest::testMergeInstallAlter()
{
    auto merged = prepareMergedTransaction(
        conn, TransactionItemAction::INSTALL, TransactionItemAction::UPGRADE, "1.0.0", "1.0.1");

    auto items = merged->getItems();
    CPPUNIT_ASSERT_EQUAL(1, (int)items.size());
    auto item = items.at(0);
    CPPUNIT_ASSERT_EQUAL(TransactionItemAction::INSTALL, item->getAction());
    auto rpm = std::dynamic_pointer_cast< RPMItem >(item->getItem());
    CPPUNIT_ASSERT_EQUAL(std::string("1.0.1"), rpm->getVersion());
}

/// Downgrade/Upgrade/Obsoleting -> Reinstall = (old action)
void
MergedTransactionTest::testMergeAlterReinstall()
{
    auto merged = prepareMergedTransaction(conn,
                                           TransactionItemAction::UPGRADE,
                                           TransactionItemAction::REINSTALL,
                                           "1.0.0",
                                           "1.0.0",
                                           TransactionItemAction::UPGRADED,
                                           "0.9.9");

    auto items = merged->getItems();
    CPPUNIT_ASSERT_EQUAL(2, (int)items.size());
    auto item = items.at(1);
    CPPUNIT_ASSERT_EQUAL(TransactionItemAction::UPGRADE, item->getAction());
    auto oldItem = items.at(0);
    CPPUNIT_ASSERT_EQUAL(TransactionItemAction::UPGRADED, oldItem->getAction());
}

/// Downgrade/Upgrade/Obsoleting -> Erase/Obsoleted = Erase/Obsolete (with old package)
void
MergedTransactionTest::testMergeAlterErase()
{
    auto merged = prepareMergedTransaction(conn,
                                           TransactionItemAction::UPGRADE,
                                           TransactionItemAction::REMOVE,
                                           "1.0.0",
                                           "1.0.0",
                                           TransactionItemAction::UPGRADED,
                                           "0.9.9");
    auto items = merged->getItems();
    CPPUNIT_ASSERT_EQUAL(1, (int)items.size());
    auto item = items.at(0);
    CPPUNIT_ASSERT_EQUAL(TransactionItemAction::REMOVE, item->getAction());
    auto rpm = std::dynamic_pointer_cast< RPMItem >(item->getItem());
    CPPUNIT_ASSERT_EQUAL(std::string("0.9.9"), rpm->getVersion());
}

/// Downgrade/Upgrade/Obsoleting -> Downgrade/Upgrade = Downgrade/Upgrade/Reinstall
void
MergedTransactionTest::testMergeAlterAlter()
{
    auto merged = prepareMergedTransaction(conn,
                                           TransactionItemAction::DOWNGRADE,
                                           TransactionItemAction::UPGRADE,
                                           "1.0.0",
                                           "1.0.1",
                                           TransactionItemAction::DOWNGRADED,
                                           "1.0.1");
    auto items = merged->getItems();
    CPPUNIT_ASSERT_EQUAL(1, (int)items.size());
    auto item = items.at(0);
    CPPUNIT_ASSERT_EQUAL(TransactionItemAction::REINSTALL, item->getAction());
}
