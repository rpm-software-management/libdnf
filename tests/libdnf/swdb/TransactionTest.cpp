#include "TransactionTest.hpp"
#include "libdnf/swdb/RPMItem.hpp"
#include "libdnf/swdb/Transaction.hpp"
#include "libdnf/swdb/private/Transaction.hpp"
#include "libdnf/swdb/Transformer.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(TransactionTest);

void
TransactionTest::setUp()
{
    conn = std::make_shared< SQLite3 >(":memory:");
    Transformer::createDatabase(conn);
}

void
TransactionTest::tearDown()
{
}

void
TransactionTest::testInsert()
{
    libdnf::swdb_private::Transaction trans(conn);
    trans.setDtBegin(1);
    trans.setDtEnd(2);
    trans.setRpmdbVersionBegin("begin - TransactionTest::testInsert");
    trans.setRpmdbVersionEnd("end - TransactionTest::testInsert");
    trans.setReleasever("26");
    trans.setUserId(1000);
    trans.setCmdline("dnf install foo");
    trans.setState(TransactionState::DONE);
    trans.begin();

    // 2nd begin must throw an exception
    CPPUNIT_ASSERT_THROW(trans.begin(), std::runtime_error);

    // load the saved transaction from database and compare values
    libdnf::Transaction trans2(conn, trans.getId());
    CPPUNIT_ASSERT(trans2.getId() == trans.getId());
    CPPUNIT_ASSERT(trans2.getDtBegin() == trans.getDtBegin());
    CPPUNIT_ASSERT(trans2.getDtEnd() == trans.getDtEnd());
    CPPUNIT_ASSERT(trans2.getRpmdbVersionBegin() == trans.getRpmdbVersionBegin());
    CPPUNIT_ASSERT(trans2.getRpmdbVersionEnd() == trans.getRpmdbVersionEnd());
    CPPUNIT_ASSERT(trans2.getReleasever() == trans.getReleasever());
    CPPUNIT_ASSERT(trans2.getUserId() == trans.getUserId());
    CPPUNIT_ASSERT(trans2.getCmdline() == trans.getCmdline());
    CPPUNIT_ASSERT(trans2.getState() == trans.getState());
}

void
TransactionTest::testInsertWithSpecifiedId()
{
    // it is not allowed to save a transaction with arbitrary ID
    libdnf::swdb_private::Transaction trans(conn);
    trans.setId(INT64_MAX);
    CPPUNIT_ASSERT_THROW(trans.begin(), std::runtime_error);
}

void
TransactionTest::testUpdate()
{
    libdnf::swdb_private::Transaction trans(conn);
    trans.setDtBegin(1);
    trans.setDtEnd(2);
    trans.setRpmdbVersionBegin("begin - TransactionTest::testInsert");
    trans.setRpmdbVersionEnd("end - TransactionTest::testInsert");
    trans.setReleasever("26");
    trans.setUserId(1000);
    trans.setCmdline("dnf install foo");
    trans.setState(TransactionState::ERROR);
    trans.begin();
    trans.finish(TransactionState::DONE);

    libdnf::Transaction trans2(conn, trans.getId());
    CPPUNIT_ASSERT(trans2.getId() == trans.getId());
    CPPUNIT_ASSERT(trans2.getDtBegin() == trans.getDtBegin());
    CPPUNIT_ASSERT(trans2.getDtEnd() == trans.getDtEnd());
    CPPUNIT_ASSERT(trans2.getRpmdbVersionBegin() == trans.getRpmdbVersionBegin());
    CPPUNIT_ASSERT(trans2.getRpmdbVersionEnd() == trans.getRpmdbVersionEnd());
    CPPUNIT_ASSERT(trans2.getReleasever() == trans.getReleasever());
    CPPUNIT_ASSERT(trans2.getUserId() == trans.getUserId());
    CPPUNIT_ASSERT(trans2.getCmdline() == trans.getCmdline());
    CPPUNIT_ASSERT_EQUAL(TransactionState::DONE, trans2.getState());
}

void
TransactionTest::testComparison()
{
    // test operator ==, > and <
    libdnf::swdb_private::Transaction first(conn);
    libdnf::swdb_private::Transaction second(conn);

    // id comparison test
    first.setId(1);
    second.setId(2);
    CPPUNIT_ASSERT(first > second);
    CPPUNIT_ASSERT(second < first);

    // begin timestamp comparison test
    second.setId(1);
    first.setDtBegin(1);
    second.setDtBegin(2);
    CPPUNIT_ASSERT(first > second);
    CPPUNIT_ASSERT(second < first);

    // rpmdb comparison test
    second.setDtBegin(1);
    first.setRpmdbVersionBegin("0");
    second.setRpmdbVersionBegin("1");
    CPPUNIT_ASSERT(first > second);
    CPPUNIT_ASSERT(second < first);

    // equality
    second.setRpmdbVersionBegin("0");
    CPPUNIT_ASSERT(first == second);
}
