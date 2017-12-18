#ifndef LIBDNF_SWDB_RPMITEM_TEST_HPP
#define LIBDNF_SWDB_RPMITEM_TEST_HPP

#include "libdnf/swdb/transformer.hpp"
#include "libdnf/utils/sqlite3/sqlite3.hpp"
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

class TransformerMock : protected Transformer {
public:
    TransformerMock();
    using Transformer::transformRPMItems;
    using Transformer::transformTrans;
    using Transformer::transformTransWith;
    using Transformer::transformOutput;
    using Transformer::Exception;
};

class TransformerTest : public CppUnit::TestCase {
    CPPUNIT_TEST_SUITE(TransformerTest);
    // CPPUNIT_TEST(testTransformRPMItems);
    CPPUNIT_TEST(testTransformTrans);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testTransformRPMItems();
    void testTransformTrans();

protected:
    TransformerMock transformer;
    std::shared_ptr<SQLite3> swdb;
    std::shared_ptr<SQLite3> history;
};

#endif // LIBDNF_SWDB_RPMITEM_TEST_HPP
