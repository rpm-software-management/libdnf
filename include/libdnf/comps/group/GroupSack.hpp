#ifndef LIBDNF_COMPS_GROUP_GROUPSACK_HPP
#define LIBDNF_COMPS_GROUP_GROUPSACK_HPP


#include "../../utils/sack/Sack.hpp"
#include "Group.hpp"
#include "GroupQuery.hpp"


namespace libdnf::comps {


class GroupSack : public libdnf::utils::sack::Sack<Group, GroupQuery> {
};


}  // namespace libdnf::comps


#endif
