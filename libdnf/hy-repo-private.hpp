/*
 * Copyright (C) 2012-2013 Red Hat, Inc.
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

#ifndef HY_REPO_INTERNAL_H
#define HY_REPO_INTERNAL_H

#include "repo/Repo-private.hpp"

// libsolv
#include <solv/pooltypes.h>

// hawkey
#include "hy-iutil.h"
#include "hy-types.h"

enum _hy_repo_repodata {
    _HY_REPODATA_FILENAMES,
    _HY_REPODATA_PRESTO,
    _HY_REPODATA_UPDATEINFO,
    _HY_REPODATA_OTHER
};

namespace libdnf {

HyRepo hy_repo_create(const char *name);
void hy_repo_free(HyRepo repo);

void repo_internalize_all_trigger(Pool *pool);
void repo_internalize_trigger(LibsolvRepo *r);
void repo_update_state(HyRepo repo, enum _hy_repo_repodata which,
                       enum _hy_repo_state state);
Id repo_get_repodata(HyRepo repo, enum _hy_repo_repodata which);
void repo_set_repodata(HyRepo repo, enum _hy_repo_repodata which, Id repodata);

Repo::Impl * repoGetImpl(Repo * repo);

}

#endif // HY_REPO_INTERNAL_H
