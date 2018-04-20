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
#include <utime.h>

// libsolv
#include <solv/util.h>

// hawkey
#include "hy-repo-private.hpp"
#include "hy-util-private.hpp"

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
    auto hrepo = static_cast<HyRepo>(repo->appdata);
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

LrHandle *
lr_handle_init_base(HyRemote *remote)
{
    LrHandle *h = lr_handle_init();
    const char *dlist[] = {"primary", "filelists", "prestodelta", "group_gz",
                           "updateinfo", NULL};
    lr_handle_setopt(h, NULL, LRO_REPOTYPE, LR_YUMREPO);
    lr_handle_setopt(h, NULL, LRO_USERAGENT, "libdnf/1.0"); //FIXME
    lr_handle_setopt(h, NULL, LRO_YUMDLIST, dlist);
    lr_handle_setopt(h, NULL, LRO_INTERRUPTIBLE, 1L);
    lr_handle_setopt(h, NULL, LRO_GPGCHECK, remote->gpgcheck);
    lr_handle_setopt(h, NULL, LRO_MAXMIRRORTRIES, remote->max_mirror_tries);
    lr_handle_setopt(h, NULL, LRO_MAXPARALLELDOWNLOADS,
                     remote->max_parallel_downloads);
    return h;
}

LrHandle *
lr_handle_init_local(HyRemote *remote, const char *cachedir)
{
    LrHandle *h = lr_handle_init_base(remote);
    const char *urls[] = {cachedir, NULL};
    //TODO set LRO_VARSUB
    lr_handle_setopt(h, NULL, LRO_URLS, urls);
    lr_handle_setopt(h, NULL, LRO_DESTDIR, cachedir);
    lr_handle_setopt(h, NULL, LRO_LOCAL, 1L);
    return h;
}

LrHandle *
lr_handle_init_remote(HyRemote *remote, const char *destdir)
{
    LrHandle *h = lr_handle_init_base(remote);
    const char *urls[] = {remote->url, NULL};
    lr_handle_setopt(h, NULL, LRO_URLS, urls);
    lr_handle_setopt(h, NULL, LRO_DESTDIR, destdir);
    return h;
}

int
hy_repo_load_cache(HyRepo repo, HyRemote *remote)
{
    GError *err = NULL;
    char **mirrors;
    LrYumRepo *yum_repo;
    LrYumRepoMd *yum_repomd;

    LrHandle *h = lr_handle_init_local(remote, repo->cachedir);
    LrResult *r = lr_result_init();

    // Fetch data
    lr_handle_perform(h, r, &err);
    if (err)
        return 0;
    lr_handle_getinfo(h, NULL, LRI_MIRRORS, &mirrors);
    lr_result_getinfo(r, NULL, LRR_YUM_REPO, &yum_repo);
    lr_result_getinfo(r, NULL, LRR_YUM_REPOMD, &yum_repomd);
    const char *repomd_fn = yum_repo->repomd;
    const char *primary_fn = lr_yum_repo_path(yum_repo, "primary");
    const char *filelists_fn = lr_yum_repo_path(yum_repo, "filelists");
    const char *presto_fn = lr_yum_repo_path(yum_repo, "prestodelta");
    const char *updateinfo_fn = lr_yum_repo_path(yum_repo, "updateinfo");

    // Populate repo
    hy_repo_set_string(repo, HY_REPO_MD_FN, repomd_fn);
    hy_repo_set_string(repo, HY_REPO_PRIMARY_FN, primary_fn);
    hy_repo_set_string(repo, HY_REPO_FILELISTS_FN, filelists_fn);
    hy_repo_set_string(repo, HY_REPO_PRESTO_FN, presto_fn);
    hy_repo_set_string(repo, HY_REPO_UPDATEINFO_FN, updateinfo_fn);
    // Load timestamp unless explicitly expired
    if (repo->timestamp != 0) {
        repo->timestamp = mtime(primary_fn);
    }
    // These are for DNF compatibility
    repo->mirrors = mirrors;
    repo->yum_repo = lr_yum_repo_init();
    repo->yum_repomd = lr_yum_repomd_init();
    copy_yum_repo(repo->yum_repo, yum_repo);
    copy_yum_repomd(repo->yum_repomd, yum_repomd);

    lr_handle_free(h);
    lr_result_free(r);

    return 1;
}

