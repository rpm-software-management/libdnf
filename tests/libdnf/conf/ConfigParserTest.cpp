#include "ConfigParserTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(ConfigParserTest);

void ConfigParserTest::setUp()
{}

void ConfigParserTest::testConfigParserReleasever()
{
    {
        // Test $releasever_major, $releasever_minor
        std::map<std::string, std::string> substitutions = {
            {"releasever", "1.23"},
        };

        std::string text = "major: $releasever_major, minor: $releasever_minor";
        libdnf::ConfigParser::substitute(text, substitutions);
        CPPUNIT_ASSERT(text == "major: 1, minor: 23");

        text = "full releasever: $releasever";
        libdnf::ConfigParser::substitute(text, substitutions);
        CPPUNIT_ASSERT(text == "full releasever: 1.23");
    }
    {
        // Test with empty $releasever, should set empty $releasever_major, $releasever_minor
        std::map<std::string, std::string> substitutions = {
            {"releasever", ""},
        };
        std::string text = "major: $releasever_major, minor: $releasever_minor";
        libdnf::ConfigParser::substitute(text, substitutions);
        CPPUNIT_ASSERT(text == "major: , minor: ");
    }
}
