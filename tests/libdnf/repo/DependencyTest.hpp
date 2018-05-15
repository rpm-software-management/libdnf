#ifndef LIBDNF_SOLVABLEDEPENDENCYTEST_HPP
#define LIBDNF_SOLVABLEDEPENDENCYTEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include "libdnf/repo/solvable/Dependency.hpp"

class DependencyTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(DependencyTest);
        CPPUNIT_TEST(testName);
        CPPUNIT_TEST(testVersion);
        CPPUNIT_TEST(testParse);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;

    void testName();
    void testVersion();
    void testParse();

private:
    std::unique_ptr<libdnf::Dependency> dependency;
    DnfSack *sack;
};


#endif //LIBDNF_SOLVABLEDEPENDENCYTEST_HPP
