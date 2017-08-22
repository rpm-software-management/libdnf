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

#include <assert.h>

// libsolv
#include <solv/util.h>

// hawkey
#include "hy-repo-private.h"

HyRepo
hy_repo_link(HyRepo repo)
{
    repo->nrefs++;
    return repo;
}

void
repo_finalize_init(HyRepo hrepo, Repo *repo)
{
    repo->appdata = hy_repo_link(hrepo);
    repo->subpriority = -hrepo->cost;
    repo->priority = -hrepo->priority;
    hrepo->libsolv_repo = repo;
}

void
repo_internalize_all_trigger(Pool *pool)
{
    int i;
    Repo *repo;

    FOR_REPOS(i, repo)
        repo_internalize_trigger(repo);
}

void
repo_internalize_trigger(Repo * repo)
{
    HyRepo hrepo = repo->appdata;
    assert(hrepo->libsolv_repo == repo);
    if (!hrepo->needs_internalizing)
        return;
    hrepo->needs_internalizing = 0;
    repo_internalize(repo);
}

void
repo_update_state(HyRepo repo, enum _hy_repo_repodata which,
                  enum _hy_repo_state state)
{
    assert(state <= _HY_WRITTEN);
    switch (which) {
    case _HY_REPODATA_FILENAMES:
        repo->state_filelists = state;
        return;
    case _HY_REPODATA_PRESTO:
        repo->state_presto = state;
        return;
    case _HY_REPODATA_UPDATEINFO:
        repo->state_updateinfo = state;
        return;
    default:
        assert(0);
    }
    return;
}

Id
repo_get_repodata(HyRepo repo, enum _hy_repo_repodata which)
{
    switch (which) {
    case _HY_REPODATA_FILENAMES:
        return repo->filenames_repodata;
    case _HY_REPODATA_PRESTO:
        return repo->presto_repodata;
    case _HY_REPODATA_UPDATEINFO:
        return repo->updateinfo_repodata;
    default:
        assert(0);
        return 0;
    }
}

void
repo_set_repodata(HyRepo repo, enum _hy_repo_repodata which, Id repodata)
{
    switch (which) {
    case _HY_REPODATA_FILENAMES:
        repo->filenames_repodata = repodata;
        return;
    case _HY_REPODATA_PRESTO:
        repo->presto_repodata = repodata;
        return;
    case _HY_REPODATA_UPDATEINFO:
        repo->updateinfo_repodata = repodata;
        return;
    default:
        assert(0);
        return;
    }
}

// public functions

HyRepo
hy_repo_create(const char *name)
{
    assert(name);
    HyRepo repo = g_malloc0(sizeof(*repo));
    repo->nrefs = 1;
    hy_repo_set_string(repo, HY_REPO_NAME, name);
    return repo;
}

int
hy_repo_get_cost(HyRepo repo)
{
    return repo->cost;
}

int
hy_repo_get_priority(HyRepo repo)
{
    return repo->priority;
}

gboolean
hy_repo_get_use_includes(HyRepo repo)
{
  return repo->use_includes;
}

guint
hy_repo_get_n_solvables(HyRepo repo)
{
  return (guint)repo->libsolv_repo->nsolvables;
}

void
hy_repo_set_cost(HyRepo repo, int value)
{
    repo->cost = value;
    if (repo->libsolv_repo)
        repo->libsolv_repo->subpriority = -value;
}

void
hy_repo_set_priority(HyRepo repo, int value)
{
    repo->priority = value;
    if (repo->libsolv_repo)
        repo->libsolv_repo->priority = -value;
}

void
hy_repo_set_use_includes(HyRepo repo, gboolean enabled)
{
    repo->use_includes = enabled;
}

void
hy_repo_set_string(HyRepo repo, int which, const char *str_val)
{
    switch (which) {
    case HY_REPO_NAME:
        g_free(repo->name);
        repo->name = g_strdup(str_val);
        break;
    case HY_REPO_MD_FN:
        g_free(repo->repomd_fn);
        repo->repomd_fn = g_strdup(str_val);
        break;
    case HY_REPO_PRIMARY_FN:
        g_free(repo->primary_fn);
        repo->primary_fn = g_strdup(str_val);
        break;
    case HY_REPO_FILELISTS_FN:
        g_free(repo->filelists_fn);
        repo->filelists_fn = g_strdup(str_val);
        break;
    case HY_REPO_PRESTO_FN:
        g_free(repo->presto_fn);
        repo->presto_fn = g_strdup(str_val);
        break;
    case HY_REPO_UPDATEINFO_FN:
        g_free(repo->updateinfo_fn);
        repo->updateinfo_fn = g_strdup(str_val);
        break;
    default:
        assert(0);
    }
}

const char *
hy_repo_get_string(HyRepo repo, int which)
{
    switch(which) {
    case HY_REPO_NAME:
        return repo->name;
    case HY_REPO_MD_FN:
        return repo->repomd_fn;
    case HY_REPO_PRIMARY_FN:
        return repo->primary_fn;
    case HY_REPO_FILELISTS_FN:
        return repo->filelists_fn;
    case HY_REPO_PRESTO_FN:
        return repo->presto_fn;
    case HY_REPO_UPDATEINFO_FN:
        return repo->updateinfo_fn;
    default:
        assert(0);
    }
    return NULL;
}

void
hy_repo_free(HyRepo repo)
{
    if (--repo->nrefs > 0)
        return;

    if (repo->libsolv_repo)
        repo->libsolv_repo->appdata = NULL;
    g_free(repo->name);
    g_free(repo->repomd_fn);
    g_free(repo->primary_fn);
    g_free(repo->filelists_fn);
    g_free(repo->presto_fn);
    g_free(repo->updateinfo_fn);
    g_free(repo);
}
