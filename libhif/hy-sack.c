/*
 * Copyright (C) 2012-2015 Red Hat, Inc.
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

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// libsolv
#include <solv/evr.h>
#include <solv/pool.h>
#include <solv/poolarch.h>
#include <solv/repo.h>
#include <solv/repo_deltainfoxml.h>
#include <solv/repo_repomdxml.h>
#include <solv/repo_updateinfoxml.h>
#include <solv/repo_rpmmd.h>
#include <solv/repo_rpmdb.h>
#include <solv/repo_solv.h>
#include <solv/repo_write.h>
#include <solv/solv_xfopen.h>
#include <solv/solver.h>
#include <solv/solverdebug.h>

// hawkey
#include "hif-types.h"
#include "hy-iutil.h"
#include "hy-package-private.h"
#include "hy-packageset-private.h"
#include "hy-query.h"
#include "hy-repo-private.h"
#include "hy-sack-private.h"
#include "hy-util.h"
#include "hif-version.h"

#define DEFAULT_CACHE_ROOT "/var/cache/hawkey"
#define DEFAULT_CACHE_USER "/var/tmp/hawkey"

static int
current_rpmdb_checksum(Pool *pool, unsigned char csout[CHKSUM_BYTES])
{
    const char *rpmdb_prefix_paths[] = { "/var/lib/rpm/Packages",
                                         "/usr/share/rpm/Packages" };
    unsigned int i;
    const char *fn;
    FILE *fp_rpmdb = NULL;
    int ret = 0;

    for (i = 0; i < sizeof(rpmdb_prefix_paths)/sizeof(*rpmdb_prefix_paths); i++) {
        fn = pool_prepend_rootdir_tmp(pool, rpmdb_prefix_paths[i]);
        fp_rpmdb = fopen(fn, "r");
        if (fp_rpmdb)
            break;
    }

    if (!fp_rpmdb || checksum_stat(csout, fp_rpmdb))
        ret = 1;
    if (fp_rpmdb)
        fclose(fp_rpmdb);
    return ret;
}

static int
can_use_rpmdb_cache(FILE *fp_solv, unsigned char cs[CHKSUM_BYTES])
{
    unsigned char cs_cache[CHKSUM_BYTES];

    if (fp_solv &&
        !checksum_read(cs_cache, fp_solv) &&
        !checksum_cmp(cs_cache, cs))
        return 1;

    return 0;
}

static int
can_use_repomd_cache(FILE *fp_solv, unsigned char cs_repomd[CHKSUM_BYTES])
{
    unsigned char cs_cache[CHKSUM_BYTES];

    if (fp_solv &&
        !checksum_read(cs_cache, fp_solv) &&
        !checksum_cmp(cs_cache, cs_repomd))
        return 1;

    return 0;
}

static Map *
free_map_fully(Map *m)
{
    if (m) {
        map_free(m);
        g_free(m);
    }
    return NULL;
}

void
sack_recompute_considered(HySack sack)
{
    Pool *pool = sack_pool(sack);
    if (sack->considered_uptodate)
        return;
    if (!pool->considered) {
        if (!sack->repo_excludes && !sack->pkg_excludes)
            return;
        pool->considered = solv_calloc(1, sizeof(Map));
        map_init(pool->considered, pool->nsolvables);
    } else
        map_grow(pool->considered, pool->nsolvables);

    // considered = (all - repo_excludes - pkg_excludes) and pkg_includes
    map_setall(pool->considered);
    if (sack->repo_excludes)
        map_subtract(pool->considered, sack->repo_excludes);
    if (sack->pkg_excludes)
        map_subtract(pool->considered, sack->pkg_excludes);
    if (sack->pkg_includes)
        map_and(pool->considered, sack->pkg_includes);
    pool_createwhatprovides(sack->pool);
    sack->considered_uptodate = 1;
}

static int
setarch(HySack sack, const char *req_arch)
{
    int ret = 0;
    Pool *pool = sack_pool(sack);

    const char *arch = req_arch;
    char *detected = NULL;
    if (arch == NULL) {
        ret = hy_detect_arch(&detected);
        if (ret) {
            g_warning("hy_detect_arch() failed: %d", ret);
            return ret;
        }
        arch = detected;
    }

    g_debug("Architecture is: %s", arch);
    pool_setarch(pool, arch);
    if (!strcmp(arch, "noarch"))
        // noarch never fails
        goto done;

    /* pool_setarch() doesn't tell us when it defaulted to 'noarch' but we
       consider it a failure. the only way to find out is count the
       architectures known to the Pool. */
    int count = 0;
    for (Id id = 0; id <= pool->lastarch; ++id)
        if (pool->id2arch[id])
            count++;
    if (count < 2)
        ret = HIF_ERROR_FAILED;

 done:
    g_free(detected);
    return ret;
}

// log levels (see also SOLV_ERROR etc. in <solv/pool.h>)
#define HY_LL_INFO  (1 << 20)
#define HY_LL_ERROR (1 << 21)

