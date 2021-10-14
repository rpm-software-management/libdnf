/*
Copyright Contributors to the libdnf project.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "libdnf/advisory/advisory.hpp"

#include "libdnf/advisory/advisory_collection.hpp"
#include "libdnf/advisory/advisory_reference.hpp"
#include "libdnf/common/exception.hpp"
#include "libdnf/common/format.hpp"
#include "libdnf/solv/pool.hpp"

namespace libdnf::advisory {

Advisory::Advisory(const libdnf::BaseWeakPtr & base, AdvisoryId id) : base(base), id(id) {}

Advisory::Advisory(libdnf::Base & base, AdvisoryId id) : Advisory(base.get_weak_ptr(), id) {}

std::string Advisory::get_name() const {
    const char * name;
    name = get_pool(base).lookup_str(id.id, SOLVABLE_NAME);

    if (strncmp(
            libdnf::solv::SOLVABLE_NAME_ADVISORY_PREFIX, name, libdnf::solv::SOLVABLE_NAME_ADVISORY_PREFIX_LENGTH) !=
        0) {
        auto msg = format_runtime(
            R"**(Bad libsolv id for advisory "{}", solvable name "{}" doesn't have advisory prefix "{}")**",
            id.id,
            name,
            libdnf::solv::SOLVABLE_NAME_ADVISORY_PREFIX);
        throw RuntimeError(msg);
    }

    return std::string(name + libdnf::solv::SOLVABLE_NAME_ADVISORY_PREFIX_LENGTH);
}

Advisory::Type Advisory::get_type() const {
    const char * type;
    type = get_pool(base).lookup_str(id.id, SOLVABLE_PATCHCATEGORY);

    if (type == NULL) {
        return Advisory::Type::UNKNOWN;
    }
    if (!strcmp(type, "bugfix")) {
        return Advisory::Type::BUGFIX;
    }
    if (!strcmp(type, "enhancement")) {
        return Advisory::Type::ENHANCEMENT;
    }
    if (!strcmp(type, "security")) {
        return Advisory::Type::SECURITY;
    }
    if (!strcmp(type, "newpackage")) {
        return Advisory::Type::NEWPACKAGE;
    }

    return Advisory::Type::UNKNOWN;
}

const char * Advisory::get_type_cstring() const {
    return get_pool(base).lookup_str(id.id, SOLVABLE_PATCHCATEGORY);
}

const char * Advisory::advisory_type_to_cstring(Type type) {
    switch (type) {
        case Type::ENHANCEMENT:
            return "enhancement";
        case Type::SECURITY:
            return "security";
        case Type::NEWPACKAGE:
            return "newpackage";
        case Type::BUGFIX:
            return "bugfix";
        default:
            return NULL;
    }
}

std::string Advisory::get_severity() const {
    //TODO(amatej): should we call SolvPrivate::internalize_libsolv_repo(solvable->repo);
    //              before pool.lookup_str?
    //              If so do this just once in solv::advisroy_private
    const char * severity = get_pool(base).lookup_str(id.id, UPDATE_SEVERITY);
    return severity ? std::string(severity) : std::string();
}

AdvisoryId Advisory::get_id() const {
    return id;
}

std::vector<AdvisoryReference> Advisory::get_references(AdvisoryReferenceType ref_type) const {
    auto & pool = get_pool(base);

    std::vector<AdvisoryReference> output;

    Dataiterator di;
    dataiterator_init(&di, *pool, 0, id.id, UPDATE_REFERENCE, 0, 0);

    for (int index = 0; dataiterator_step(&di); index++) {
        dataiterator_setpos(&di);
        const char * current_type = pool.lookup_str(SOLVID_POS, UPDATE_REFERENCE_TYPE);

        if (((ref_type & AdvisoryReferenceType::CVE) == AdvisoryReferenceType::CVE &&
             (strcmp(current_type, "cve") == 0)) ||
            ((ref_type & AdvisoryReferenceType::BUGZILLA) == AdvisoryReferenceType::BUGZILLA &&
             (strcmp(current_type, "bugzilla") == 0)) ||
            ((ref_type & AdvisoryReferenceType::VENDOR) == AdvisoryReferenceType::VENDOR &&
             (strcmp(current_type, "vendor") == 0))) {
            output.emplace_back(AdvisoryReference(base, id, index));
        }
    }

    dataiterator_free(&di);
    return output;
}

std::vector<AdvisoryCollection> Advisory::get_collections() const {
    std::vector<AdvisoryCollection> output;

    Dataiterator di;
    dataiterator_init(&di, *get_pool(base), 0, id.id, UPDATE_COLLECTIONLIST, 0, 0);

    for (int index = 0; dataiterator_step(&di); index++) {
        dataiterator_setpos(&di);
        output.emplace_back(AdvisoryCollection(base, id, index));
    }

    dataiterator_free(&di);

    return output;
}

//TODO(amatej): this could be possibly removed?
bool Advisory::is_applicable() const {
    for (const auto & collection : get_collections()) {
        if (collection.is_applicable()) {
            return true;
        }
    }

    return false;
}

Advisory::~Advisory() = default;

}  // namespace libdnf::advisory
