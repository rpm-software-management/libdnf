#ifndef LIBDNF_CONFIGREPOTEST_HPP
#define LIBDNF_CONFIGREPOTEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include <libdnf/conf/ConfigMain.hpp>
#include <libdnf/conf/ConfigRepo.hpp>

class ConfigRepoTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(ConfigRepoTest);
        CPPUNIT_TEST(testRepoGpgcheckAutoImportKeysDefault);
        CPPUNIT_TEST(testRepoGpgcheckAutoImportKeysInheritsFromMain);
        CPPUNIT_TEST(testRepoGpgcheckAutoImportKeysParsedFromConfig);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void testRepoGpgcheckAutoImportKeysDefault();
    void testRepoGpgcheckAutoImportKeysInheritsFromMain();
    void testRepoGpgcheckAutoImportKeysParsedFromConfig();
};

#endif // LIBDNF_CONFIGREPOTEST_HPP