static void
log_cb(Pool *pool, void *cb_data, int level, const char *buf)
{
    /* just proxy this to the GLib logging handler */
    switch(level) {
        case HY_LL_INFO:
            g_debug ("%s", buf);
            break;
        case HY_LL_ERROR:
            g_warning ("%s", buf);
            break;
        default:
            g_info ("%s", buf);
            break;
    }
}

static void
queue_di(Pool *pool, Queue *queue, const char *str, int di_key, int flags)
{
    Dataiterator di;
    int di_flags = SEARCH_STRING;

    if (flags & HY_ICASE)
        di_flags |= SEARCH_NOCASE;
    if (flags & HY_GLOB)
        di_flags |= SEARCH_GLOB;

    dataiterator_init(&di, pool, 0, 0, di_key, str, di_flags);
    while (dataiterator_step(&di))
        if (is_package(pool, pool_id2solvable(pool, di.solvid)))
            queue_push(queue, di.solvid);
    dataiterator_free(&di);
    return;
}

static void
queue_pkg_name(HySack sack, Queue *queue, const char *provide, int flags)
{
    Pool *pool = sack_pool(sack);
    if (!flags) {
        Id id = pool_str2id(pool, provide, 0);
        if (id == 0)
            return;
        Id p, pp;
        FOR_PKG_PROVIDES(p, pp, id) {
            Solvable *s = pool_id2solvable(pool, p);
            if (s->name == id)
                queue_push(queue, p);
        }
        return;
    }

    queue_di(pool, queue, provide, SOLVABLE_NAME, flags);
    return;
}

static void
queue_provides(HySack sack, Queue *queue, const char *provide, int flags)
{
    Pool *pool = sack_pool(sack);
    if (!flags) {
        Id id = pool_str2id(pool, provide, 0);
        if (id == 0)
            return;
        Id p, pp;
        FOR_PKG_PROVIDES(p, pp, id)
            queue_push(queue, p);
        return;
    }

    queue_di(pool, queue, provide, SOLVABLE_PROVIDES, flags);
    return;
}

static void
queue_filter_version(HySack sack, Queue *queue, const char *version)
{
    Pool *pool = sack_pool(sack);
    int j = 0;
    for (int i = 0; i < queue->count; ++i) {
        Id p = queue->elements[i];
        Solvable *s = pool_id2solvable(pool, p);
        char *e, *v, *r;
        const char *evr = pool_id2str(pool, s->evr);

        pool_split_evr(pool, evr, &e, &v, &r);
        if (!strcmp(v, version))
            queue->elements[j++] = p;
    }
    queue_truncate(queue, j);
}

static gboolean
load_ext(HySack sack, HyRepo hrepo, int which_repodata,
         const char *suffix, int which_filename,
         int (*cb)(Repo *, FILE *), GError **error)
{
    int ret = 0;
    Repo *repo = hrepo->libsolv_repo;
    const char *name = repo->name;
    const char *fn = hy_repo_get_string(hrepo, which_filename);
    FILE *fp;
    gboolean done = FALSE;

    /* nothing set */
    if (fn == NULL) {
        g_set_error (error,
                     HIF_ERROR,
                     HIF_ERROR_FILE_INVALID,
                     "no %d string for %s",
                     which_filename, name);
        return FALSE;
    }

    char *fn_cache =  hy_sack_give_cache_fn(sack, name, suffix);
    fp = fopen(fn_cache, "r");
    assert(hrepo->checksum);
    if (can_use_repomd_cache(fp, hrepo->checksum)) {
        int flags = 0;
        /* the updateinfo is not a real extension */
        if (which_repodata != _HY_REPODATA_UPDATEINFO)
            flags |= REPO_EXTEND_SOLVABLES;
        /* do not pollute the main pool with directory component ids */
        if (which_repodata == _HY_REPODATA_FILENAMES)
            flags |= REPO_LOCALPOOL;
        done = TRUE;
        g_debug("%s: using cache file: %s", __func__, fn_cache);
        ret = repo_add_solv(repo, fp, flags);
        if (ret) {
            g_set_error_literal (error,
                                 HIF_ERROR,
                                 HIF_ERROR_INTERNAL_ERROR,
                                 "failed to add solv");
            return FALSE;
        } else {
            repo_update_state(hrepo, which_repodata, _HY_LOADED_CACHE);
            repo_set_repodata(hrepo, which_repodata, repo->nrepodata - 1);
        }
    }
    g_free(fn_cache);
    if (fp)
        fclose(fp);
    if (done)
        return TRUE;

    fp = solv_xfopen(fn, "r");
    if (fp == NULL) {
        g_set_error (error,
                     HIF_ERROR,
                     HIF_ERROR_FILE_INVALID,
                     "failed to open: %s", fn);
        return FALSE;
    }
    g_debug("%s: loading: %s", __func__, fn);

    int previous_last = repo->nrepodata - 1;
    ret = cb(repo, fp);
    fclose(fp);
    if (ret == 0) {
        repo_update_state(hrepo, which_repodata, _HY_LOADED_FETCH);
        assert(previous_last == repo->nrepodata - 2); (void)previous_last;
        repo_set_repodata(hrepo, which_repodata, repo->nrepodata - 1);
    }
    sack->provides_ready = 0;
    return TRUE;
}

