#ifndef LIBDNF_COMPS_ENVIRONMENT_ENVIRONMENTSACK_HPP
#define LIBDNF_COMPS_ENVIRONMENT_ENVIRONMENTSACK_HPP


#include "../../utils/sack/Sack.hpp"
#include "Environment.hpp"


namespace libdnf::comps {


class EnvironmentSack : public libdnf::utils::sack::Sack<libdnf::comps::Environment> {
};


}  // namespace libdnf::comps


#endif
