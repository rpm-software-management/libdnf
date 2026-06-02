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
 * License along with this library; if not, see
 * <https://www.gnu.org/licenses/>.
 */


#ifndef __ADVISORY_HPP
#define __ADVISORY_HPP

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <solv/pooltypes.h>
#include "../dnf-advisory.h"
#include "../dnf-types.h"
#include "advisoryref.hpp"

namespace libdnf {

struct AdvisoryPkg;
struct AdvisoryModule;

struct Advisory {
public:
    Advisory(DnfSack *sack, Id advisory);
    bool operator ==(const Advisory & other) const;
    const char *getDescription() const;
    DnfAdvisoryKind getKind() const;
    const char *getName() const;
    void getPackages(std::vector<AdvisoryPkg> & pkglist, bool withFilemanes = true) const;
    std::vector<AdvisoryModule> getModules() const;
    /// Return advisory packages from applicable collections. Collections with
    /// a <module> tag are checked against active module streams. Non-modular
    /// collections (no <module> tag) are excluded when any of their package
    /// names appears in activeModuleArtifactNames.
    void getApplicablePackages(std::vector<AdvisoryPkg> & pkglist, bool withFilemanes,
                               const std::set<std::string> & activeModuleArtifactNames) const;
    /// Convenience overload that computes the active module artifact names
    /// internally. When processing multiple advisories, prefer the overload
    /// above with a precomputed set for better performance.
    void getApplicablePackages(std::vector<AdvisoryPkg> & pkglist, bool withFilemanes = true) const;

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
