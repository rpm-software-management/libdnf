#include "HandleTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION (HandleTest);

const char *path = "/tmp/swdb_test.db";

void
HandleTest::setUp ()
{
    handle = Handle::getInstance (path);
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
