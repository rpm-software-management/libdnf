#ifndef LIBDNF_KEYAUTOIMPORTTEST_HPP
#define LIBDNF_KEYAUTOIMPORTTEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include <string>

class KeyAutoImportTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(KeyAutoImportTest);
        CPPUNIT_TEST(testOptionAndAssumeno);
        CPPUNIT_TEST(testKeysInsideDirectory);
        CPPUNIT_TEST(testUrlSpellings);
        CPPUNIT_TEST(testKeysOutsideDirectory);
        CPPUNIT_TEST(testKeysDirsList);
        CPPUNIT_TEST(testRepoMethod);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testOptionAndAssumeno();
    void testKeysInsideDirectory();
    void testUrlSpellings();
    void testKeysOutsideDirectory();
    void testKeysDirsList();
    void testRepoMethod();

private:
    char * tmpdir{nullptr};
    std::string keysDir;
};

#endif // LIBDNF_KEYAUTOIMPORTTEST_HPP