static int
load_filelists_cb(Repo *repo, FILE *fp)
{
    if (repo_add_rpmmd(repo, fp, "FL", REPO_EXTEND_SOLVABLES))
        return HIF_ERROR_INTERNAL_ERROR;
    return 0;
}

static int
load_presto_cb(Repo *repo, FILE *fp)
{
    if (repo_add_deltainfoxml(repo, fp, 0))
        return HIF_ERROR_INTERNAL_ERROR;
    return 0;
}

static int
load_updateinfo_cb(Repo *repo, FILE *fp)
{
    if (repo_add_updateinfoxml(repo, fp, 0))
        return HIF_ERROR_INTERNAL_ERROR;
    return 0;
}

static int
repo_is_one_piece(Repo *repo)
{
    int i;
    for (i = repo->start; i < repo->end; i++)
        if (repo->pool->solvables[i].repo != repo)
            return 0;
    return 1;
}

static gboolean
write_main(HySack sack, HyRepo hrepo, int switchtosolv, GError **error)
{
    Repo *repo = hrepo->libsolv_repo;
    const char *name = repo->name;
    const char *chksum = pool_checksum_str(sack_pool(sack), hrepo->checksum);
    char *fn = hy_sack_give_cache_fn(sack, name, NULL);
    char *tmp_fn_templ = solv_dupjoin(fn, ".XXXXXX", NULL);
    int tmp_fd  = mkstemp(tmp_fn_templ);
    gboolean ret = TRUE;
    gint rc;

    g_debug("caching repo: %s (0x%s)", name, chksum);

    if (tmp_fd < 0) {
        ret = FALSE;
        g_set_error (error,
                     HIF_ERROR,
                     HIF_ERROR_FILE_INVALID,
                     "cannot create temporary file: %s",
                     tmp_fn_templ);
        goto done;
    }

    FILE *fp = fdopen(tmp_fd, "w+");
    if (!fp) {
        ret = FALSE;
        g_set_error (error,
                     HIF_ERROR,
                     HIF_ERROR_FILE_INVALID,
                     "failed opening tmp file: %s",
                     strerror(errno));
        goto done;
    }
    rc = repo_write(repo, fp);
    rc |= checksum_write(hrepo->checksum, fp);
    rc |= fclose(fp);
    if (rc) {
        ret = FALSE;
        g_set_error (error,
                     HIF_ERROR,
                     HIF_ERROR_FILE_INVALID,
                     "write_main() failed writing data: %i", rc);
        goto done;
    }

    if (switchtosolv && repo_is_one_piece(repo)) {
        /* switch over to written solv file activate paging */
        fp = fopen(tmp_fn_templ, "r");
        if (fp) {
            repo_empty(repo, 1);
            rc = repo_add_solv(repo, fp, 0);
            fclose(fp);
            if (rc) {
                /* this is pretty fatal */
                ret = FALSE;
                g_set_error_literal (error,
                                     HIF_ERROR,
                                     HIF_ERROR_FILE_INVALID,
                                     "write_main() failed to re-load "
                                     "written solv file");
                goto done;
            }
        }
    }

    ret = mv(tmp_fn_templ, fn, error);
    if (!ret)
        goto done;
    hrepo->state_main = _HY_WRITTEN;

 done:
    if (!ret && tmp_fd >= 0)
        unlink(tmp_fn_templ);
    g_free(tmp_fn_templ);
    g_free(fn);
    return ret;
}

/* this filter makes sure only the updateinfo repodata is written */
static int
write_ext_updateinfo_filter(Repo *repo, Repokey *key, void *kfdata)
{
    Repodata *data = kfdata;
    if (key->name == 1 && key->size != data->repodataid)
        return -1;
    return repo_write_stdkeyfilter(repo, key, 0);
}

static int
write_ext_updateinfo(HyRepo hrepo, Repodata *data, FILE *fp)
{
    Repo *repo = hrepo->libsolv_repo;
    int oldstart = repo->start;
    repo->start = hrepo->main_end;
    repo->nsolvables -= hrepo->main_nsolvables;
    int res = repo_write_filtered(repo, fp, write_ext_updateinfo_filter, data, 0);
    repo->start = oldstart;
    repo->nsolvables += hrepo->main_nsolvables;
    return res;
}

