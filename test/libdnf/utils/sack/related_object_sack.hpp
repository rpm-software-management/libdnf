#pragma once


#include "related_object.hpp"
#include "related_object_query.hpp"

#include "libdnf/utils/sack/sack.hpp"


class RelatedObjectSack : public libdnf::utils::sack::Sack<RelatedObject, RelatedObjectQuery> {
public:
    using libdnf::utils::sack::Sack<RelatedObject, RelatedObjectQuery>::get_data;
};
