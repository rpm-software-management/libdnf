#ifndef LIBDNF_QUERYTEST_HPP
#define LIBDNF_QUERYTEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <memory>

#include <libdnf/sack/query.hpp>
#include <libdnf/sack/advisory.hpp>

class QueryTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(QueryTest);
        CPPUNIT_TEST(testQueryGetAdvisoryPkgs);
        CPPUNIT_TEST(testQueryFilterAdvisory);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testQueryGetAdvisoryPkgs();
    void testQueryFilterAdvisory();

private:
    DnfSack *sack = nullptr;
    HyRepo repo = nullptr;
    char* tmpdir = nullptr;
};


#endif //LIBDNF_QUERYTEST_HPP
