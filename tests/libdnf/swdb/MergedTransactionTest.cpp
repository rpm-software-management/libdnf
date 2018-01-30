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

void
MergedTransactionTest::tearDown()
{
}

void
MergedTransactionTest::testMerge()
{
    auto first = std::make_shared< Transaction >(conn);
    first->setDtBegin(1);
    first->setDtEnd(2);
    first->setRpmdbVersionBegin("begin 1");
    first->setRpmdbVersionEnd("end 1");
    first->setUserId(1000);
    first->setCmdline("dnf install foo");
    first->begin();
    first->addConsoleOutputLine(1, "Foo");
    first->finish(true);

    auto second = std::make_shared< Transaction >(conn);
    second->setDtBegin(3);
    second->setDtEnd(4);
    second->setRpmdbVersionBegin("begin 2");
    second->setRpmdbVersionEnd("end 2");
    second->setUserId(1001);
    second->setCmdline("dnf install bar");
    second->begin();
    first->addConsoleOutputLine(1, "Bar");
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

    // TODO software performed with, packages
}
