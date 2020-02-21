#ifndef LIBDNF_COMPS_ENVIRONMENT_SACK_HPP
#define LIBDNF_COMPS_ENVIRONMENT_SACK_HPP


#include "environment.hpp"
#include "environment_query.hpp"

#include "libdnf/utils/sack/sack.hpp"


namespace libdnf::comps {


class EnvironmentSack : public libdnf::utils::sack::Sack<Environment, EnvironmentQuery> {};


}  // namespace libdnf::comps


#endif
