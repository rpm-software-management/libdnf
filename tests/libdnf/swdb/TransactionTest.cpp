#include "TransactionTest.hpp"
#include "libdnf/swdb/item_rpm.hpp"
#include "libdnf/swdb/swdb.hpp"
#include "libdnf/swdb/transaction.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(TransactionTest);

void
TransactionTest::setUp()
{
    conn = std::make_shared< SQLite3 >(":memory:");
    Swdb swdb(conn);
    swdb.createDatabase();
}

void
TransactionTest::tearDown()
{
}

void
TransactionTest::testInsert()
{
    Transaction trans(conn);
    trans.setDtBegin(1);
    trans.setDtEnd(2);
    trans.setRpmdbVersionBegin("begin - TransactionTest::testInsert");
    trans.setRpmdbVersionEnd("end - TransactionTest::testInsert");
    trans.setReleasever("26");
    trans.setUserId(1000);
    trans.setCmdline("dnf install foo");
    trans.setDone(false);
    trans.save();
    // 2nd save must pass and do nothing
    trans.save();

    // load the saved transaction from database and compare values
    Transaction trans2(conn, trans.getId());
    CPPUNIT_ASSERT(trans2.getId() == trans.getId());
    CPPUNIT_ASSERT(trans2.getDtBegin() == trans.getDtBegin());
    CPPUNIT_ASSERT(trans2.getDtEnd() == trans.getDtEnd());
    CPPUNIT_ASSERT(trans2.getRpmdbVersionBegin() == trans.getRpmdbVersionBegin());
    CPPUNIT_ASSERT(trans2.getRpmdbVersionEnd() == trans.getRpmdbVersionEnd());
    CPPUNIT_ASSERT(trans2.getReleasever() == trans.getReleasever());
    CPPUNIT_ASSERT(trans2.getUserId() == trans.getUserId());
    CPPUNIT_ASSERT(trans2.getCmdline() == trans.getCmdline());
    CPPUNIT_ASSERT(trans2.getDone() == trans.getDone());
}

void
TransactionTest::testInsertWithSpecifiedId()
{
    // it is not allowed to save a transaction with arbitrary ID
    Transaction trans(conn);
    trans.setId(INT64_MAX);
    trans.save();
    // CPPUNIT_ASSERT_THROW(trans.save(), SQLError);
    // TODO: throw a more specific exception
}

void
TransactionTest::testUpdate()
{
    testInsert();
    Transaction trans(conn, 1);
    trans.setDtBegin(10);
    trans.setDtEnd(20);
    trans.setRpmdbVersionBegin("begin - TransactionTest::testUpdate");
    trans.setRpmdbVersionEnd("end - TransactionTest::testUpdate");
    trans.save();

    Transaction trans2(conn, trans.getId());
    CPPUNIT_ASSERT(trans2.getId() == trans.getId());
    CPPUNIT_ASSERT(trans2.getDtBegin() == trans.getDtBegin());
    CPPUNIT_ASSERT(trans2.getDtEnd() == trans.getDtEnd());
    CPPUNIT_ASSERT(trans2.getRpmdbVersionBegin() == trans.getRpmdbVersionBegin());
    CPPUNIT_ASSERT(trans2.getRpmdbVersionEnd() == trans.getRpmdbVersionEnd());
    CPPUNIT_ASSERT(trans2.getReleasever() == trans.getReleasever());
    CPPUNIT_ASSERT(trans2.getUserId() == trans.getUserId());
    CPPUNIT_ASSERT(trans2.getCmdline() == trans.getCmdline());
    CPPUNIT_ASSERT(trans2.getDone() == trans.getDone());
}

void
TransactionTest::testAddItem()
{
}
