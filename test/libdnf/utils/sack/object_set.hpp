#pragma once


#include "object.hpp"

#include "libdnf/utils/sack/set.hpp"


class ObjectSet : public libdnf::utils::sack::Set<Object *> {};