int
hy_repo_can_reuse(HyRepo repo, HyRemote *remote)
{
    LrYumRepo *yum_repo;
    GError *err = NULL;
    char templt[] = "/tmp/tmpdir.XXXXXX";
    char *tmpdir = mkdtemp(templt);
    const char *dlist[] = LR_YUM_REPOMDONLY;

    LrHandle *h = lr_handle_init_remote(remote, tmpdir);
    LrResult *r = lr_result_init();

    lr_handle_setopt(h, NULL, LRO_YUMDLIST, dlist);

    lr_handle_perform(h, r, &err);
    lr_result_getinfo(r, NULL, LRR_YUM_REPO, &yum_repo);

    const char *ock = cksum(repo->repomd_fn, G_CHECKSUM_SHA256);
    const char *nck = cksum(yum_repo->repomd, G_CHECKSUM_SHA256);

    lr_handle_free(h);
    lr_result_free(r);
    rmtree(tmpdir);

    if (strcmp(ock, nck) == 0)
        return 1;
    return 0;
}

void
hy_repo_fetch(HyRepo repo, HyRemote *remote)
{
    GError *err = NULL;
    char templt[] = "/var/tmp/tmpdir.XXXXXX";
    char *tmpdir = mkdtemp(templt);
    char tmprepodir[strlen(templt) + 11];
    char repodir[strlen(repo->cachedir) + 11];

    sprintf(tmprepodir, "%s/repodata", tmpdir);
    sprintf(repodir, "%s/repodata", repo->cachedir);

    LrHandle *h = lr_handle_init_remote(remote, tmpdir);
    LrResult *r = lr_result_init();
    lr_handle_perform(h, r, &err);

    rmtree(repodir);
    g_mkdir_with_parents(repodir, 0);
    rename(tmprepodir, repodir);
    rmtree(tmpdir);

    repo->timestamp = -1;

    lr_handle_free(h);
    lr_result_free(r);
}

// public functions

HyRepo
hy_repo_create(const char *name)
{
    assert(name);
    HyRepo repo = static_cast<HyRepo>(g_malloc0(sizeof(*repo)));
    repo->nrefs = 1;
    repo->timestamp = -1;
    hy_repo_set_string(repo, HY_REPO_NAME, name);
    return repo;
}

/**
 * hy_repo_load:
 * @repo: a #HyRepo instance.
 * @remote: a #HyRemote instance.
 *
 * Initializes the repo according to the remote spec.
 * Fetches new metadata from the remote or just reuses local cache if valid.
 *
 * FIXME: This attempts to be a C rewrite of Repo.load() in DNF.  This function
 * may be moved to a more appropriate place later.
 *
 * Returns: %TRUE for success
 **/
void
hy_repo_load(HyRepo repo, HyRemote *remote)
{
    printf("check if cache present\n");
    if (hy_repo_load_cache(repo, remote)) {
        if (!hy_repo_expired(repo)) {
            printf("using cache, age: %is\n", hy_repo_get_age(repo));
            return;
        }
        printf("try to reuse\n");
        if (hy_repo_can_reuse(repo, remote)) {
            printf("reusing expired cache\n");
            hy_repo_reset_timestamp(repo);
            return;
        }
    }

    printf("fetch\n");
    hy_repo_fetch(repo, remote);
    hy_repo_load_cache(repo, remote);
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

int
hy_repo_get_age(HyRepo repo)
{
    return time(NULL) - repo->timestamp;
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

LrYumRepo *hy_repo_get_yum_repo(HyRepo repo)
{
    return repo->yum_repo;
}

LrYumRepoMd *hy_repo_get_yum_repomd(HyRepo repo)
{
    return repo->yum_repomd;
}

char **hy_repo_get_mirrors(HyRepo repo)
{
    return repo->mirrors;
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
hy_repo_set_max_age(HyRepo repo, int value)
{
    repo->max_age = value;
}

int
hy_repo_expired(HyRepo repo)
{
    return repo->timestamp == 0 ||
           (repo->max_age >= 0 && hy_repo_get_age(repo) > repo->max_age);
}

void
hy_repo_expire(HyRepo repo)
{
    repo->timestamp = 0;
}

void
hy_repo_reset_timestamp(HyRepo repo)
{
    time_t now = time(NULL);
    struct utimbuf unow;
    unow.actime = now;
    unow.modtime = now;
    repo->timestamp = now;
    utime(repo->primary_fn, &unow);
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
    case HY_REPO_CACHEDIR:
        g_free(repo->cachedir);
        repo->cachedir = g_strdup(str_val);
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
    case HY_REPO_CACHEDIR:
        return repo->cachedir;
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
    g_free(repo->cachedir);
    g_free(repo);
}
