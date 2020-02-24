#ifndef LIBDNF_SWDB_MIGRATION_TEST_HPP
#define LIBDNF_SWDB_MIGRATION_TEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "libdnf/utils/sqlite3/Sqlite3.hpp"

class MigrationTest : public CppUnit::TestCase {
    CPPUNIT_TEST_SUITE(MigrationTest);
    CPPUNIT_TEST(testVersionMissing);
    CPPUNIT_TEST(testVersionBeforeMigration);
    CPPUNIT_TEST(testVersionAfterMigration);
    CPPUNIT_TEST(testEmptyCommentAfterMigration);
    CPPUNIT_TEST(testNonEmptyCommentAfterMigration);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testVersionMissing();
    void testVersionBeforeMigration();
    void testVersionAfterMigration();
    void testEmptyCommentAfterMigration();
    void testNonEmptyCommentAfterMigration();

private:
    std::shared_ptr< SQLite3 > history;
};

#endif // LIBDNF_SWDB_MIGRATION_TEST_HPP
