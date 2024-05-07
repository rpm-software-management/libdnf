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
    {
        std::map<std::string, std::string> substitutions = {
            {"var1", "value123"},
            {"var2", "456"},
        };
        std::string text = "foo$var1-bar";
        libdnf::ConfigParser::substitute(text, substitutions);
        CPPUNIT_ASSERT(text == "foovalue123-bar");

        text = "${var1:+alternate}-${unset:-default}-${nn:+n${nn:-${nnn:}";
        libdnf::ConfigParser::substitute(text, substitutions);
        CPPUNIT_ASSERT(text == "alternate-default-${nn:+n${nn:-${nnn:}");

        text = "${unset:-${var1:+${var2:+$var2}}}";
        libdnf::ConfigParser::substitute(text, substitutions);
        CPPUNIT_ASSERT(text == "456");
    }
}
