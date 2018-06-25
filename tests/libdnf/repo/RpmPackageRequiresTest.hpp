#ifndef LIBDNF_RPMPACKAGEREQUIRESTEST_HPP
#define LIBDNF_RPMPACKAGEREQUIRESTEST_HPP

#include "RpmPackageTest.hpp"

class RpmPackageRequiresTest : public RpmPackageTest
{
    CPPUNIT_TEST_SUITE(RpmPackageRequiresTest);
        CPPUNIT_TEST(testGetRequires);
        CPPUNIT_TEST(testChecksumFail);
        CPPUNIT_TEST(testInstalled);
        CPPUNIT_TEST(testTwoSacks);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
};


#endif //LIBDNF_RPMPACKAGEREQUIRESTEST_HPP
