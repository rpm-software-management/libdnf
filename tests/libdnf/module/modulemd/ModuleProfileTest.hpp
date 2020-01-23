#ifndef LIBDNF_MODULEPROFILETEST_HPP
#define LIBDNF_MODULEPROFILETEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "libdnf/module/modulemd/ModuleProfile.hpp"
#include "libdnf/module/ModulePackageContainer.hpp"

class ModuleProfileTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(ModuleProfileTest);
    CPPUNIT_TEST(testGetName);
    CPPUNIT_TEST(testGetDescription);
    CPPUNIT_TEST(testGetContent);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testGetName();
    void testGetDescription();
    void testGetContent();

private:
    DnfContext *context;
    std::vector<libdnf::ModuleProfile> profiles;
};

#endif /* LIBDNF_MODULEPROFILETEST_HPP */
