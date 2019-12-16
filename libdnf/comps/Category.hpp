#pragma once


#include <vector>
#include <string>

#include "Group.hpp"


namespace libdnf::comps {


/// class Category
///
class Category {
    std::string get_id() const;

    std::string get_name() const;

    std::string get_description() const;

    std::string get_translated_name() const;

    std::string get_translated_description() const;

    std::vector<Group> get_groups() const;

    std::vector<Group> get_groups(bool mandatory_groups, bool optional_groups) const;
};


}  // namespace libdnf::comps
