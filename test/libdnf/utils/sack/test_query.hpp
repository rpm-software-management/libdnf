#pragma once

#include "object_sack.hpp"
#include "related_object_sack.hpp"

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

class QueryTest : public CppUnit::TestCase {
    CPPUNIT_TEST_SUITE(QueryTest);

    CPPUNIT_TEST(test_filter_string_exact);

    CPPUNIT_TEST(test_filter_string_glob);
    CPPUNIT_TEST(test_filter_string_iglob);
    CPPUNIT_TEST(test_filter_string_regex);
    CPPUNIT_TEST(test_filter_string_iregex);

    CPPUNIT_TEST(test_filter_string_vector_exact);
    CPPUNIT_TEST(test_filter_string_vector_iexact);

    CPPUNIT_TEST(test_filter_int32_eq);
    CPPUNIT_TEST(test_filter_int32_lt);

    CPPUNIT_TEST(test_filter_related_object_string);

    CPPUNIT_TEST(test_match_installed);
    CPPUNIT_TEST(test_match_repoid);

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void test_filter_string_exact();
    void test_filter_string_glob();
    void test_filter_string_iglob();
    void test_filter_string_regex();
    void test_filter_string_iregex();

    void test_filter_string_vector_exact();
    void test_filter_string_vector_iexact();

    void test_filter_int32_eq();
    void test_filter_int32_lt();

    void test_filter_related_object_string();

    void test_match_installed();
    void test_match_repoid();

private:
    ObjectSack sack;
    RelatedObjectSack sack_related;
};
