#ifndef LIBDNF_COMPS_CATEGORY_CATEGORYSACK_HPP
#define LIBDNF_COMPS_CATEGORY_CATEGORYSACK_HPP


#include "../../utils/sack/Sack.hpp"
#include "Category.hpp"


namespace libdnf::comps {


class CategorySack : public libdnf::utils::sack::Sack<Category> {
};


}  // namespace libdnf::comps


#endif