static gboolean
write_ext(HySack sack, HyRepo hrepo, int which_repodata, const char *suffix, GError **error)
{
    Repo *repo = hrepo->libsolv_repo;
    int ret = 0;
    const char *name = repo->name;

    Id repodata = repo_get_repodata(hrepo, which_repodata);
    assert(repodata);
    Repodata *data = repo_id2repodata(repo, repodata);
    char *fn = hy_sack_give_cache_fn(sack, name, suffix);
    char *tmp_fn_templ = solv_dupjoin(fn, ".XXXXXX", NULL);
    int tmp_fd = mkstemp(tmp_fn_templ);
    gboolean success;
    if (tmp_fd < 0) {
        success = FALSE;
        g_set_error (error,
                     HIF_ERROR,
                     HIF_ERROR_FILE_INVALID,
                     "can not create temporary file %s",
                     tmp_fn_templ);
        goto done;
    }
    FILE *fp = fdopen(tmp_fd, "w+");

    g_debug("%s: storing %s to: %s", __func__, repo->name, tmp_fn_templ);
    if (which_repodata != _HY_REPODATA_UPDATEINFO)
        ret |= repodata_write(data, fp);
    else
        ret |= write_ext_updateinfo(hrepo, data, fp);
    ret |= checksum_write(hrepo->checksum, fp);
    ret |= fclose(fp);
    if (ret) {
        success = FALSE;
        g_set_error (error,
                     HIF_ERROR,
                     HIF_ERROR_FAILED,
                     "write_ext(%d) has failed: %d",
                     which_repodata, ret);
        goto done;
    }

    if (repo_is_one_piece(repo) && which_repodata != _HY_REPODATA_UPDATEINFO) {
        /* switch over to written solv file activate paging */
        fp = fopen(tmp_fn_templ, "r");
        if (fp) {
            int flags = REPO_USE_LOADING | REPO_EXTEND_SOLVABLES;
            /* do not pollute the main pool with directory component ids */
            if (which_repodata == _HY_REPODATA_FILENAMES)
                flags |= REPO_LOCALPOOL;
            repodata_extend_block(data, repo->start, repo->end - repo->start);
            data->state = REPODATA_LOADING;
            repo_add_solv(repo, fp, flags);
            data->state = REPODATA_AVAILABLE;
            fclose(fp);
        }
    }

    if (!mv(tmp_fn_templ, fn, error)) {
        success = FALSE;
        goto done;
    }
    repo_update_state(hrepo, which_repodata, _HY_WRITTEN);
    success = TRUE;
 done:
    if (ret && tmp_fd >=0 )
        unlink(tmp_fn_templ);
    g_free(tmp_fn_templ);
    g_free(fn);
    return success;
}

static gboolean
load_yum_repo(HySack sack, HyRepo hrepo, GError **error)
{
    gboolean retval = TRUE;
    Pool *pool = sack->pool;
    const char *name = hy_repo_get_string(hrepo, HY_REPO_NAME);
    Repo *repo = repo_create(pool, name);
    const char *fn_repomd = hy_repo_get_string(hrepo, HY_REPO_MD_FN);
    char *fn_cache = hy_sack_give_cache_fn(sack, name, NULL);

    FILE *fp_primary = NULL;
    FILE *fp_cache = fopen(fn_cache, "r");
    FILE *fp_repomd = fopen(fn_repomd, "r");
    if (fp_repomd == NULL) {
        g_set_error (error,
                     HIF_ERROR,
                     HIF_ERROR_FILE_INVALID,
                     "can not read file %s: %s",
                     fn_repomd, strerror(errno));
        retval = FALSE;
        goto out;
    }
    checksum_fp(hrepo->checksum, fp_repomd);

    assert(hrepo->state_main == _HY_NEW);
    if (can_use_repomd_cache(fp_cache, hrepo->checksum)) {
        const char *chksum = pool_checksum_str(pool, hrepo->checksum);
        g_debug("using cached %s (0x%s)", name, chksum);
        if (repo_add_solv(repo, fp_cache, 0)) {
            g_set_error (error,
                         HIF_ERROR,
                         HIF_ERROR_INTERNAL_ERROR,
                         "repo_add_solv() has failed.");
            retval = FALSE;
            goto out;
        }
        hrepo->state_main = _HY_LOADED_CACHE;
    } else {
        fp_primary = solv_xfopen(hy_repo_get_string(hrepo, HY_REPO_PRIMARY_FN),
                                 "r");
        assert(fp_primary);

        g_debug("fetching %s", name);
        if (repo_add_repomdxml(repo, fp_repomd, 0) || \
            repo_add_rpmmd(repo, fp_primary, 0, 0)) {
            g_set_error (error,
                         HIF_ERROR,
                         HIF_ERROR_INTERNAL_ERROR,
                         "repo_add_repomdxml/rpmmd() has failed.");
            retval = FALSE;
            goto out;
        }
        hrepo->state_main = _HY_LOADED_FETCH;
    }
out:
    if (fp_cache)
        fclose(fp_cache);
    if (fp_repomd)
        fclose(fp_repomd);
    if (fp_primary)
        fclose(fp_primary);
    g_free(fn_cache);

    if (retval) {
        repo_finalize_init(hrepo, repo);
        sack->provides_ready = 0;
    } else
        repo_free(repo, 1);
    return retval;
}

/**
 * Creates a new package sack, the fundamental hawkey structure.
 *
 * If 'cache_path' is not NULL it is used to override the default filesystem
 * location where hawkey will store its metadata cache and log file.
 *
 * 'arch' specifies the architecture. NULL value causes autodetection.
 * 'rootdir' is the installroot. NULL means the current root, '/'.
 */
