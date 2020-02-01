#ifndef LIBDNF_COMPS_ENVIRONMENT_ENVIRONMENTSACK_HPP
#define LIBDNF_COMPS_ENVIRONMENT_ENVIRONMENTSACK_HPP


#include "../../utils/sack/Sack.hpp"
#include "Environment.hpp"
#include "EnvironmentQuery.hpp"


namespace libdnf::comps {


class EnvironmentSack : public libdnf::utils::sack::Sack<Environment, EnvironmentQuery> {
};


}  // namespace libdnf::comps


#endif
