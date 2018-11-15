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


#include <solv/selection.h>
#include <solv/solver.h>
#include <solv/util.h>

#include "../dnf-sack-private.hpp"
#include "../dnf-types.h"
#include "../hy-goal-private.hpp"
#include "../hy-iutil.h"
#include "../hy-package-private.hpp"
#include "../hy-query-private.hpp"
#include "../hy-selector-private.hpp"
#include "../hy-util-private.hpp"
#include "../repo/solvable/Dependency.hpp"

namespace libdnf {

static bool
valid_setting(int keyname, int cmp_type)
{
    switch (keyname) {
        case HY_PKG_ARCH:
        case HY_PKG_EVR:
        case HY_PKG_REPONAME:
        case HY_PKG_VERSION:
            return cmp_type == HY_EQ;
        case HY_PKG_PROVIDES:
        case HY_PKG_NAME:
            return cmp_type == HY_EQ || cmp_type == HY_GLOB;
        case HY_PKG_FILE:
            return true;
        default:
            return false;
    }
}

class Selector::Impl {
public:
    DnfSack * sack;
    std::unique_ptr<Filter> filterArch;
    std::unique_ptr<Filter> filterEvr;
    std::unique_ptr<Filter> filterFile;
    std::unique_ptr<Filter> filterName;
    /**
    * @brief Set pkgs created by pool_queuetowhatprovides
    */
    Id pkgs{0};
    std::unique_ptr<Filter> filterProvides;
    std::unique_ptr<Filter> filterReponame;
private:
    friend struct Selector;
    std::vector<_Match> matches;
};

Selector::Selector(DnfSack* sack) : pImpl(new Impl) { pImpl->sack = sack; }

Selector::Selector(Selector && src)
{
    pImpl = std::move(src.pImpl);
    src.pImpl.reset(new Impl);
    src.pImpl->sack = pImpl->sack;
}

Selector::~Selector() = default;

DnfSack *Selector::getSack() { return pImpl->sack; }
const Filter *Selector::getFilterArch() const { return pImpl->filterArch.get(); }
const Filter *Selector::getFilterEvr() const { return pImpl->filterEvr.get(); }
const Filter *Selector::getFilterFile() const { return pImpl->filterFile.get(); }
const Filter *Selector::getFilterName() const { return pImpl->filterName.get(); }
Id Selector::getPkgs() const { return pImpl->pkgs; }
const Filter *Selector::getFilterProvides() const { return pImpl->filterProvides.get(); }
const Filter *Selector::getFilterReponame() const { return pImpl->filterReponame.get(); }

int
Selector::set(const DnfPackageSet *pset)
{
    if (pImpl->filterName || pImpl->filterProvides || pImpl->filterFile) {
        return DNF_ERROR_BAD_SELECTOR;
    }
    dnf_sack_recompute_considered(pImpl->sack);
    dnf_sack_make_provides_ready(pImpl->sack);
    Id id = -1;
    IdQueue pkgs;
    while(true) {
        id = pset->next(id);
        if (id == -1)
            break;
        pkgs.pushBack(id);
    }
    pImpl->pkgs = pool_queuetowhatprovides(dnf_sack_get_pool(pImpl->sack), pkgs.getQueue());

    return 0;
}

int
Selector::set(int keyname, int cmp_type, const char *match)
{
    if (!valid_setting(keyname, cmp_type))
        return DNF_ERROR_BAD_SELECTOR;

    switch (keyname) {
        case HY_PKG_ARCH:
            pImpl->filterArch.reset(new Filter(keyname, cmp_type, match));
            return 0;
        case HY_PKG_EVR:
        case HY_PKG_VERSION:
            pImpl->filterEvr.reset(new Filter(keyname, cmp_type, match));
            return 0;
        case HY_PKG_NAME:
            if (pImpl->filterProvides || pImpl->filterFile || pImpl->pkgs)
                return DNF_ERROR_BAD_SELECTOR;
            pImpl->filterName.reset(new Filter(keyname, cmp_type, match));
            return 0;
        case HY_PKG_PROVIDES:
            if (pImpl->filterName || pImpl->filterFile || pImpl->pkgs)
                return DNF_ERROR_BAD_SELECTOR;
            if (cmp_type != HY_GLOB) {
                try {
                    Dependency reldep(pImpl->sack, match);
                    pImpl->filterProvides.reset(new Filter(keyname, cmp_type, &reldep));
                    return 0;
                }
                catch (...) {
                    pImpl->filterProvides.reset();
                    return DNF_ERROR_BAD_SELECTOR;
                }
            }
            pImpl->filterProvides.reset(new Filter(keyname, cmp_type, match));
            return 0;
        case HY_PKG_REPONAME:
            pImpl->filterReponame.reset(new Filter(keyname, cmp_type, match));
            return 0;
        case HY_PKG_FILE:
            if (pImpl->filterName || pImpl->filterProvides || pImpl->pkgs)
                return DNF_ERROR_BAD_SELECTOR;
            pImpl->filterFile.reset(new Filter(keyname, cmp_type, match));
            return 0;
        default:
            return DNF_ERROR_BAD_SELECTOR;
    }
}

GPtrArray *
Selector::matches()
{
    DnfSack *sack = pImpl->sack;
    Pool *pool = dnf_sack_get_pool(sack);
    Queue job, solvables;

    queue_init(&job);
    sltrToJob(this, &job, 0);

    queue_init(&solvables);
    selection_solvables(pool, &job, &solvables);

    GPtrArray *plist = hy_packagelist_create();
    for (int i = 0; i < solvables.count; i++)
        g_ptr_array_add(plist, dnf_package_new(sack, solvables.elements[i]));

    queue_free(&solvables);
    queue_free(&job);
    return plist;
}

}
