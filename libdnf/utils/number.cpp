#include "libdnf/utils/number.hpp"

#include <random>


namespace libdnf::utils::number {


int32_t random_int32(int32_t min, int32_t max) {
    std::random_device rd;
    std::default_random_engine gen(rd());
    std::uniform_int_distribution<int32_t> dist(min, max);
    return dist(gen);
}


}  // namespace libdnf::utils::number
