#pragma once


#include "object.hpp"
#include "object_query.hpp"

#include "libdnf/utils/sack/sack.hpp"


class ObjectSack : public libdnf::utils::sack::Sack<Object, ObjectQuery> {
public:
    using libdnf::utils::sack::Sack<Object, ObjectQuery>::get_data;
};