HySack
hy_sack_create(const char *cache_path, const char *arch, const char *rootdir,
               const char* log_file, int flags, GError **error)
{
    HySack sack = solv_calloc(1, sizeof(*sack));
    Pool *pool = pool_create();

    pool_set_rootdir(pool, rootdir);
    sack->pool = pool;
    sack->running_kernel_id = -1;
    sack->running_kernel_fn = running_kernel;
    sack->considered_uptodate = 1;
    sack->cmdline_repo_created = 0;
    if (log_file)
        sack->log_file = solv_strdup(log_file);

    if (cache_path != NULL) {
        sack->cache_dir = solv_strdup(cache_path);
    } else if (geteuid()) {
        char *username = this_username();
        char *path = pool_tmpjoin(pool, DEFAULT_CACHE_USER, "-", username);
        path = pool_tmpappend(pool, path, "-", "XXXXXX");
        sack->cache_dir = solv_strdup(path);
        g_free(username);
    } else
        sack->cache_dir = solv_strdup(DEFAULT_CACHE_ROOT);

    int ret;
    if (flags & HY_MAKE_CACHE_DIR) {
        ret = mkcachedir(sack->cache_dir);
        if (ret) {
            g_set_error (error,
                         HIF_ERROR,
                         HIF_ERROR_FILE_INVALID,
                         "failed creating cachedir %s",
                         sack->cache_dir);
            goto fail;
        }
    }
    queue_init(&sack->installonly);

    /* logging up after this*/
    pool_setdebugcallback(pool, log_cb, sack);
    pool_setdebugmask(pool,
                      SOLV_ERROR | SOLV_FATAL | SOLV_WARN | SOLV_DEBUG_RESULT |
                      HY_LL_INFO | HY_LL_ERROR);

    if (setarch(sack, arch)) {
        g_set_error (error,
                     HIF_ERROR,
                     HIF_ERROR_INVALID_ARCHITECTURE,
                     "invalid architecture %s", arch);
        goto fail;
    }

    return sack;
 fail:
    hy_sack_free(sack);
    return NULL;
}

void
hy_sack_free(HySack sack)
{
    Pool *pool = sack->pool;
    Repo *repo;
    int i;

    FOR_REPOS(i, repo) {
        HyRepo hrepo = repo->appdata;
        hy_repo_free(hrepo);
    }
    if (sack->log_out) {
        g_debug("finished");
        fclose(sack->log_out);
    }
    g_free(sack->cache_dir);
    g_free(sack->log_file);
    queue_free(&sack->installonly);

    free_map_fully(sack->pkg_excludes);
    free_map_fully(sack->pkg_includes);
    free_map_fully(sack->repo_excludes);
    free_map_fully(pool->considered);
    pool_free(sack->pool);
    g_free(sack);
}

int
hy_sack_evr_cmp(HySack sack, const char *evr1, const char *evr2)
{
    return pool_evrcmp_str(sack_pool(sack), evr1, evr2, EVRCMP_COMPARE);
}

const char *
hy_sack_get_cache_dir(HySack sack)
{
    return sack->cache_dir;
}

HyPackage
hy_sack_get_running_kernel(HySack sack)
{
    Id id = sack_running_kernel(sack);
    if (id < 0)
        return NULL;
    return package_create(sack, id);
}

char *
hy_sack_give_cache_fn(HySack sack, const char *reponame, const char *ext)
{
    assert(reponame);
    char *fn = solv_dupjoin(sack->cache_dir, "/", reponame);
    if (ext)
        return solv_dupappend(fn, ext, ".solvx");
    return solv_dupappend(fn, ".solv", NULL);
}

const char **
hy_sack_list_arches(HySack sack)
{
    Pool *pool = sack_pool(sack);
    const int BLOCK_SIZE = 31;
    int c = 0;
    const char **ss = NULL;

    if (!(pool->id2arch && pool->lastarch))
        return NULL;

    for (Id id = 0; id <= pool->lastarch; ++id) {
        if (!pool->id2arch[id])
            continue;
        ss = solv_extend(ss, c, 1, sizeof(char*), BLOCK_SIZE);
        ss[c++] = pool_id2str(pool, id);
    }
    ss = solv_extend(ss, c, 1, sizeof(char*), BLOCK_SIZE);
    ss[c++] = NULL;
    return ss;
}

void
hy_sack_set_installonly(HySack sack, const char **installonly)
{
    const char *name;

    queue_empty(&sack->installonly);
    if (installonly == NULL)
        return;
    while ((name = *installonly++) != NULL)
        queue_pushunique(&sack->installonly, pool_str2id(sack->pool, name, 1));
}

void
hy_sack_set_installonly_limit(HySack sack, int limit)
{
    sack->installonly_limit = limit;
}

/**
 * Creates repo for command line rpms.
 *
 * Does nothing if one already created.
 *
 * deprecated in 0.5.3, eligible for dropping after 2015-06-23 AND no sooner
 * than in 0.5.8
 */
