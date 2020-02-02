#pragma once


#include "libdnf/utils/sack/Sack.hpp"
#include "RelatedObject.hpp"
#include "RelatedObjectQuery.hpp"


class RelatedObjectSack : public libdnf::utils::sack::Sack<RelatedObject, RelatedObjectQuery> {
public:
    using libdnf::utils::sack::Sack<RelatedObject, RelatedObjectQuery>::get_data;
};
