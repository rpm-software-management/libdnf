#include "number.hpp"

#include "libdnf/utils/number.hpp"

#include <limits>


CPPUNIT_TEST_SUITE_REGISTRATION(UtilsNumberTest);


void UtilsNumberTest::setUp() {
}


void UtilsNumberTest::tearDown() {
}


void UtilsNumberTest::test_random_int32() {
    auto min_int32 = std::numeric_limits<int32_t>::min();
    auto rnd_min_int32 = libdnf::utils::number::random_int32(min_int32, min_int32);
    CPPUNIT_ASSERT_EQUAL(min_int32, rnd_min_int32);

    auto max_int32 = std::numeric_limits<int32_t>::max();
    auto rnd_max_int32 = libdnf::utils::number::random_int32(max_int32, max_int32);
    CPPUNIT_ASSERT_EQUAL(max_int32, rnd_max_int32);
}
