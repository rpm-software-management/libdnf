#ifndef LIBDNF_CONTEXTTEST_HPP
#define LIBDNF_CONTEXTTEST_HPP

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include "libdnf/dnf-context.hpp"
#include "libdnf/module/modulemd/ModuleMetadata.hpp"

class ContextTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE(ContextTest);
        CPPUNIT_TEST(testLoadModules);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;

    void testLoadModules();

private:
    DnfContext *context;
    void sackHas(DnfSack *sack, const ModuleMetadata & module) const;
    void sackHasNot(DnfSack *sack, const ModuleMetadata & module) const;
};

#endif //LIBDNF_CONTEXTTEST_HPP
