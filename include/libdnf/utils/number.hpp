#ifndef LIBDNF_UTILS_NUMBER_HPP
#define LIBDNF_UTILS_NUMBER_HPP


#include <cstdint>


namespace libdnf::utils::number {


/// @replaces libdnf:utils.hpp:function:random(const int min, const int max)
int32_t random_int32(int32_t min, int32_t max);


}  // namespace libdnf::utils::number


#endif
