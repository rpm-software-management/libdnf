#ifndef LIBDNF_SWDBHANDLETEST_HPP
#define LIBDNF_SWDBHANDLETEST_HPP

// FIXME cmake includes
#include "../../../libdnf/swdb/handle/handle.hpp"

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

class HandleTest : public CppUnit::TestCase
{
    CPPUNIT_TEST_SUITE (HandleTest);
    CPPUNIT_TEST (testPath);
    CPPUNIT_TEST_SUITE_END ();

  public:
    void setUp () override;
    void tearDown () override;

    void testPath ();

  private:
    Handle *handle;
};

#endif // LIBDNF_PACKAGETEST_HPP
