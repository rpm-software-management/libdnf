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

// libsolv
#include <solv/pooltypes.h>

// hawkey
#include "hy-iutil.h"
#include "hy-repo.h"

enum _hy_repo_state {
    _HY_NEW,
    _HY_LOADED_FETCH,
    _HY_LOADED_CACHE,
    _HY_WRITTEN
};

struct _HyRepo {
    Repo *libsolv_repo;
    int cost;
    int needs_internalizing;
    int nrefs;
    int priority;
    char *name;
    char *repomd_fn;
    char *primary_fn;
    char *filelists_fn;
    char *presto_fn;
    char *updateinfo_fn;
    enum _hy_repo_state state_main;
    enum _hy_repo_state state_filelists;
    enum _hy_repo_state state_presto;
    enum _hy_repo_state state_updateinfo;
    Id filenames_repodata;
    Id presto_repodata;
    Id updateinfo_repodata;
    unsigned char checksum[CHKSUM_BYTES];
    int load_flags;
    /* the following three elements are needed for repo rewriting */
    int main_nsolvables;
    int main_nrepodata;
    int main_end;
    gboolean use_includes; 
};

enum _hy_repo_repodata {
    _HY_REPODATA_FILENAMES,
    _HY_REPODATA_PRESTO,
    _HY_REPODATA_UPDATEINFO
};

HyRepo hy_repo_link(HyRepo repo);
int hy_repo_transition(HyRepo repo, enum _hy_repo_state new_state);

void repo_finalize_init(HyRepo hrepo, Repo *repo);
void repo_internalize_all_trigger(Pool *pool);
void repo_internalize_trigger(Repo *r);
void repo_update_state(HyRepo repo, enum _hy_repo_repodata which,
                       enum _hy_repo_state state);
Id repo_get_repodata(HyRepo repo, enum _hy_repo_repodata which);
void repo_set_repodata(HyRepo repo, enum _hy_repo_repodata which, Id repodata);

#endif // HY_REPO_INTERNAL_H
