#include "ObjectSet.hpp"
#include "Object.hpp"

#include "test_set.hpp"

#include <limits>


CPPUNIT_TEST_SUITE_REGISTRATION(SetTest);


void SetTest::setUp() {
}


void SetTest::tearDown() {
}


void SetTest::test_update() {
    ObjectSet s1;
    ObjectSet s2;

    auto o1 = new Object();
    auto o2 = new Object();

    s2.add(o1);

    CPPUNIT_ASSERT(s1.size() == 0);
    CPPUNIT_ASSERT(s2.size() == 1);

    s1.update(s2);

    CPPUNIT_ASSERT(s1.size() == 1);
    CPPUNIT_ASSERT(s2.size() == 1);

    s2.add(o2);

    CPPUNIT_ASSERT(s1.size() == 1);
    CPPUNIT_ASSERT(s2.size() == 2);

    s1.update(s2);

    CPPUNIT_ASSERT(s1.size() == 2);
    CPPUNIT_ASSERT(s2.size() == 2);

    delete o1;
    delete o2;
}


void SetTest::test_difference() {
    ObjectSet s1;
    ObjectSet s2;

    auto o1 = new Object();
    auto o2 = new Object();

    s2.add(o1);
    s2.add(o2);

    CPPUNIT_ASSERT(s1.size() == 0);
    CPPUNIT_ASSERT(s2.size() == 2);

    s2.difference(s1);

    CPPUNIT_ASSERT(s1.size() == 0);
    CPPUNIT_ASSERT(s2.size() == 2);

    s2.add(o1);
    s2.add(o2);
    s1.difference(s2);

    CPPUNIT_ASSERT(s1.size() == 0);
    CPPUNIT_ASSERT(s2.size() == 2);

    delete o1;
    delete o2;
}