void
hy_sack_create_cmdline_repo(HySack sack)
{
    if (sack->cmdline_repo_created)
        return;

    HyRepo hrepo = hy_repo_create(HY_CMDLINE_REPO_NAME);
    Repo *repo = repo_create(sack_pool(sack), HY_CMDLINE_REPO_NAME);
    repo->appdata = hrepo;
    hrepo->libsolv_repo = repo;
    hrepo->needs_internalizing = 1;
}

/**
 * Adds the given .rpm file to the command line repo.
 */
HyPackage
hy_sack_add_cmdline_package(HySack sack, const char *fn)
{
    hy_sack_create_cmdline_repo(sack);
    Repo *repo = repo_by_name(sack, HY_CMDLINE_REPO_NAME);
    Id p;

    assert(repo);
    if (!is_readable_rpm(fn)) {
        g_warning("not a readable RPM file: %s, skipping", fn);
        return NULL;
    }
    p = repo_add_rpm(repo, fn, REPO_REUSE_REPODATA|REPO_NO_INTERNALIZE);
    sack->provides_ready = 0;    /* triggers internalizing later */
    return package_create(sack, p);
}

int
hy_sack_count(HySack sack)
{
    int cnt = 0;
    Id p;
    Pool *pool = sack_pool(sack);

    FOR_PKG_SOLVABLES(p)
        cnt++;
    return cnt;
}

void
hy_sack_add_excludes(HySack sack, HyPackageSet pset)
{
    Pool *pool = sack_pool(sack);
    Map *excl = sack->pkg_excludes;
    Map *nexcl = packageset_get_map(pset);

    if (excl == NULL) {
        excl = solv_calloc(1, sizeof(Map));
        map_init(excl, pool->nsolvables);
        sack->pkg_excludes = excl;
    }
    assert(excl->size >= nexcl->size);
    map_or(excl, nexcl);
    sack->considered_uptodate = 0;
}

void
hy_sack_add_includes(HySack sack, HyPackageSet pset)
{
    Pool *pool = sack_pool(sack);
    Map *incl = sack->pkg_includes;
    Map *nincl = packageset_get_map(pset);

    if (incl == NULL) {
        incl = solv_calloc(1, sizeof(Map));
        map_init(incl, pool->nsolvables);
        sack->pkg_includes = incl;
    }
    assert(incl->size >= nincl->size);
    map_or(incl, nincl);
    sack->considered_uptodate = 0;
}

void
hy_sack_set_excludes(HySack sack, HyPackageSet pset)
{
    sack->pkg_excludes = free_map_fully(sack->pkg_excludes);

    if (pset) {
        Map *nexcl = packageset_get_map(pset);

        sack->pkg_excludes = solv_calloc(1, sizeof(Map));
        map_init_clone(sack->pkg_excludes, nexcl);
    }
    sack->considered_uptodate = 0;
}

void
hy_sack_set_includes(HySack sack, HyPackageSet pset)
{
    sack->pkg_includes = free_map_fully(sack->pkg_includes);

    if (pset) {
        Map *nincl = packageset_get_map(pset);

        sack->pkg_includes = solv_calloc(1, sizeof(Map));
        map_init_clone(sack->pkg_includes, nincl);
    }
    sack->considered_uptodate = 0;
}

int
hy_sack_repo_enabled(HySack sack, const char *reponame, int enabled)
{
    Pool *pool = sack_pool(sack);
    Repo *repo = repo_by_name(sack, reponame);
    Map *excl = sack->repo_excludes;

    if (repo == NULL)
        return HIF_ERROR_INTERNAL_ERROR;
    if (excl == NULL) {
        excl = solv_calloc(1, sizeof(Map));
        map_init(excl, pool->nsolvables);
        sack->repo_excludes = excl;
    }
    repo->disabled = !enabled;
    sack->provides_ready = 0;

    Id p;
    Solvable *s;
    if (repo->disabled)
        FOR_REPO_SOLVABLES(repo, p, s)
            MAPSET(sack->repo_excludes, p);
    else
        FOR_REPO_SOLVABLES(repo, p, s)
            MAPCLR(sack->repo_excludes, p);
    sack->considered_uptodate = 0;
    return 0;
}

