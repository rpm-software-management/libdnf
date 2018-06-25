#include "RpmPackageWithCommandlinePackageTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(RpmPackageWithCommandlinePackageTest);

void RpmPackageWithCommandlinePackageTest::setUp()
{
    sack.addComandline();
}
