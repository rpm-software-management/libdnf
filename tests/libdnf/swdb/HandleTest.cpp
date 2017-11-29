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
HandleTest::testExec ()
{
    handle->exec ("CREATE TABLE test(id INTEGER, name TEXT)");
    handle->exec ("INSERT INTO test VALUES (1, \"foo\")");
    handle->exec ("INSERT INTO test VALUES (2, \"bar\")");

    const char *sql = "SELECT id, name FROM test WHERE id = ?";

    handle->prepare (sql, 1);

    CPPUNIT_ASSERT (handle->step ());

    int id = handle->getInt (0);
    std::string name = handle->getString (1);

    CPPUNIT_ASSERT_EQUAL (id, 1);
    CPPUNIT_ASSERT_EQUAL (name, std::string ("foo"));
}

void
HandleTest::testPrepare ()
{
    const char *sql = "SELECT id, name FROM test WHERE id = ?";

    handle->prepare (sql, 1);
    CPPUNIT_ASSERT (handle->step ());

    int id = handle->getInt (0);
    std::string name = handle->getString (1);

    CPPUNIT_ASSERT_EQUAL (id, 1);
    CPPUNIT_ASSERT_EQUAL (name, std::string ("foo"));
}

void
HandleTest::testInsert ()
{
    const char *sql = "INSERT INTO item VALUES(null, ?)";
    handle->prepare (sql, 1);
    handle->step ();

    sql = "INSERT INTO rpm VALUES(1, @n, @e, @v, @r, @a, @ct, @cd)";
    handle->prepare (sql, "name", 0, "1.1", "rel", "noarch", "sha0", "ABC0");
    handle->step ();
}

void
HandleTest::testSelect ()
{
    const char *sql = "SELECT name, epoch, version FROM rpm WHERE name=?";
    handle->prepare (sql, "name");
    handle->step ();
    CPPUNIT_ASSERT_EQUAL (handle->getString (0), std::string ("name"));
    CPPUNIT_ASSERT_EQUAL (handle->getInt (1), 0);
    CPPUNIT_ASSERT_EQUAL (handle->getString (2), std::string ("1.1"));
}
