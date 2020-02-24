#include <string>

#include "libdnf/transaction/Swdb.hpp"
#include "libdnf/transaction/Transaction.hpp"
#include "libdnf/transaction/Transformer.hpp"

#include "MigrationTest.hpp"

using namespace libdnf;

CPPUNIT_TEST_SUITE_REGISTRATION(MigrationTest);

static const char * const test_sql_create_tables =
#include "libdnf/transaction/sql/create_tables.sql"
    ;

void
MigrationTest::setUp()
{
    history = std::make_shared< SQLite3 >(":memory:");
    history.get()->exec(test_sql_create_tables);
}

void
MigrationTest::testVersionMissing()
{
    history.get()->exec("delete from config where key='version';");
    try{
        Swdb swdb(history);
    }
    catch (Transformer::Exception & ex) {
        CPPUNIT_ASSERT_EQUAL(
                std::string(ex.what()),
                std::string("Database Corrupted: no row 'version' in table 'config'"));
    }
}

void
MigrationTest::testVersionBeforeMigration()
{
    SQLite3::Query query(*history, "select value from config where key = 'version';");
    query.step();
    // hardcoded 1.1 is fine since it is the initial version
    CPPUNIT_ASSERT_EQUAL(query.get<std::string>("value"), std::string("1.1"));
}

void
MigrationTest::testVersionAfterMigration()
{
    Swdb swdb(history);
    SQLite3::Query query(*history, "select value from config where key = 'version';");
    query.step();
    CPPUNIT_ASSERT_EQUAL(query.get<std::string>("value"), std::string(Transformer::getVersion()));
}

void
MigrationTest::testEmptyCommentAfterMigration()
{
    history.get()->exec("INSERT INTO trans VALUES(1,1,1,'','','1',-1,'',1);"); // make record of old transaction
    Swdb swdb(history); // migrate
    Transaction trans(history, 1);
    CPPUNIT_ASSERT_EQUAL(trans.getComment(), std::string());
}

void
MigrationTest::testNonEmptyCommentAfterMigration()
{
    Swdb swdb(history); // migrate
    swdb.initTransaction();
    swdb.beginTransaction(1, "", "", 0, "Test comment");
    swdb.endTransaction(2, "", TransactionState::DONE);
    swdb.closeTransaction();
    Transaction trans(history, 1);
    CPPUNIT_ASSERT_EQUAL(trans.getComment(), std::string("Test comment"));
}

void
MigrationTest::tearDown()
{
}
