#ifndef LIBDNF_CPPUNITCUSTOMMACROS_HPP
#define LIBDNF_CPPUNITCUSTOMMACROS_HPP

#include <cppunit/extensions/HelperMacros.h>

#define CPPUNIT_ASSERT_CHAR_EQUAL(expected, actual) CPPUNIT_ASSERT(strcmp(expected, actual) == 0)
#define CPPUNIT_ASSERT_FAIL(assert) CPPUNIT_ASSERT(!(assert))

#endif //LIBDNF_CPPUNITCUSTOMMACROS_HPP
