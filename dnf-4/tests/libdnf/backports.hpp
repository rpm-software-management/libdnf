#ifndef LIBDNF_TESTS_BACKPORTS_HPP
#define LIBDNF_TESTS_BACKPORTS_HPP

#include <ostream>
#include <type_traits>

// fix CPPUNIT_ASSERT_EQUAL for enums in CppUnit < 1.14
template<typename E, typename = typename std::enable_if<std::is_enum<E>::value>::type>
std::ostream & operator <<(std::ostream & out, E e) {
    return out << static_cast<typename std::underlying_type<E>::type>(e);
}

#endif // LIBDNF_TESTS_BACKPORTS_HPP
