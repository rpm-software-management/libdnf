#ifndef LIBDNF_CONFIGREPOTEST_HPP
#define LIBDNF_CONFIGREPOTEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

class ConfigRepoTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(ConfigRepoTest);
        CPPUNIT_TEST(testAutoImportLocalKeysDefault);
        CPPUNIT_TEST(testAutoImportLocalKeysInheritsFromMain);
        CPPUNIT_TEST(testAutoImportLocalKeysParsedFromConfig);
    CPPUNIT_TEST_SUITE_END();

public:
    void testAutoImportLocalKeysDefault();
    void testAutoImportLocalKeysInheritsFromMain();
    void testAutoImportLocalKeysParsedFromConfig();
};

#endif // LIBDNF_CONFIGREPOTEST_HPP
