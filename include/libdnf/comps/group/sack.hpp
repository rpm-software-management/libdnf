#ifndef LIBDNF_COMPS_GROUP_SACK_HPP
#define LIBDNF_COMPS_GROUP_SACK_HPP


#include "group.hpp"
#include "query.hpp"

#include "libdnf/utils/sack/sack.hpp"


namespace libdnf::comps {


class GroupSack : public libdnf::utils::sack::Sack<Group, GroupQuery> {};


}  // namespace libdnf::comps


#endif
