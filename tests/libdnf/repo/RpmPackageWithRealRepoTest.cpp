#include "RpmPackageWithRealRepoTest.hpp"
#include "tests/libdnf/TestMacros.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(RpmPackageWithRealRepoTest);

void RpmPackageWithRealRepoTest::setUp()
{
    sack.setUpYumSack(YUM_REPO_NAME);
}
