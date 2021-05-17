#ifndef LIBDNF_ADVISORYTEST_HPP
#define LIBDNF_ADVISORYTEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include <libdnf/sack/advisory.hpp>
#include <libdnf/sack/advisorymodule.hpp>
#include <libdnf/sack/query.hpp>

class AdvisoryTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(AdvisoryTest);
        CPPUNIT_TEST(testGetName);
        CPPUNIT_TEST(testGetKind);
        CPPUNIT_TEST(testGetDescription);
        CPPUNIT_TEST(testGetRights);
        CPPUNIT_TEST(testGetSeverity);
        CPPUNIT_TEST(testGetTitle);
        CPPUNIT_TEST(testGetPackages);
        CPPUNIT_TEST(testGetApplicablePackagesModulesSetupNoneEnabled);
        CPPUNIT_TEST(testGetApplicablePackagesOneApplicableCollection);
        CPPUNIT_TEST(testGetApplicablePackagesMultipleApplicableCollections);
        CPPUNIT_TEST(testGetModules);
        CPPUNIT_TEST(testGetReferences);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testGetName();
    void testGetKind();
    void testGetDescription();
    void testGetRights();
    void testGetSeverity();
    void testGetTitle();
    void testGetPackages();
    void testGetApplicablePackagesModulesSetupNoneEnabled();
    void testGetApplicablePackagesOneApplicableCollection();
    void testGetApplicablePackagesMultipleApplicableCollections();
    void testGetModules();
    void testGetReferences();

private:
    DnfContext *context = nullptr;
    DnfSack *sack = nullptr;
    HyRepo repo = nullptr;
    libdnf::Advisory *advisory = nullptr;
    char* tmpdir = nullptr;
};


#endif //LIBDNF_ADVISORYTEST_HPP
