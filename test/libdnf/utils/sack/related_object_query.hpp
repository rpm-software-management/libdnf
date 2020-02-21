#pragma once

#include "related_object.hpp"

#include "libdnf/utils/sack/query.hpp"
#include "libdnf/utils/weak_ptr.hpp"

using RelatedObjectWeakPtr = libdnf::utils::WeakPtr<RelatedObject, false>;

class RelatedObjectQuery : public libdnf::utils::sack::Query<RelatedObjectWeakPtr> {
public:
    static std::string get_id(const RelatedObjectWeakPtr & obj) { return obj->id; }
    static int64_t get_value(const RelatedObjectWeakPtr & obj) { return obj->value; }
};