gboolean
hy_sack_load_system_repo(HySack sack, HyRepo a_hrepo, int flags, GError **error)
{
    Pool *pool = sack_pool(sack);
    char *cache_fn = hy_sack_give_cache_fn(sack, HY_SYSTEM_REPO_NAME, NULL);
    FILE *cache_fp = fopen(cache_fn, "r");
    int rc;
    gboolean ret = TRUE;
    HyRepo hrepo = a_hrepo;

    g_free(cache_fn);
    if (hrepo)
        hy_repo_set_string(hrepo, HY_REPO_NAME, HY_SYSTEM_REPO_NAME);
    else
        hrepo = hy_repo_create(HY_SYSTEM_REPO_NAME);
    hrepo->load_flags = flags;

    rc = current_rpmdb_checksum(pool, hrepo->checksum);
    if (rc) {
        ret = FALSE;
        g_set_error (error,
                     HIF_ERROR,
                     HIF_ERROR_FILE_INVALID,
                     "failed calculating RPMDB checksum");
        goto finish;
    }

    Repo *repo = repo_create(pool, HY_SYSTEM_REPO_NAME);
    if (can_use_rpmdb_cache(cache_fp, hrepo->checksum)) {
        const char *chksum = pool_checksum_str(pool, hrepo->checksum);
        g_debug("using cached rpmdb (0x%s)", chksum);
        rc = repo_add_solv(repo, cache_fp, 0);
        if (!rc)
            hrepo->state_main = _HY_LOADED_CACHE;
    } else {
        g_debug("fetching rpmdb");
        int flags = REPO_REUSE_REPODATA | RPM_ADD_WITH_HDRID | REPO_USE_ROOTDIR;
        rc = repo_add_rpmdb_reffp(repo, cache_fp, flags);
        if (!rc)
            hrepo->state_main = _HY_LOADED_FETCH;
    }
    if (rc) {
        repo_free(repo, 1);
        ret = FALSE;
        g_set_error (error,
                     HIF_ERROR,
                     HIF_ERROR_FILE_INVALID,
                     "failed loading RPMDB");
        goto finish;
    }

    repo_finalize_init(hrepo, repo);
    pool_set_installed(pool, repo);
    sack->provides_ready = 0;

    const int build_cache = flags & HY_BUILD_CACHE;
    if (hrepo->state_main == _HY_LOADED_FETCH && build_cache) {
        ret = write_main(sack, hrepo, 1, error);
        if (!ret)
            goto finish;
    }

    hrepo->main_nsolvables = repo->nsolvables;
    hrepo->main_nrepodata = repo->nrepodata;
    hrepo->main_end = repo->end;
    sack->considered_uptodate = 0;

 finish:
    if (cache_fp)
        fclose(cache_fp);
    if (a_hrepo == NULL)
        hy_repo_free(hrepo);
    return ret;
}

gboolean
hy_sack_load_repo(HySack sack, HyRepo repo, int flags, GError **error)
{
    GError *error_local = NULL;
    const int build_cache = flags & HY_BUILD_CACHE;
    gboolean retval;
    if (!load_yum_repo(sack, repo, error))
        return FALSE;
    repo->load_flags = flags;
    if (repo->state_main == _HY_LOADED_FETCH && build_cache) {
        if (!write_main(sack, repo, 1, error))
            return FALSE;
    }
    repo->main_nsolvables = repo->libsolv_repo->nsolvables;
    repo->main_nrepodata = repo->libsolv_repo->nrepodata;
    repo->main_end = repo->libsolv_repo->end;
    if (flags & HY_LOAD_FILELISTS) {
        retval = load_ext(sack, repo, _HY_REPODATA_FILENAMES,
                          HY_EXT_FILENAMES, HY_REPO_FILELISTS_FN,
                          load_filelists_cb, &error_local);
        /* allow missing files */
        if (!retval) {
            if (g_error_matches (error_local,
                                 HIF_ERROR,
                                 HIF_ERROR_NO_CAPABILITY)) {
                g_debug("no filelists metadata available for %s", repo->name);
                g_clear_error (&error_local);
            } else {
                g_propagate_error (error, error_local);
                return FALSE;
            }
        }
        if (repo->state_filelists == _HY_LOADED_FETCH && build_cache) {
            if (!write_ext(sack, repo,
                           _HY_REPODATA_FILENAMES,
                           HY_EXT_FILENAMES, error))
                return FALSE;
        }
    }
    if (flags & HY_LOAD_PRESTO) {
        retval = load_ext(sack, repo, _HY_REPODATA_PRESTO,
                          HY_EXT_PRESTO, HY_REPO_PRESTO_FN,
                          load_presto_cb, &error_local);
        if (!retval) {
            if (g_error_matches (error_local,
                                 HIF_ERROR,
                                 HIF_ERROR_NO_CAPABILITY)) {
                g_debug("no presto metadata available for %s", repo->name);
                g_clear_error (&error_local);
            } else {
                g_propagate_error (error, error_local);
                return FALSE;
            }
        }
        if (repo->state_presto == _HY_LOADED_FETCH && build_cache)
            if (!write_ext(sack, repo, _HY_REPODATA_PRESTO, HY_EXT_PRESTO, error))
                return FALSE;
    }
    /* updateinfo must come *after* all other extensions, as it is not a real
       extension, but contains a new set of packages */
    if (flags & HY_LOAD_UPDATEINFO) {
        retval = load_ext(sack, repo, _HY_REPODATA_UPDATEINFO,
                          HY_EXT_UPDATEINFO, HY_REPO_UPDATEINFO_FN,
                          load_updateinfo_cb, &error_local);
        /* allow missing files */
        if (!retval) {
            if (g_error_matches (error_local,
                                 HIF_ERROR,
                                 HIF_ERROR_NO_CAPABILITY)) {
                g_debug("no updateinfo available for %s", repo->name);
                g_clear_error (&error_local);
            } else {
                g_propagate_error (error, error_local);
                return FALSE;
            }
        }
        if (repo->state_updateinfo == _HY_LOADED_FETCH && build_cache)
            if (!write_ext(sack, repo, _HY_REPODATA_UPDATEINFO, HY_EXT_UPDATEINFO, error))
                return FALSE;
    }
    sack->considered_uptodate = 0;
    return TRUE;
}

