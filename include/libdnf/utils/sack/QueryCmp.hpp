#ifndef LIBDNF_UTILS_SACK_QUERYCMP_HPP
#define LIBDNF_UTILS_SACK_QUERYCMP_HPP


namespace libdnf::utils::sack {


// https://docs.djangoproject.com/en/3.0/ref/models/querysets/#field-lookups
enum class QueryCmp : int {
    // GENERIC
    EXACT = 1,   // exact match
    IEXACT = 2,  // exact match; case-insentsitive exact match for strings
    ISNULL = 3,  // value is NULL/None

    // NUMBERS
    EQ = 10,   // equal; identical meaning to EXACT
    GT = 11,   // greater than
    GTE = 12,  // greater than or equal to
    LT = 13,   // less than
    LTE = 14,  // less than or equal to
    // RANGE = 15,  // range test (inclusive)
    // NE = ???  -  not equal is not implemented

    // STRINGS
    CONTAINS = 20,     // case-sensitive containment test
    ICONTAINS = 21,    // case-insensitive containment test
    STARTSWITH = 22,   // case-sensitive starts-with
    ISTARTSWITH = 23,  // case-insensitive starts-with
    ENDSWITH = 24,     // case-sensitive ends-with
    IENDSWITH = 25,    // case-insensitive ends-with
    REGEX = 26,        // case-sensitive regular expression match.
    IREGEX = 27,       // case-insensitive regular expression match.
    GLOB = 28,         // case-sensitive glob match
    IGLOB = 29,        // case-insensitive glob match
};


}  // namespace libdnf::utils::sack


#endif
