/*
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef LIBDNF_DEPENDENCY_HPP
#define LIBDNF_DEPENDENCY_HPP

#include <memory>
#include <string>
#include <vector>
#include <solv/knownid.h>

#include "libdnf/dnf-sack.h"

namespace libdnf {

struct Dependency
{
public:
    /**
    * @brief Creates a reldep from Id
    */
    Dependency(DnfSack *sack, Id id);

    /**
    * @brief Creates a reldep from name, version, and comparison type.
    *
    * @param sack p_sack: DnfSack*
    * @param name p_name: Required
    * @param version p_version: Can be also NULL
    * @param cmpType p_cmpType: Can be 0 or HY_EQ, HY_LT, HY_GT, and their combinations
    */
    Dependency(DnfSack *sack, const char *name, const char *version, int cmpType);

    /**
    * @brief Creates a reldep from Char*. If parsing fails it raises std::runtime_error.
    *
    * @param sack p_sack:...
    * @param dependency p_dependency:...
    */
    Dependency(DnfSack *sack, const std::string &dependency);
    Dependency(const Dependency &dependency);
    ~Dependency();

    const char *getName() const;
    const char *getRelation() const;
    const char *getVersion() const;
    const char *toString() const;
    Id getId() const noexcept;

private:
    friend DependencyContainer;

    /**
    * @brief Returns Id of reldep
    *
    * @param sack p_sack: DnfSack*
    * @param name p_name: Required
    * @param version p_version: Can be also NULL
    * @param cmpType p_cmpType: Can be 0 or HY_EQ, HY_LT, HY_GT, and their combinations
    * @return Id
    */
    static Id getReldepId(DnfSack *sack, const char *name, const char *version, int cmpType);

    /**
    * @brief Returns Id of reldep or raises std::runtime_error if parsing fails
    *
    * @param sack p_sack:DnfSack
    * @param reldepStr p_reldepStr: const Char* of reldep
    * @return Id
    */
    static Id getReldepId(DnfSack *sack, const char * reldepStr);

    DnfSack *sack;
    Id id;
};

inline Id Dependency::getId() const noexcept { return id; }

}

#endif //LIBDNF_DEPENDENCY_HPP
