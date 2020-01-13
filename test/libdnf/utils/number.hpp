#ifndef TEST_LIBDNF_UTILS_NUMBER_HPP
#define TEST_LIBDNF_UTILS_NUMBER_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

class UtilsNumberTest : public CppUnit::TestCase {
    CPPUNIT_TEST_SUITE(UtilsNumberTest);
    CPPUNIT_TEST(test_random_int32);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void test_random_int32();

private:
};

#endif // TEST_LIBDNF_UTILS_NUMBER_HPP
