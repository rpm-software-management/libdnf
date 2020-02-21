#ifndef LIBDNF_COMPS_CATEGORY_CATEGORY_HPP
#define LIBDNF_COMPS_CATEGORY_CATEGORY_HPP


#include "libdnf/comps/group/group.hpp"

#include <string>
#include <vector>


namespace libdnf::comps {


class Category {
public:
    std::string get_id() const;

    std::string get_name() const;

    std::string get_description() const;

    std::string get_translated_name() const;

    std::string get_translated_description() const;

    std::vector<Group> get_groups() const;

    std::vector<Group> get_groups(bool mandatory_groups, bool optional_groups) const;
};


}  // namespace libdnf::comps


#endif
