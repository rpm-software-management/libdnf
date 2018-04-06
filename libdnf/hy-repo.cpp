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
#include <sys/stat.h>
#include <utime.h>

// libsolv
#include <solv/util.h>

// hawkey
#include "hy-repo-private.hpp"

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

// public functions

HyRepo
hy_repo_create(const char *name)
{
    assert(name);
    HyRepo repo = static_cast<HyRepo>(g_malloc0(sizeof(*repo)));
    repo->nrefs = 1;
    hy_repo_set_string(repo, HY_REPO_NAME, name);
    return repo;
}

LrHandle *
lr_handle_init_local(const char *cachedir)
{
    LrHandle *h = lr_handle_init();
    const char *urls[] = {cachedir, NULL};
    char *download_list[] = LR_YUM_HAWKEY;
    lr_handle_setopt(h, NULL, LRO_REPOTYPE, LR_YUMREPO);
    lr_handle_setopt(h, NULL, LRO_URLS, urls);
    lr_handle_setopt(h, NULL, LRO_YUMDLIST, download_list);
    lr_handle_setopt(h, NULL, LRO_DESTDIR, cachedir);
    lr_handle_setopt(h, NULL, LRO_LOCAL, 1L);
    return h;
}

LrHandle *
lr_handle_init_remote(HyRemote *remote, const char *destdir)
{
    LrHandle *h = lr_handle_init();
    const char *urls[] = {remote->url, NULL};
    char *download_list[] = LR_YUM_HAWKEY;
    lr_handle_setopt(h, NULL, LRO_REPOTYPE, LR_YUMREPO);
    lr_handle_setopt(h, NULL, LRO_URLS, urls);
    lr_handle_setopt(h, NULL, LRO_YUMDLIST, download_list);
    lr_handle_setopt(h, NULL, LRO_DESTDIR, destdir);
    return h;
}

int
mtime(const char *filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_mtime;
}

unsigned long
age(const char *filename)
{
    return time(NULL) - mtime(filename);
}

const char *
cksum(const char *filename, GChecksumType ctype)
{
    FILE *fp;
    if (!(fp = fopen(filename, "rb")))
        return "";
    guchar buffer[4096];
    gssize len = 0;
    GChecksum *csum = g_checksum_new(ctype);
    while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0)
        g_checksum_update(csum, buffer, len);
    gchar *result = g_strdup(g_checksum_get_string(csum));
    g_checksum_free(csum);
    return result;
}

int
hy_repo_load_cache(HyRepo repo, HyMeta *meta, const char *cachedir)
{
    LrYumRepo *md;
    GError *err = NULL;

    LrHandle *h = lr_handle_init_local(cachedir);
    LrResult *r = lr_result_init();

    lr_handle_perform(h, r, &err);
    if (err)
        return 0;
    lr_result_getinfo(r, NULL, LRR_YUM_REPO, &md);

    // Populate repo
    const char *primary_fn = lr_yum_repo_path(md, "primary");
    hy_repo_set_string(repo, HY_REPO_MD_FN, md->repomd);
    hy_repo_set_string(repo, HY_REPO_PRIMARY_FN, primary_fn);

    // Populate meta
    meta->age = age(primary_fn);

    lr_handle_free(h);
    lr_result_free(r);

    return 1;
}

int
hy_repo_can_reuse(HyRepo repo, HyRemote *remote)
{
    LrYumRepo *md;
    GError *err = NULL;
    char tpt[] = "/tmp/tmpdir.XXXXXX";
    char *tmpdir = mkdtemp(tpt);
    char *download_list[] = LR_YUM_REPOMDONLY;

    LrHandle *h = lr_handle_init_remote(remote, tmpdir);
    LrResult *r = lr_result_init();

    lr_handle_setopt(h, NULL, LRO_YUMDLIST, download_list);

    lr_handle_perform(h, r, &err);
    lr_result_getinfo(r, NULL, LRR_YUM_REPO, &md);

    const char *ock = cksum(repo->repomd_fn, G_CHECKSUM_SHA256);
    const char *nck = cksum(md->repomd, G_CHECKSUM_SHA256);

    lr_handle_free(h);
    lr_result_free(r);
    // FIXME create custom function
    dnf_remove_recursive(tmpdir, NULL);

    if (strcmp(ock, nck) == 0)
        return 1;
    return 0;
}

void
hy_repo_fetch(HyRemote *remote)
{
    GError *err = NULL;
    char tpt[] = "/var/tmp/tmpdir.XXXXXX";
    char *tmpdir = mkdtemp(tpt);
    char tmprepodir[strlen(tpt) + 11];
    char repodir[strlen(remote->cachedir) + 11];

    sprintf(repodir, "%s/repodata", remote->cachedir);
    sprintf(tmprepodir, "%s/repodata", tmpdir);

    LrHandle *h = lr_handle_init_remote(remote, tmpdir);
    LrResult *r = lr_result_init();
    lr_handle_perform(h, r, &err);

    dnf_remove_recursive(repodir, NULL);
    mkdir(repodir, NULL);
    rename(tmprepodir, repodir);
    dnf_remove_recursive(tmpdir, NULL);

    lr_handle_free(h);
    lr_result_free(r);
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
 * will be moved to a more appropriate place later.
 *
 * Returns: %TRUE for success
 **/
void
hy_repo_load(HyRepo repo, HyRemote *remote)
{
    HyMeta meta;

    printf("check if cache present\n");
    int cached = hy_repo_load_cache(repo, &meta, remote->cachedir);
    if (cached) {
        if (meta.age <= remote->maxage) {
            printf("using cache, age: %is\n", meta.age);
            return;
        }
        printf("try to reuse\n");
        int ok = hy_repo_can_reuse(repo, remote);
        if (ok) {
            printf("reusing expired cache\n");
            utime(repo->primary_fn, NULL);
            return;
        }
    }

    printf("fetch\n");
    hy_repo_fetch(remote);
    hy_repo_load_cache(repo, &meta, remote->cachedir);
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
