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
        CPPUNIT_TEST(testDisableModules);
        CPPUNIT_TEST(testDisabledModules);
        CPPUNIT_TEST(testDisableModuleSpecs);
        CPPUNIT_TEST(testEnableModuleSpecs);
        CPPUNIT_TEST(testEnableModules);
        CPPUNIT_TEST(testRollback);
        CPPUNIT_TEST(testInstallProfile);
        CPPUNIT_TEST(testRemoveProfile);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testEnabledModules();
    void testDisableModules();
    void testDisabledModules();
    void testDisableModuleSpecs();
    void testEnableModuleSpecs();
    void testEnableModules();
    void testRollback();
    void testInstallProfile();
    void testRemoveProfile();

private:
    DnfContext *context;
    ModulePackageContainer *modules;
};

#endif /* LIBDNF_MODULEPACKAGECONTAINERTEST_HPP */
