#ifndef LIBDNF_RELATIONALDEPENDENCYCONTAINERTEST_HPP
#define LIBDNF_RELATIONALDEPENDENCYCONTAINERTEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "libdnf/repo/solvable/DependencyContainer.hpp"

class DependencyContainerTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(DependencyContainerTest);
        CPPUNIT_TEST(testAdd);
        CPPUNIT_TEST(testExtend);
        CPPUNIT_TEST(testGet);
        CPPUNIT_TEST(testCount);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;

    void testAdd();
    void testExtend();
    void testGet();
    void testCount();

private:
    std::unique_ptr<libdnf::DependencyContainer> container;
    DnfSack *sack;
};


#endif //LIBDNF_RELATIONALDEPENDENCYCONTAINERTEST_HPP
