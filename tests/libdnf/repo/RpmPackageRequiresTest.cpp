#include "RpmPackageRequiresTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(RpmPackageRequiresTest);

void RpmPackageRequiresTest::setUp()
{
    sack.loadRepos(HY_SYSTEM_REPO_NAME, "main", nullptr);
}
