#ifndef LIBDNF_DNFPACKAGETEST_HPP
#define LIBDNF_DNFPACKAGETEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include <libdnf/sack/query.hpp>
#include <libdnf/sack/advisory.hpp>

class DnfPackageTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(DnfPackageTest);
        CPPUNIT_TEST(testDnfPackageGetAdvisories);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testDnfPackageGetAdvisories();

private:
    DnfSack *sack = nullptr;
    HyRepo repo = nullptr;
    char* tmpdir = nullptr;
};


#endif //LIBDNF_DNFPACKAGETEST_HPP
