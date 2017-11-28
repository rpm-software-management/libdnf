#include "HandleTest.hpp"
#include <cstdio>
#include <string>

CPPUNIT_TEST_SUITE_REGISTRATION (HandleTest);

const char *path = "/tmp/swdb_test.db";

void
HandleTest::setUp ()
{
    handle = new Handle (path);
}

void
HandleTest::tearDown ()
{
    delete handle;
}

void
HandleTest::testPath ()
{
    CPPUNIT_ASSERT_EQUAL (handle->getPath (), path);
}

void
HandleTest::testCreate ()
{
    remove (path);
    CPPUNIT_ASSERT (handle->exists () == false);
    handle->createDB ();
    CPPUNIT_ASSERT (handle->exists () == true);
}

void
HandleTest::testReset ()
{
    CPPUNIT_ASSERT (handle->exists () == true);
    handle->resetDB ();
    CPPUNIT_ASSERT (handle->exists () == true);
}

void
HandleTest::testPrepare ()
{
    handle->exec ("CREATE TABLE test(id INTEGER, name TEXT)");
    handle->exec ("INSERT INTO test VALUES (1, \"foo\")");
    handle->exec ("INSERT INTO test VALUES (2, \"bar\")");

    const char *sql = "SELECT id, name FROM test WHERE id = ?";

    Statement statement = handle->prepare (sql, 1);

    CPPUNIT_ASSERT (statement.step ());

    int id = -1;
    statement.getColumn (0, id);
    std::string name;
    statement.getColumn (1, name);

    CPPUNIT_ASSERT_EQUAL (id, 1);
    CPPUNIT_ASSERT_EQUAL (name, std::string ("foo"));
}
