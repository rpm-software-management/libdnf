#ifndef LIBDNF_MODULEPACKAGETEST_HPP
#define LIBDNF_MODULEPACKAGETEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "libdnf/module/ModulePackageContainer.hpp"
#include "libdnf/module/ModulePackage.hpp"
#include "libdnf/dnf-context.hpp"

class ModulePackageTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(ModulePackageTest);
    CPPUNIT_TEST(testSimpleGetters);
    CPPUNIT_TEST(testGetArtifacts);
    CPPUNIT_TEST(testGetProfiles);
    CPPUNIT_TEST(testGetModuleDependencies);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testSimpleGetters();
    void testGetArtifacts();
    void testGetProfiles();
    void testGetModuleDependencies();

private:
    DnfContext *context;
    std::vector<libdnf::ModulePackage *> packages;
};

#endif /* LIBDNF_MODULEPACKAGETEST_HPP */
