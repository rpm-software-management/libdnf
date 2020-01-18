#ifndef LIBDNF_COMPS_ENVIRONMENT_ENVIRONMENTQUERY_HPP
#define LIBDNF_COMPS_ENVIRONMENT_ENVIRONMENTQUERY_HPP


#include "../../utils/sack/Query.hpp"
#include "Environment.hpp"

#include <string>
#include <vector>


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
using EnvironmentQuery = libdnf::utils::sack::Query<Environment>;


}  // namespace libdnf::comps


template <>
void libdnf::utils::sack::Query<libdnf::comps::Environment>::initialize_filters() {
    add_filter("id", [](libdnf::comps::Environment * obj) { return obj->get_id(); });
    add_filter("name", [](libdnf::comps::Environment * obj) { return obj->get_name(); });
    add_filter("description", [](libdnf::comps::Environment * obj) { return obj->get_description(); });
    add_filter("translated_name", [](libdnf::comps::Environment * obj) { return obj->get_translated_name(); });
    add_filter("translated_description", [](libdnf::comps::Environment * obj) { return obj->get_translated_description(); });
}


// TODO(dmach): filter environments by included groups
/// @replaces dnf:dnf/db/group.py:method:EnvironmentPersistor.get_group_environments(self, group_id)
// FILTER: Environment.filter(group_id=...).list()



#endif
