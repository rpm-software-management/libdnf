#ifndef LIBDNF_MODULARITYLABELTEST_HPP
#define LIBDNF_MODULARITYLABELTEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "libdnf/dnf-sack.h"
#include "libdnf/hy-types.h"

class ModularityLabelTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(ModularityLabelTest);
        CPPUNIT_TEST(testInstallLabelledPackage);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testInstallLabelledPackage();

private:
    DnfSack *sack = nullptr;
    HyRepo repo = nullptr;
    char *tmpdir = nullptr;
};

#endif /* LIBDNF_MODULARITYLABELTEST_HPP */
