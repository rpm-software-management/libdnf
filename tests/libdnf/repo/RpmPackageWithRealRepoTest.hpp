#ifndef LIBDNF_RPMPACKAGEWITHREALREPOTEST_HPP
#define LIBDNF_RPMPACKAGEWITHREALREPOTEST_HPP

#include "RpmPackageTest.hpp"

class RpmPackageWithRealRepoTest : public RpmPackageTest
{
    CPPUNIT_TEST_SUITE(RpmPackageWithRealRepoTest);
        CPPUNIT_TEST(testChecksum);
        CPPUNIT_TEST(testGetFiles);
        CPPUNIT_TEST(testGetAdvisories);
        CPPUNIT_TEST(testLookupNum);
        CPPUNIT_TEST(testPackager);
        CPPUNIT_TEST(testSourcerpm);
        CPPUNIT_TEST(testPresto);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
};


#endif //LIBDNF_RPMPACKAGEWITHREALREPOTEST_HPP
