#ifndef LIBDNF_COMPS_ENVIRONMENT_ENVIRONMENTQUERY_HPP
#define LIBDNF_COMPS_ENVIRONMENT_ENVIRONMENTQUERY_HPP


#include "../../utils/sack/Query.hpp"
#include "Environment.hpp"

#include <string>
#include <vector>


template <>
enum class libdnf::utils::sack::Query<libdnf::comps::Environment>::Key {
    id,
    name,
    description,
    translated_name,
    translated_description,
};


namespace libdnf::comps {


/// @replaces dnf:dnf/comps.py:class:Comps
/// @replaces dnf:dnf/comps.py:class:CompsQuery
/// @replaces dnf:dnf/db/group.py:method:EnvironmentPersistor.search_by_pattern(self, pattern)
/// @replaces dnf:dnf/db/group.py:method:EnvironmentPersistor.get(self, obj_id)
/// @replaces dnf:dnf/comps.py:attribute:CompsQuery.AVAILABLE
/// @replaces dnf:dnf/comps.py:attribute:CompsQuery.ENVIRONMENTS
/// @replaces dnf:dnf/comps.py:attribute:CompsQuery.INSTALLED
/// @replaces dnf:dnf/comps.py:method:Comps.environment_by_pattern(self, pattern, case_sensitive=False)
/// @replaces dnf:dnf/comps.py:method:Comps.environments_by_pattern(self, pattern, case_sensitive=False)
/// @replaces dnf:dnf/comps.py:attribute:Comps.environments
/// @replaces dnf:dnf/comps.py:method:Comps.environments_iter(self)
class EnvironmentQuery : public libdnf::utils::sack::Query<Environment> {
    EnvironmentQuery();
};


inline EnvironmentQuery::EnvironmentQuery() {
    register_filter_string(Key::id, [](Environment * obj) { return obj->get_id(); });
    register_filter_string(Key::name, [](Environment * obj) { return obj->get_name(); });
    register_filter_string(Key::description, [](Environment * obj) { return obj->get_description(); });
    register_filter_string(Key::translated_name, [](Environment * obj) { return obj->get_translated_name(); });
    register_filter_string(Key::translated_description, [](Environment * obj) { return obj->get_translated_description(); });
}


// TODO(dmach): filter Environments by included groups
/// @replaces dnf:dnf/db/group.py:method:EnvironmentPersistor.get_group_environments(self, group_id)
// FILTER: Environment.filter(group_id=...).list()


}  // namespace libdnf::comps


#endif
