#include "test_query.hpp"

#include "libdnf/utils/sack/QueryCmp.hpp"
#include "ObjectQuery.hpp"
#include "Object.hpp"

#include <limits>


using QueryCmp = libdnf::utils::sack::QueryCmp;


CPPUNIT_TEST_SUITE_REGISTRATION(QueryTest);


void QueryTest::setUp() {
    auto o1 = new Object();
    o1->string = "foo";
    o1->int32 = 10;
    sack.get_data().add(o1);

    auto o2 = new Object();
    o2->string = "bar";
    o2->int32 = 20;
    sack.get_data().add(o2);
}


void QueryTest::tearDown() {
}


void QueryTest::test_filter_string_exact() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q.filter(ObjectQuery::Key::string, QueryCmp::EXACT, "no-match");
    CPPUNIT_ASSERT(q.size() == 0);

    // ObjectQuery::Key::string field == "foo"
    q = sack.new_query();
    q.filter(ObjectQuery::Key::string, QueryCmp::EXACT, "foo");
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_string_glob() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q.filter(ObjectQuery::Key::string, QueryCmp::GLOB, "no-match");
    CPPUNIT_ASSERT(q.size() == 0);

    // ObjectQuery::Key::string field == "fo.*"
    q = sack.new_query();
    q.filter(ObjectQuery::Key::string, QueryCmp::GLOB, "fo*");
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_string_iglob() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q.filter(ObjectQuery::Key::string, QueryCmp::IGLOB, "no-match");
    CPPUNIT_ASSERT(q.size() == 0);

    // ObjectQuery::Key::string field == "fo.*"
    q = sack.new_query();
    q.filter(ObjectQuery::Key::string, QueryCmp::IGLOB, "FO*");
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_string_regex() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q.filter(ObjectQuery::Key::string, QueryCmp::REGEX, "no-match");
    CPPUNIT_ASSERT(q.size() == 0);

    // ObjectQuery::Key::string field == "fo.*"
    q = sack.new_query();
    q.filter(ObjectQuery::Key::string, QueryCmp::REGEX, "fo.*");
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_string_iregex() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q.filter(ObjectQuery::Key::string, QueryCmp::IREGEX, "no-match");
    CPPUNIT_ASSERT(q.size() == 0);

    // ObjectQuery::Key::string field == "fo.*"
    q = sack.new_query();
    q.filter(ObjectQuery::Key::string, QueryCmp::IREGEX, "FO.*");
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_string_vector_exact() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q.filter(ObjectQuery::Key::string, QueryCmp::EXACT, std::vector<std::string>({"no-match"}));
    CPPUNIT_ASSERT(q.size() == 0);

    // ObjectQuery::Key::string field in ["no-match", "foo"]
    q = sack.new_query();
    q.filter(ObjectQuery::Key::string, QueryCmp::EXACT, std::vector<std::string>({"no-match", "foo"}));
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_string_vector_iexact() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q.filter(ObjectQuery::Key::string, QueryCmp::IEXACT, std::vector<std::string>({"no-match"}));
    CPPUNIT_ASSERT(q.size() == 0);

    // ObjectQuery::Key::string field in ["no-match", "fOo"]
    q = sack.new_query();
    q.filter(ObjectQuery::Key::string, QueryCmp::IEXACT, std::vector<std::string>({"no-match", "fOo"}));
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_int32_eq() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q = sack.new_query();
    q.filter(ObjectQuery::Key::int32, QueryCmp::EQ, 15);
    CPPUNIT_ASSERT(q.size() == 0);

    // "int32" field == 10
    q = sack.new_query();
    q.filter(ObjectQuery::Key::int32, QueryCmp::EQ, 10);
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_int32_lt() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q = sack.new_query();
    q.filter(ObjectQuery::Key::int32, QueryCmp::LT, 10);
    CPPUNIT_ASSERT(q.size() == 0);

    // "int32" field < 11
    q = sack.new_query();
    q.filter(ObjectQuery::Key::int32, QueryCmp::LT, 11);
    CPPUNIT_ASSERT(q.size() == 1);
}
