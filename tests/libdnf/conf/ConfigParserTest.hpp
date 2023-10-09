#ifndef LIBDNF_CONFIGPARSERTEST_HPP
#define LIBDNF_CONFIGPARSERTEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include <libdnf/conf/ConfigParser.hpp>

class ConfigParserTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(ConfigParserTest);
        CPPUNIT_TEST(testConfigParserReleasever);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void testConfigParserReleasever();

};

#endif // LIBDNF_CONFIGPARSERTEST_HPP
