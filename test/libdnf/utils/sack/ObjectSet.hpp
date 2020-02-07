#pragma once


#include "libdnf/utils/sack/Set.hpp"
#include "Object.hpp"


class ObjectSet : public libdnf::utils::sack::Set<Object *> {};
