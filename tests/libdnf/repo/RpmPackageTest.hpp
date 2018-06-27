#ifndef LIBDNF_RPMPACKAGETEST_HPP
#define LIBDNF_RPMPACKAGETEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "../TestSack.hpp"

#define TESTREPODATADIR TESTDATADIR "/hawkey/"

class RpmPackageTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(RpmPackageTest);
        CPPUNIT_TEST(testSummary);
        CPPUNIT_TEST(testIdentical);
        CPPUNIT_TEST(testVersions);
        CPPUNIT_TEST(testNoSourcerpm);
    CPPUNIT_TEST_SUITE_END();

public:
    RpmPackageTest();

    void setUp() override;
    void tearDown() override;

    void testSummary();
    void testIdentical();
    void testVersions();
    void testNoSourcerpm();
    void testSourcerpm();
    void testGetRequires();
    void testChecksumFail();
    void testChecksum();
    void testGetFiles();
    void testGetAdvisories();
    void testLookupNum();
    void testInstalled();
    void testPackager();
    void testPresto();
    void testGetFilesCommandLine();
    void testTwoSacks();

protected:
    TestSack sack;
};


#endif //LIBDNF_RPMPACKAGETEST_HPP
