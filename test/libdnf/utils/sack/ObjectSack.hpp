#pragma once


#include "libdnf/utils/sack/Sack.hpp"
#include "Object.hpp"
#include "ObjectQuery.hpp"


class ObjectSack : public libdnf::utils::sack::Sack<Object> {
public:
    using libdnf::utils::sack::Sack<Object>::get_data;
};
