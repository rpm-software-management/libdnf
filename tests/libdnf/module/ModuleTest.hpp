#ifndef LIBDNF_MODULETEST_HPP
#define LIBDNF_MODULETEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "libdnf/dnf-module.h"

class ModuleTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(ModuleTest);
        CPPUNIT_TEST(testDummy);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testDummy();

private:
};

#endif //LIBDNF_MODULETEST_HPP
