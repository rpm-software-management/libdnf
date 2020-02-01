#ifndef LIBDNF_COMPS_CATEGORY_CATEGORYQUERY_HPP
#define LIBDNF_COMPS_CATEGORY_CATEGORYQUERY_HPP


#include "../../utils/sack/Query.hpp"
#include "Category.hpp"


template <>
enum class libdnf::utils::sack::Query<libdnf::comps::Category>::Key {
    id,
    name,
    description,
    translated_name,
    translated_description,
};


namespace libdnf::comps {


class CategoryQuery : public libdnf::utils::sack::Query<Category> {
    CategoryQuery();
};


inline CategoryQuery::CategoryQuery() {
    register_filter_string(Key::id, [](Category * obj) { return obj->get_id(); });
    register_filter_string(Key::name, [](Category * obj) { return obj->get_name(); });
    register_filter_string(Key::description, [](Category * obj) { return obj->get_description(); });
    register_filter_string(Key::translated_name, [](Category * obj) { return obj->get_translated_name(); });
    register_filter_string(Key::translated_description, [](Category * obj) { return obj->get_translated_description(); });
}


}  // namespace libdnf::comps


#endif