// internal to hawkey

// return true if q1 is a superset of q2
// only works if there are no duplicates both in q1 and q2
// the map parameter must point to an empty map that can hold all ids
// (it is also returned empty)
static int
is_superset(Queue *q1, Queue *q2, Map *m)
{
    int i, cnt = 0;
    for (i = 0; i < q2->count; i++)
        MAPSET(m, q2->elements[i]);
    for (i = 0; i < q1->count; i++)
        if (MAPTST(m, q1->elements[i]))
            cnt++;
    for (i = 0; i < q2->count; i++)
        MAPCLR(m, q2->elements[i]);
    return cnt == q2->count;
}


static void
rewrite_repos(HySack sack, Queue *addedfileprovides,
              Queue *addedfileprovides_inst)
{
    Pool *pool = sack_pool(sack);
    int i;

    Map providedids;
    map_init(&providedids, pool->ss.nstrings);

    Queue fileprovidesq;
    queue_init(&fileprovidesq);

    Repo *repo;
    FOR_REPOS(i, repo) {
        HyRepo hrepo = repo->appdata;
        if (!hrepo)
            continue;
        if (!(hrepo->load_flags & HY_BUILD_CACHE))
            continue;
        if (hrepo->main_nrepodata < 2)
            continue;
        /* now check if the repo already contains all of our file provides */
        Queue *addedq = repo == pool->installed && addedfileprovides_inst ?
            addedfileprovides_inst : addedfileprovides;
        if (!addedq->count)
            continue;
        Repodata *data = repo_id2repodata(repo, 1);
        queue_empty(&fileprovidesq);
        if (repodata_lookup_idarray(data, SOLVID_META,
                                    REPOSITORY_ADDEDFILEPROVIDES,
                                    &fileprovidesq)) {
            if (is_superset(&fileprovidesq, addedq, &providedids))
                continue;
        }
        repodata_set_idarray(data, SOLVID_META,
                             REPOSITORY_ADDEDFILEPROVIDES, addedq);
        repodata_internalize(data);
        /* re-write main data only */
        int oldnrepodata = repo->nrepodata;
        int oldnsolvables = repo->nsolvables;
        int oldend = repo->end;
        repo->nrepodata = hrepo->main_nrepodata;
        repo->nsolvables = hrepo->main_nsolvables;
        repo->end = hrepo->main_end;
        g_debug("rewriting repo: %s", repo->name);
        write_main(sack, hrepo, 0, NULL);
        repo->nrepodata = oldnrepodata;
        repo->nsolvables = oldnsolvables;
        repo->end = oldend;
    }
    queue_free(&fileprovidesq);
    map_free(&providedids);
}

void
sack_make_provides_ready(HySack sack)
{
    if (!sack->provides_ready) {
        Queue addedfileprovides;
        Queue addedfileprovides_inst;
        queue_init(&addedfileprovides);
        queue_init(&addedfileprovides_inst);
        pool_addfileprovides_queue(sack->pool, &addedfileprovides,
                                   &addedfileprovides_inst);
        if (addedfileprovides.count || addedfileprovides_inst.count)
            rewrite_repos(sack, &addedfileprovides, &addedfileprovides_inst);
        queue_free(&addedfileprovides);
        queue_free(&addedfileprovides_inst);
        pool_createwhatprovides(sack->pool);
        sack->provides_ready = 1;
    }
}

Id
sack_running_kernel(HySack sack)
{
    if (sack->running_kernel_id >= 0)
        return sack->running_kernel_id;
    sack->running_kernel_id = sack->running_kernel_fn(sack);
    return sack->running_kernel_id;
}

void
sack_log(HySack sack, int level, const char *format, ...)
{
    Pool *pool = sack_pool(sack);
    char buf[1024];
    va_list args;
    const char *format_nl = pool_tmpjoin(pool, format, "\n", NULL);

    /* add a newline and forward everything to the pool logging */
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format_nl, args);
    va_end(args);
    POOL_DEBUG(level, "%s", buf);
}

int
sack_knows(HySack sack, const char *name, const char *version, int flags)
{
    Queue *q = solv_malloc(sizeof(*q));
    int ret;
    int name_only = flags & HY_NAME_ONLY;

    assert((flags & ~(HY_ICASE|HY_NAME_ONLY|HY_GLOB)) == 0);
    queue_init(q);
    sack_make_provides_ready(sack);
    flags &= ~HY_NAME_ONLY;

    if (name_only) {
        queue_pkg_name(sack, q, name, flags);
        if (version != NULL)
            queue_filter_version(sack, q, version);
    } else
        queue_provides(sack, q, name, flags);

    ret = q->count > 0;
    queue_free(q);
    g_free(q);
    return ret;
}
