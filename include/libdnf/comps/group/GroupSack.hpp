#ifndef LIBDNF_COMPS_GROUP_GROUPSACK_HPP
#define LIBDNF_COMPS_GROUP_GROUPSACK_HPP


#include "../../utils/sack/Sack.hpp"
#include "Group.hpp"


namespace libdnf::comps {


class GroupSack : public libdnf::utils::sack::Sack<Group> {
};


}  // namespace libdnf::comps


#endif
