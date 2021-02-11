#ifndef LIBDNF_MODULEPACKAGECONTAINERTEST_HPP
#define LIBDNF_MODULEPACKAGECONTAINERTEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "libdnf/module/ModulePackageContainer.hpp"
#include "libdnf/dnf-context.hpp"

class ModulePackageContainerTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(ModulePackageContainerTest);
        CPPUNIT_TEST(testEnabledModules);
        CPPUNIT_TEST(testDisableEnableModules);
        CPPUNIT_TEST(testRollback);
        CPPUNIT_TEST(testInstallRemoveProfile);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testEnabledModules();
    void testDisableEnableModules();
    void testRollback();
    void testInstallRemoveProfile();

private:
    DnfContext *context;
    libdnf::ModulePackageContainer *modules;
    char* tmpdir;
};

#endif /* LIBDNF_MODULEPACKAGECONTAINERTEST_HPP */
