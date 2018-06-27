#ifndef LIBDNF_RPMPACKAGEWITHCOMMANDLINEPACKAGETEST_HPP
#define LIBDNF_RPMPACKAGEWITHCOMMANDLINEPACKAGETEST_HPP

#include "RpmPackageTest.hpp"

class RpmPackageWithCommandlinePackageTest : public RpmPackageTest
{
    CPPUNIT_TEST_SUITE(RpmPackageWithCommandlinePackageTest);
        CPPUNIT_TEST(testGetFilesCommandLine);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
};


#endif //LIBDNF_RPMPACKAGEWITHCOMMANDLINEPACKAGETEST_HPP
