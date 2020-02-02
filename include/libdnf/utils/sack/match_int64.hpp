#ifndef LIBDNF_UTILS_SACK_MATCH_INT64_HPP
#define LIBDNF_UTILS_SACK_MATCH_INT64_HPP


#include "QueryCmp.hpp"

#include <cstdint>
#include <vector>


namespace libdnf::utils::sack {


bool match_int64(int64_t value, QueryCmp cmp, int64_t pattern);
bool match_int64(int64_t value, QueryCmp cmp, const std::vector<int64_t> & patterns);
bool match_int64(const std::vector<int64_t> & values, QueryCmp cmp, int64_t pattern);
bool match_int64(const std::vector<int64_t> & values, QueryCmp cmp, const std::vector<int64_t> & patterns);


}  // namespace libdnf::utils::sack

#endif
