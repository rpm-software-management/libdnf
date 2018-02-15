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


#ifndef __ADVISORY_HPP
#define __ADVISORY_HPP

#include <memory>
#include <vector>

#include <solv/pooltypes.h>
#include "../dnf-advisory.h"
#include "../dnf-types.h"
#include "advisoryref.hpp"

namespace libdnf {

struct AdvisoryPkg;

struct Advisory {
public:
    Advisory(DnfSack *sack, Id advisory);
    bool operator ==(const Advisory & other) const;
    const char *getDescription() const;
    DnfAdvisoryKind getKind() const;
    const char *getName() const;
    void getPackages(std::vector<AdvisoryPkg> & pkglist, bool withFilemanes = true) const;
    void getReferences(std::vector<AdvisoryRef> & reflist) const;
    const char *getRights() const;
    const char *getSeverity() const;
    const char *getTitle() const;
    unsigned long long int getUpdated() const;
    bool matchBug(const char *bug) const;
    bool matchCVE(const char *cve) const;
    bool matchKind(const char *kind) const;
    bool matchName(const char *name) const;
    bool matchSeverity(const char *severity) const;

private:
    DnfSack *sack;
    Id advisory;
    bool matchBugOrCVE(const char *bug, bool withBug) const;
};
}

#endif /* __ADVISORY_HPP */
