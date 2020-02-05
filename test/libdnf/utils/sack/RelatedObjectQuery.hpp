#pragma once

#include "libdnf/utils/sack/Query.hpp"
#include "RelatedObject.hpp"
#include "libdnf/utils/WeakPtr.hpp"

using RelatedObjectWeakPtr = libdnf::utils::WeakPtr<RelatedObject, false>;

class RelatedObjectQuery : public libdnf::utils::sack::Query<RelatedObjectWeakPtr> {
public:
    static std::string get_id(const RelatedObjectWeakPtr & obj) { return obj->id; }
    static int64_t get_value(const RelatedObjectWeakPtr & obj) { return obj->value; }
};
