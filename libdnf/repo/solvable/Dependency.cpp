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

#include "Dependency.hpp"
#include "libdnf/utils/utils.hpp"
#include "libdnf/repo/DependencySplitter.hpp"

/* workaround, libsolv lacks 'extern "C"' in its header file */
extern "C" {
#include <solv/pool_parserpmrichdep.h>
#include <solv/util.h>
}


#define NUMBER_OF_ITEMS_IN_RELATIONAL_DEPENDENCY 3

static int transformToLibsolvComparisonType(int cmp_type)
{
    int type = 0;
    if (cmp_type & HY_EQ)
        type |= REL_EQ;
    if (cmp_type & HY_LT)
        type |= REL_LT;
    if (cmp_type & HY_GT)
        type |= REL_GT;

    return type;
}

Dependency::Dependency(DnfSack *sack, Id id)
        : sack(sack)
        , id(id)
{}

Dependency::Dependency(DnfSack *sack, const char *name, const char *version,
    int solvComparisonOperator)
        : sack(sack)
{
    
    setSolvId(name, version, transformToLibsolvComparisonType(solvComparisonOperator));
}

Dependency::Dependency(DnfSack *sack, const std::string &dependency)
        : sack(sack)
{
    const char * reldep_str = dependency.c_str();
    if (dependency.c_str()[0] == '(') {
        /* Rich dependency */
        Pool *pool = dnf_sack_get_pool (sack);
        id = pool_parserpmrichdep(pool, reldep_str);
        if (!id)
            throw std::runtime_error("Cannot parse a dependency string");
        return;
    } else {
        libdnf::DependencySplitter depSplitter;
        if(!depSplitter.parse(reldep_str))
            throw std::runtime_error("Cannot parse a dependency string");
        int solvComparisonOperator = transformToLibsolvComparisonType(depSplitter.getCmpType());
        setSolvId(depSplitter.getNameCStr(), depSplitter.getEVRCStr(), solvComparisonOperator);
    }
}


Dependency::Dependency(const Dependency &dependency)
        : sack(dependency.sack)
        , id(dependency.id)
{}

Dependency::~Dependency() = default;
const char *Dependency::getName() const { return pool_id2str(dnf_sack_get_pool(sack), id); }
const char *Dependency::getRelation() const { return pool_id2rel(dnf_sack_get_pool(sack), id); }
const char *Dependency::getVersion() const { return pool_id2evr(dnf_sack_get_pool(sack), id); }
const char *Dependency::toString() const { return pool_dep2str(dnf_sack_get_pool(sack), id); }

void
Dependency::setSolvId(const char *name, const char *version, int solvComparisonOperator)
{
    Pool *pool = dnf_sack_get_pool(sack);
    id = pool_str2id(pool, name, 1);

    if (version) {
        Id evrId = pool_str2id(pool, version, 1);
        id = pool_rel2id(pool, id, evrId, solvComparisonOperator, 1);
    }
}
