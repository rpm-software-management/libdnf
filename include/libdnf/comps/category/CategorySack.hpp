#ifndef LIBDNF_COMPS_CATEGORY_CATEGORYSACK_HPP
#define LIBDNF_COMPS_CATEGORY_CATEGORYSACK_HPP


#include "../../utils/sack/Sack.hpp"
#include "Category.hpp"
#include "CategoryQuery.hpp"


namespace libdnf::comps {


class CategorySack : public libdnf::utils::sack::Sack<Category, CategoryQuery> {
};


}  // namespace libdnf::comps


#endif
