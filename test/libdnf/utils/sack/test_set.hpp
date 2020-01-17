#pragma once


#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>


class SetTest : public CppUnit::TestCase {
    CPPUNIT_TEST_SUITE(SetTest);
    CPPUNIT_TEST(test_update);
    CPPUNIT_TEST(test_difference);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void test_update();
    void test_difference();

private:
};
