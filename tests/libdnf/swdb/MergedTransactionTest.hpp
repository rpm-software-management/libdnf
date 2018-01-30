#ifndef LIBDNF_SWDB_MERGEDTRANSACTION_TEST_HPP
#define LIBDNF_SWDB_MERGEDTRANSACTION_TEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "libdnf/utils/sqlite3/sqlite3.hpp"

class MergedTransactionTest : public CppUnit::TestCase {
    CPPUNIT_TEST_SUITE(MergedTransactionTest);
    CPPUNIT_TEST(testMerge);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testMerge();

private:
    std::shared_ptr< SQLite3 > conn;
};

#endif // LIBDNF_SWDB_MERGEDTRANSACTION_TEST_HPP
