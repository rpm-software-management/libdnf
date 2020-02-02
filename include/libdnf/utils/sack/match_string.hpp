#ifndef LIBDNF_UTILS_SACK_MATCH_STRING_HPP
#define LIBDNF_UTILS_SACK_MATCH_STRING_HPP


#include "QueryCmp.hpp"

#include <string>
#include <vector>


namespace libdnf::utils::sack {


bool match_string(const std::string & value, QueryCmp cmp, const std::string & pattern);
bool match_string(const std::string & value, QueryCmp cmp, const std::vector<std::string> & patterns);
bool match_string(const std::vector<std::string> & values, QueryCmp cmp, const std::string & pattern);
bool match_string(const std::vector<std::string> & values, QueryCmp cmp, const std::vector<std::string> & patterns);


}  // namespace libdnf::utils::sack


#endif
