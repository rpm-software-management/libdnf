#ifndef LIBDNF_COMPS_CATEGORY_SACK_HPP
#define LIBDNF_COMPS_CATEGORY_SACK_HPP


#include "category.hpp"
#include "query.hpp"

#include "libdnf/utils/sack/sack.hpp"


namespace libdnf::comps {


class CategorySack : public libdnf::utils::sack::Sack<Category, CategoryQuery> {};


}  // namespace libdnf::comps


#endif
