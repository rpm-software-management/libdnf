#ifndef LIBDNF_COMPS_CATEGORY_CATEGORYQUERY_HPP
#define LIBDNF_COMPS_CATEGORY_CATEGORYQUERY_HPP


#include "../../utils/sack/Query.hpp"
#include "Category.hpp"


namespace libdnf::comps {


using CategoryQuery = libdnf::utils::sack::Query<Category>;


template <>
enum class CategoryQuery::Key {
    id,
    name,
    description,
    translated_name,
    translated_description,
};


}  // namespace libdnf::comps


template <>
void libdnf::utils::sack::Query<libdnf::comps::Category>::initialize_filters() {
    add_filter(Key::id, [](libdnf::comps::Category * obj) { return obj->get_id(); });
    add_filter(Key::name, [](libdnf::comps::Category * obj) { return obj->get_name(); });
    add_filter(Key::description, [](libdnf::comps::Category * obj) { return obj->get_description(); });
    add_filter(Key::translated_name, [](libdnf::comps::Category * obj) { return obj->get_translated_name(); });
    add_filter(Key::translated_description, [](libdnf::comps::Category * obj) { return obj->get_translated_description(); });
}


#endif
