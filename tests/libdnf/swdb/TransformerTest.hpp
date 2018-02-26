#ifndef LIBDNF_SWDB_RPMITEM_TEST_HPP
#define LIBDNF_SWDB_RPMITEM_TEST_HPP

#include "libdnf/swdb/Transformer.hpp"
#include "libdnf/utils/sqlite3/Sqlite3.hpp"
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

class TransformerMock : protected Transformer {
public:
    TransformerMock();
    using Transformer::Exception;
    using Transformer::processGroupPersistor;
    using Transformer::transformTrans;
};

class TransformerTest : public CppUnit::TestCase {
    CPPUNIT_TEST_SUITE(TransformerTest);
    CPPUNIT_TEST(testGroupTransformation);
    CPPUNIT_TEST(testTransformTrans);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testTransformTrans();
    void testGroupTransformation();

protected:
    TransformerMock transformer;
    std::shared_ptr< SQLite3 > swdb;
    std::shared_ptr< SQLite3 > history;
};

#endif // LIBDNF_SWDB_RPMITEM_TEST_HPP
