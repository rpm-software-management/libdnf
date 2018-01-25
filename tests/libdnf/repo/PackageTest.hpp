#ifndef LIBDNF_SOLVABLESTEST_HPP
#define LIBDNF_SOLVABLESTEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include "PackageInstantiable.hpp"

class PackageTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(PackageTest);
        CPPUNIT_TEST(testName);
        CPPUNIT_TEST(testVersion);
        CPPUNIT_TEST(testArch);
        CPPUNIT_TEST(testIsInRepo);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testName();
    void testVersion();
    void testArch();
    void testIsInRepo();

private:
    std::unique_ptr<PackageInstantiable> package;
    DnfSack *sack;
    HyRepo repo;
};


#endif //LIBDNF_SOLVABLESTEST_HPP
