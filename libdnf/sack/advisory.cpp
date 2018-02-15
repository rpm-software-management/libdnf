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

#include <assert.h>

#include <solv/repo.h>

#include "advisory.hpp"
#include "advisorypkg.hpp"
#include "advisoryref.hpp"
#include "../dnf-advisory-private.hpp"
#include "../dnf-advisoryref.h"
#include "../dnf-sack-private.hpp"

namespace libdnf {

/**
 * str2dnf_advisory_kind:
 * @str: a string
 *
 * Returns: a #DnfAdvisoryKind, e.g. %DNF_ADVISORY_KIND_BUGFIX
 *
 * Since: 0.7.0
 */
static DnfAdvisoryKind
str2dnf_advisory_kind(const char *str)
{
    if (str == NULL)
        return DNF_ADVISORY_KIND_UNKNOWN;
    if (!strcmp (str, "bugfix"))
        return DNF_ADVISORY_KIND_BUGFIX;
    if (!strcmp (str, "enhancement"))
        return DNF_ADVISORY_KIND_ENHANCEMENT;
    if (!strcmp (str, "security"))
        return DNF_ADVISORY_KIND_SECURITY;
    if (!strcmp (str, "newpackage"))
        return DNF_ADVISORY_KIND_NEWPACKAGE;
    return DNF_ADVISORY_KIND_UNKNOWN;
}

bool
Advisory::matchBugOrCVE(const char* what, bool matchBug) const
{
    Pool *pool = dnf_sack_get_pool(sack);
    const char * whatMatch = matchBug ? "bugzilla" : "cve";
    Dataiterator di;
    dataiterator_init(&di, pool, 0, advisory, UPDATE_REFERENCE, 0, 0);
    while (dataiterator_step(&di)) {
        dataiterator_setpos(&di);
        if (strcmp(pool_lookup_str(pool, SOLVID_POS, UPDATE_REFERENCE_TYPE), whatMatch) == 0) {
            if (strcmp(pool_lookup_str(pool, SOLVID_POS, UPDATE_REFERENCE_ID), what) == 0) {
                dataiterator_free(&di);
                return true;
            }
        }
    }
    dataiterator_free(&di);
    return false;
}

Advisory::Advisory(DnfSack *sack, Id advisory) : sack(sack), advisory(advisory) {}

bool
Advisory::operator==(const Advisory & other) const
{
    return sack == other.sack && advisory == other.advisory;
}

const char *
Advisory::getDescription() const
{
    return pool_lookup_str(dnf_sack_get_pool(sack), advisory, SOLVABLE_DESCRIPTION);
}


DnfAdvisoryKind
Advisory::getKind() const
{
    const char *type;
    type = pool_lookup_str(dnf_sack_get_pool(sack), advisory, SOLVABLE_PATCHCATEGORY);
    return str2dnf_advisory_kind(type);
}

const char *
Advisory::getName() const
{
    const char *name;

    name = pool_lookup_str(dnf_sack_get_pool(sack), advisory, SOLVABLE_NAME);
    size_t prefix_len = strlen(SOLVABLE_NAME_ADVISORY_PREFIX);
    assert(strncmp(SOLVABLE_NAME_ADVISORY_PREFIX, name, prefix_len) == 0);
    //remove the prefix
    name += prefix_len;

    return name;
}

void
Advisory::getPackages(std::vector<AdvisoryPkg> & pkglist, bool withFilemanes) const
{
    Dataiterator di;
    const char * filename = nullptr;
    Pool *pool = dnf_sack_get_pool(sack);

    dataiterator_init(&di, pool, 0, advisory, UPDATE_COLLECTION, 0, 0);
    while (dataiterator_step(&di)) {
        dataiterator_setpos(&di);
        Id name = pool_lookup_id(pool, SOLVID_POS, UPDATE_COLLECTION_NAME);
        Id evr = pool_lookup_id(pool, SOLVID_POS, UPDATE_COLLECTION_EVR);
        Id arch = pool_lookup_id(pool, SOLVID_POS, UPDATE_COLLECTION_ARCH);
        if (withFilemanes) {
            filename = pool_lookup_str(pool, SOLVID_POS, UPDATE_COLLECTION_FILENAME);
        }
        pkglist.emplace_back(sack, advisory, name, evr, arch, filename);
    }
    dataiterator_free(&di);
}

void
Advisory::getReferences(std::vector<AdvisoryRef> & reflist) const
{
    Dataiterator di;
    Pool *pool = dnf_sack_get_pool(sack);

    dataiterator_init(&di, pool, 0, advisory, UPDATE_REFERENCE, 0, 0);
    for (int index = 0; dataiterator_step(&di); index++) {
        reflist.emplace_back(sack, advisory, index);
    }
    dataiterator_free(&di);
}

const char *
Advisory::getRights() const
{
    return pool_lookup_str(dnf_sack_get_pool(sack), advisory, UPDATE_RIGHTS);
}

const char *
Advisory::getSeverity() const
{
    return pool_lookup_str(dnf_sack_get_pool(sack), advisory, UPDATE_SEVERITY);
}

const char *
Advisory::getTitle() const
{
    return pool_lookup_str(dnf_sack_get_pool(sack), advisory, SOLVABLE_SUMMARY);
}

unsigned long long int
Advisory::getUpdated() const
{
    return pool_lookup_num(dnf_sack_get_pool(sack), advisory, SOLVABLE_BUILDTIME, 0);
}

bool
Advisory::matchBug(const char *bug) const
{
    return matchBugOrCVE(bug, true);
}

bool
Advisory::matchCVE(const char *cve) const
{
    return matchBugOrCVE(cve, false);
}

bool
Advisory::matchKind(const char *kind) const
{
    auto advisoryKind = pool_lookup_str(dnf_sack_get_pool(sack), advisory, SOLVABLE_PATCHCATEGORY);
    return advisoryKind ? strcmp(advisoryKind, kind) == 0 : false;
}


bool
Advisory::matchName(const char *name) const
{
    auto advisoryName = getName();
    return advisoryName ? strcmp(advisoryName, name) == 0 : false;
}

bool
Advisory::matchSeverity(const char *severity) const
{
    auto advisorySeverity = getSeverity();
    return advisorySeverity ? strcmp(advisorySeverity, severity) == 0 : false;
}

}
