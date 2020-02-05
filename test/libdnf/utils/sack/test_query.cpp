#include "test_query.hpp"

#include "libdnf/utils/sack/QueryCmp.hpp"
#include "RelatedObject.hpp"
#include "ObjectQuery.hpp"
#include "Object.hpp"

#include <limits>


using QueryCmp = libdnf::utils::sack::QueryCmp;


CPPUNIT_TEST_SUITE_REGISTRATION(QueryTest);


void QueryTest::setUp() {
    Object o1;
    o1.string = "foo";
    o1.int32 = 10;

    Object o2;
    o2.string = "bar";
    o2.int32 = 20;

    RelatedObject ro1;
    ro1.id = "aaa";
    ro1.value = 100;
    o1.related_objects.push_back(ro1.id);
    o2.related_objects.push_back(ro1.id);

    RelatedObject ro2;
    ro2.id = "bbb";
    ro2.value = 200;
    o1.related_objects.push_back(ro2.id);

    RelatedObject ro3;
    ro3.id = "ccc";
    ro3.value = 100;
    o2.related_objects.push_back(ro3.id);

    sack_related.get_data().push_back(std::move(ro1));
    sack_related.get_data().push_back(std::move(ro2));
    sack_related.get_data().push_back(std::move(ro3));
    sack.get_data().push_back(std::move(o1));
    sack.get_data().push_back(std::move(o2));
}


void QueryTest::tearDown() {
}


void QueryTest::test_filter_string_exact() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q.filter(ObjectQuery::get_string, QueryCmp::EXACT, "no-match");
    CPPUNIT_ASSERT(q.size() == 0);

    // ObjectQuery::get_string field == "foo"
    q = sack.new_query();
    q.filter(ObjectQuery::get_string, QueryCmp::EXACT, "foo");
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_string_glob() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q.filter(ObjectQuery::get_string, QueryCmp::GLOB, "no-match");
    CPPUNIT_ASSERT(q.size() == 0);

    // ObjectQuery::get_string field == "fo.*"
    q = sack.new_query();
    q.filter(ObjectQuery::get_string, QueryCmp::GLOB, "fo*");
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_string_iglob() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q.filter(ObjectQuery::get_string, QueryCmp::IGLOB, "no-match");
    CPPUNIT_ASSERT(q.size() == 0);

    // ObjectQuery::get_string field == "fo.*"
    q = sack.new_query();
    q.filter(ObjectQuery::get_string, QueryCmp::IGLOB, "FO*");
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_string_regex() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q.filter(ObjectQuery::get_string, QueryCmp::REGEX, "no-match");
    CPPUNIT_ASSERT(q.size() == 0);

    // ObjectQuery::get_string field == "fo.*"
    q = sack.new_query();
    q.filter(ObjectQuery::get_string, QueryCmp::REGEX, "fo.*");
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_string_iregex() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q.filter(ObjectQuery::get_string, QueryCmp::IREGEX, "no-match");
    CPPUNIT_ASSERT(q.size() == 0);

    // ObjectQuery::get_string field == "fo.*"
    q = sack.new_query();
    q.filter(ObjectQuery::get_string, QueryCmp::IREGEX, "FO.*");
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_string_vector_exact() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q.filter(ObjectQuery::get_string, QueryCmp::EXACT, std::vector<std::string>({"no-match"}));
    CPPUNIT_ASSERT(q.size() == 0);

    // ObjectQuery::get_string field in ["no-match", "foo"]
    q = sack.new_query();
    q.filter(ObjectQuery::get_string, QueryCmp::EXACT, std::vector<std::string>({"no-match", "foo"}));
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_string_vector_iexact() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q.filter(ObjectQuery::get_string, QueryCmp::IEXACT, std::vector<std::string>({"no-match"}));
    CPPUNIT_ASSERT(q.size() == 0);

    // ObjectQuery::get_string field in ["no-match", "fOo"]
    q = sack.new_query();
    q.filter(ObjectQuery::get_string, QueryCmp::IEXACT, std::vector<std::string>({"no-match", "fOo"}));
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_int32_eq() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q = sack.new_query();
    q.filter(ObjectQuery::get_int32, QueryCmp::EQ, 15);
    CPPUNIT_ASSERT(q.size() == 0);

    // "int32" field == 10
    q = sack.new_query();
    q.filter(ObjectQuery::get_int32, QueryCmp::EQ, 10);
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_int32_lt() {
    // check if initial set has expected size
    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // filter doesn't match anything
    q = sack.new_query();
    q.filter(ObjectQuery::get_int32, QueryCmp::LT, 10);
    CPPUNIT_ASSERT(q.size() == 0);

    // "int32" field < 11
    q = sack.new_query();
    q.filter(ObjectQuery::get_int32, QueryCmp::LT, 11);
    CPPUNIT_ASSERT(q.size() == 1);
}


void QueryTest::test_filter_related_object_string() {
    auto q_related = sack_related.new_query();
    CPPUNIT_ASSERT(q_related.size() == 3);

    // there's only 1 RelatedObject with id == "aaa"
    q_related.filter(RelatedObjectQuery::get_id, QueryCmp::EXACT, "aaa");
    CPPUNIT_ASSERT(q_related.size() == 1);

    auto q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // there are 2 Objects containing "aaa" in related_objects vector
    q.filter(ObjectQuery::get_related_object, QueryCmp::EQ, q_related);
    CPPUNIT_ASSERT(q.size() == 2);

    // reset q_related
    q_related = sack_related.new_query();
    CPPUNIT_ASSERT(q_related.size() == 3);

    // there's only 1 RelatedObject with id == "bbb"
    q_related.filter(RelatedObjectQuery::get_id, QueryCmp::EXACT, "bbb");
    CPPUNIT_ASSERT(q_related.size() == 1);

    q = sack.new_query();
    CPPUNIT_ASSERT(q.size() == 2);

    // there's 1 Object containing "bbb" in related_objects vector
    q.filter(ObjectQuery::get_related_object, QueryCmp::EQ, q_related);
    CPPUNIT_ASSERT(q.size() == 1);
}
