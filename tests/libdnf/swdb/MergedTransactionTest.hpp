#ifndef LIBDNF_SWDB_MERGEDTRANSACTION_TEST_HPP
#define LIBDNF_SWDB_MERGEDTRANSACTION_TEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "libdnf/utils/sqlite3/sqlite3.hpp"

class MergedTransactionTest : public CppUnit::TestCase {
    CPPUNIT_TEST_SUITE(MergedTransactionTest);
    CPPUNIT_TEST(testMerge);
    CPPUNIT_TEST(testMergeEraseInstallReinstall);
    CPPUNIT_TEST(testMergeEraseInstallDowngrade);
    CPPUNIT_TEST(testMergeEraseInstallUpgrade);
    CPPUNIT_TEST(testMergeReinstallAny);
    CPPUNIT_TEST(testMergeInstallErase);
    CPPUNIT_TEST(testMergeInstallAlter);
    CPPUNIT_TEST(testMergeAlterReinstall);
    CPPUNIT_TEST(testMergeAlterErase);
    CPPUNIT_TEST(testMergeAlterAlter);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testMerge();
    void testMergeEraseInstallReinstall();
    void testMergeEraseInstallDowngrade();
    void testMergeEraseInstallUpgrade();
    void testMergeReinstallAny();
    void testMergeInstallErase();
    void testMergeInstallAlter();
    void testMergeAlterReinstall();
    void testMergeAlterErase();
    void testMergeAlterAlter();

private:
    std::shared_ptr< SQLite3 > conn;
};

#endif // LIBDNF_SWDB_MERGEDTRANSACTION_TEST_HPP
