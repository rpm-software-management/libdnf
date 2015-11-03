/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2012-2015 Red Hat, Inc.
 * Copyright (C) 2013-2015 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or(at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/**
 * SECTION:hif-sack
 * @short_description: A package sack
 * @include: libhif.h
 * @stability: Unstable
 *
 * Sacks are repositories of packages.
 *
 * See also: #HifContext
 */

#include "config.h"

#define _GNU_REPO
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

#include "hif-types.h"
#include "hif-version.h"
#include "hy-iutil.h"
#include "hy-package-private.h"
#include "hy-packageset-private.h"
#include "hy-query.h"
#include "hy-repo-private.h"
#include "hif-sack-private.h"
#include "hy-util.h"

#define DEFAULT_CACHE_ROOT "/var/cache/hawkey"
#define DEFAULT_CACHE_USER "/var/tmp/hawkey"

typedef struct
{
    FILE                *log_out;
    Id                   running_kernel_id;
    Map                 *pkg_excludes;
    Map                 *pkg_includes;
    Map                 *repo_excludes;
    Pool                *pool;
    Queue                installonly;
    gboolean             cmdline_repo_created;
    gboolean             considered_uptodate;
    gboolean             have_set_arch;
    gboolean             provides_ready;
    gchar               *cache_dir;
    hif_sack_running_kernel_fn_t  running_kernel_fn;
    guint                installonly_limit;
} HifSackPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(HifSack, hif_sack, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (hif_sack_get_instance_private (o))

static Map *
free_map_fully(Map *m)
{
    if (m == NULL)
        return NULL;
    map_free(m);
    g_free(m);
    return NULL;
}

/**
 * hif_sack_finalize:
 **/
static void
hif_sack_finalize(GObject *object)
{
    HifSack *sack = HIF_SACK(object);
    HifSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = priv->pool;
    Repo *repo;
    int i;

    FOR_REPOS(i, repo) {
        HyRepo hrepo = repo->appdata;
        hy_repo_free(hrepo);
    }
    if (priv->log_out) {
        g_debug("finished");
        fclose(priv->log_out);
    }
    g_free(priv->cache_dir);
    queue_free(&priv->installonly);

    free_map_fully(priv->pkg_excludes);
    free_map_fully(priv->pkg_includes);
    free_map_fully(priv->repo_excludes);
    free_map_fully(pool->considered);
    pool_free(priv->pool);

    G_OBJECT_CLASS(hif_sack_parent_class)->finalize(object);
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

/**
 * hif_sack_init:
 **/
static void
hif_sack_init(HifSack *sack)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    priv->pool = pool_create();
    priv->running_kernel_id = -1;
    priv->running_kernel_fn = running_kernel;
    priv->considered_uptodate = TRUE;
    priv->cmdline_repo_created = FALSE;
    queue_init(&priv->installonly);

    /* logging up after this*/
    pool_setdebugcallback(priv->pool, log_cb, sack);
    pool_setdebugmask(priv->pool,
                      SOLV_ERROR | SOLV_FATAL | SOLV_WARN | SOLV_DEBUG_RESULT |
                      HY_LL_INFO | HY_LL_ERROR);
}

/**
 * hif_sack_class_init:
 **/
static void
hif_sack_class_init(HifSackClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = hif_sack_finalize;
}

/**
 * hif_sack_new:
 *
 * Creates a new #HifSack.
 *
 * Returns:(transfer full): a #HifSack
 *
 * Since: 0.7.0
 **/
HifSack *
hif_sack_new(void)
{
    HifSack *sack;
    sack = g_object_new(HIF_TYPE_SACK, NULL);
    return HIF_SACK(sack);
}

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

void
hif_sack_set_running_kernel_fn (HifSack *sack, hif_sack_running_kernel_fn_t fn)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    priv->running_kernel_fn = fn;
}

/**
 * hif_sack_last_solvable: (skip)
 * @sack: a #HifSack instance.
 *
 * DOES SOMETHING.
 *
 * Returns: an #Id
 *
 * Since: 0.7.0
 */
Id
hif_sack_last_solvable(HifSack *sack)
{
    return hif_sack_get_pool(sack)->nsolvables - 1;
}

/**
 * hif_sack_recompute_considered:
 * @sack: a #HifSack instance.
 *
 * DOES SOMETHING.
 *
 * Since: 0.7.0
 */
void
hif_sack_recompute_considered(HifSack *sack)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = hif_sack_get_pool(sack);
    if (priv->considered_uptodate)
        return;
    if (!pool->considered) {
        if (!priv->repo_excludes && !priv->pkg_excludes)
            return;
        pool->considered = g_malloc0(sizeof(Map));
        map_init(pool->considered, pool->nsolvables);
    } else
        map_grow(pool->considered, pool->nsolvables);

    // considered = (all - repo_excludes - pkg_excludes) and pkg_includes
    map_setall(pool->considered);
    if (priv->repo_excludes)
        map_subtract(pool->considered, priv->repo_excludes);
    if (priv->pkg_excludes)
        map_subtract(pool->considered, priv->pkg_excludes);
    if (priv->pkg_includes)
        map_and(pool->considered, priv->pkg_includes);
    pool_createwhatprovides(priv->pool);
    priv->considered_uptodate = TRUE;
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
queue_pkg_name(HifSack *sack, Queue *queue, const char *provide, int flags)
{
    Pool *pool = hif_sack_get_pool(sack);
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
queue_provides(HifSack *sack, Queue *queue, const char *provide, int flags)
{
    Pool *pool = hif_sack_get_pool(sack);
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
queue_filter_version(HifSack *sack, Queue *queue, const char *version)
{
    Pool *pool = hif_sack_get_pool(sack);
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
load_ext(HifSack *sack, HyRepo hrepo, int which_repodata,
         const char *suffix, int which_filename,
         int (*cb)(Repo *, FILE *), GError **error)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
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

    char *fn_cache =  hif_sack_give_cache_fn(sack, name, suffix);
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
    priv->provides_ready = 0;
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
write_main(HifSack *sack, HyRepo hrepo, int switchtosolv, GError **error)
{
    Repo *repo = hrepo->libsolv_repo;
    const char *name = repo->name;
    const char *chksum = pool_checksum_str(hif_sack_get_pool(sack), hrepo->checksum);
    char *fn = hif_sack_give_cache_fn(sack, name, NULL);
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
write_ext(HifSack *sack, HyRepo hrepo, int which_repodata, const char *suffix, GError **error)
{
    Repo *repo = hrepo->libsolv_repo;
    int ret = 0;
    const char *name = repo->name;

    Id repodata = repo_get_repodata(hrepo, which_repodata);
    assert(repodata);
    Repodata *data = repo_id2repodata(repo, repodata);
    char *fn = hif_sack_give_cache_fn(sack, name, suffix);
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
load_yum_repo(HifSack *sack, HyRepo hrepo, GError **error)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    gboolean retval = TRUE;
    Pool *pool = priv->pool;
    const char *name = hy_repo_get_string(hrepo, HY_REPO_NAME);
    Repo *repo = repo_create(pool, name);
    const char *fn_repomd = hy_repo_get_string(hrepo, HY_REPO_MD_FN);
    char *fn_cache = hif_sack_give_cache_fn(sack, name, NULL);

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
        priv->provides_ready = 0;
    } else
        repo_free(repo, 1);
    return retval;
}

/**
 * hif_sack_set_cachedir:
 * @sack: a #HifSack instance.
 * @value: a a filesystem path.
 *
 * Sets the location to store the metadata cache files.
 *
 * Since: 0.7.0
 */
void
hif_sack_set_cachedir (HifSack *sack, const gchar *value)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    g_free (priv->cache_dir);
    priv->cache_dir = g_strdup(value);
}

/**
 * hif_sack_set_arch:
 * @sack: a #HifSack instance.
 * @value: an architecture, e.g. "i386", or %NULL for autodetection.
 * @error: a #GError or %NULL.
 *
 * Sets the system architecture to use for the sack. Calling this
 * function is optional before a hif_sack_setup() call.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
gboolean
hif_sack_set_arch (HifSack *sack, const gchar *value, GError **error)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = hif_sack_get_pool(sack);
    const char *arch = value;
    g_autofree gchar *detected = NULL;

    /* autodetect */
    if (arch == NULL) {
        if (hy_detect_arch(&detected)) {
            g_set_error (error,
                         HIF_ERROR,
                         HIF_ERROR_INTERNAL_ERROR,
                         "failed to auto-detect architecture");
            return FALSE;
        }
        arch = detected;
    }

    g_debug("Architecture is: %s", arch);
    pool_setarch(pool, arch);

    /* noarch never fails */
    if (!strcmp(arch, "noarch"))
        return TRUE;

    /* pool_setarch() doesn't tell us when it defaulted to 'noarch' but we
     * consider it a failure. the only way to find out is count the
     * architectures known to the Pool. */
    int count = 0;
    for (Id id = 0; id <= pool->lastarch; ++id)
        if (pool->id2arch[id])
            count++;
    if (count < 2) {
        g_set_error (error,
                     HIF_ERROR,
                     HIF_ERROR_INVALID_ARCHITECTURE,
                     "autodetection failed (noarch invalid)");
        return FALSE;
    }
    priv->have_set_arch = TRUE;
    return TRUE;
}

/**
 * hif_sack_set_rootdir:
 * @sack: a #HifSack instance.
 * @value: a directory path, or %NULL.
 *
 * Sets the install root location.
 *
 * Since: 0.7.0
 */
void
hif_sack_set_rootdir (HifSack *sack, const gchar *value)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    pool_set_rootdir(priv->pool, value);
}

/**
 * hif_sack_setup:
 * @sack: a #HifSack instance.
 * @flags: optional flags, e.g. %HIF_SACK_SETUP_FLAG_MAKE_CACHE_DIR.
 * @error: a #GError or %NULL.
 *
 * Sets up a new package sack, the fundamental hawkey structure.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
gboolean
hif_sack_setup(HifSack *sack, int flags, GError **error)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = hif_sack_get_pool(sack);

    /* we never called hif_sack_set_cachedir() */
    if (priv->cache_dir == NULL) {
        if (geteuid() != 0) {
            char *username = this_username();
            char *path = pool_tmpjoin(pool, DEFAULT_CACHE_USER, "-", username);
            path = pool_tmpappend(pool, path, "-", "XXXXXX");
            priv->cache_dir = g_strdup(path);
            g_free(username);
        } else {
            priv->cache_dir = g_strdup(DEFAULT_CACHE_ROOT);
        }
    }

    /* create the directory */
    if (flags & HIF_SACK_SETUP_FLAG_MAKE_CACHE_DIR) {
        if (mkcachedir(priv->cache_dir)) {
            g_set_error (error,
                         HIF_ERROR,
                         HIF_ERROR_FILE_INVALID,
                         "failed creating cachedir %s",
                         priv->cache_dir);
            return FALSE;
        }
    }

    /* never called hif_sack_set_arch(), so autodetect */
    if (!priv->have_set_arch) {
        if (!hif_sack_set_arch (sack, NULL, error))
            return FALSE;
    }
    return TRUE;
}

/**
 * hif_sack_evr_cmp:
 * @sack: a #HifSack instance.
 * @evr1: an EVR string.
 * @evr2: another EVR string.
 *
 * Compares one EVR with another using the sack.
 *
 * Returns: 0 for equality
 *
 * Since: 0.7.0
 */
int
hif_sack_evr_cmp(HifSack *sack, const char *evr1, const char *evr2)
{
    return pool_evrcmp_str(hif_sack_get_pool(sack), evr1, evr2, EVRCMP_COMPARE);
}

/**
 * hif_sack_get_cache_dir:
 * @sack: a #HifSack instance.
 *
 * Gets the cache directory.
 *
 * Returns: The user-set cache-dir, or %NULL for not set
 *
 * Since: 0.7.0
 */
const char *
hif_sack_get_cache_dir(HifSack *sack)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    return priv->cache_dir;
}

/**
 * hif_sack_get_running_kernel:
 * @sack: a #HifSack instance.
 *
 * Gets the running kernel.
 *
 * Returns: a #HifPackage, or %NULL
 *
 * Since: 0.7.0
 */
HifPackage *
hif_sack_get_running_kernel(HifSack *sack)
{
    Id id = hif_sack_running_kernel(sack);
    if (id < 0)
        return NULL;
    return hif_package_new(sack, id);
}

/**
 * hif_sack_give_cache_fn:
 * @sack: a #HifSack instance.
 * @reponame: a repo name.
 * @ext: a file extension
 *
 * Gets a cache filename.
 *
 * Returns: a filename
 *
 * Since: 0.7.0
 */
char *
hif_sack_give_cache_fn(HifSack *sack, const char *reponame, const char *ext)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    assert(reponame);
    char *fn = solv_dupjoin(priv->cache_dir, "/", reponame);
    if (ext)
        return solv_dupappend(fn, ext, ".solvx");
    return solv_dupappend(fn, ".solv", NULL);
}

/**
 * hif_sack_list_arches:
 * @sack: a #HifSack instance.
 *
 * DOES SOMETHING.
 *
 * Returns: a list of architectures
 *
 * Since: 0.7.0
 */
const char **
hif_sack_list_arches(HifSack *sack)
{
    Pool *pool = hif_sack_get_pool(sack);
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

/**
 * hif_sack_set_installonly:
 * @sack: a #HifSack instance.
 * @installonly: an array of package names.
 *
 * Sets the packages to use for installonlyn.
 *
 * Since: 0.7.0
 */
void
hif_sack_set_installonly(HifSack *sack, const char **installonly)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    const char *name;

    queue_empty(&priv->installonly);
    if (installonly == NULL)
        return;
    while ((name = *installonly++) != NULL)
        queue_pushunique(&priv->installonly, pool_str2id(priv->pool, name, 1));
}

/**
 * hif_sack_get_installonly: (skip)
 * @sack: a #HifSack instance.
 *
 * Gets the installonlyn packages.
 *
 * Returns: a Queue
 *
 * Since: 0.7.0
 */
Queue *
hif_sack_get_installonly(HifSack *sack)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    return &priv->installonly;
}

/**
 * hif_sack_set_installonly_limit:
 * @sack: a #HifSack instance.
 * @limit: a the number of packages.
 *
 * Sets the installonly limit.
 *
 * Since: 0.7.0
 */
void
hif_sack_set_installonly_limit(HifSack *sack, guint limit)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    priv->installonly_limit = limit;
}

/**
 * hif_sack_get_installonly_limit:
 * @sack: a #HifSack instance.
 *
 * Gets the installonly limit.
 *
 * Returns: integer value
 *
 * Since: 0.7.0
 */
guint
hif_sack_get_installonly_limit(HifSack *sack)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    return priv->installonly_limit;
}

static void
hif_sack_setup_cmdline_repo(HifSack *sack)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    if (priv->cmdline_repo_created)
        return;

    HyRepo hrepo = hy_repo_create(HY_CMDLINE_REPO_NAME);
    Repo *repo = repo_create(hif_sack_get_pool(sack), HY_CMDLINE_REPO_NAME);
    repo->appdata = hrepo;
    hrepo->libsolv_repo = repo;
    hrepo->needs_internalizing = 1;
    priv->cmdline_repo_created = TRUE;
}

/**
 * hif_sack_add_cmdline_package:
 * @sack: a #HifSack instance.
 * @fn: a filename.
 *
 * Adds the given .rpm file to the command line repo.
 *
 * Returns: a #HifPackage, or %NULL
 *
 * Since: 0.7.0
 */
HifPackage *
hif_sack_add_cmdline_package(HifSack *sack, const char *fn)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    hif_sack_setup_cmdline_repo(sack);
    Repo *repo = repo_by_name(sack, HY_CMDLINE_REPO_NAME);
    Id p;

    assert(repo);
    if (!is_readable_rpm(fn)) {
        g_warning("not a readable RPM file: %s, skipping", fn);
        return NULL;
    }
    p = repo_add_rpm(repo, fn, REPO_REUSE_REPODATA|REPO_NO_INTERNALIZE);
    priv->provides_ready = 0;    /* triggers internalizing later */
    return hif_package_new(sack, p);
}

/**
 * hif_sack_count:
 * @sack: a #HifSack instance.
 *
 * Gets the number of items in the sack.
 *
 * Returns: number of solvables.
 *
 * Since: 0.7.0
 */
int
hif_sack_count(HifSack *sack)
{
    int cnt = 0;
    Id p;
    Pool *pool = hif_sack_get_pool(sack);

    FOR_PKG_SOLVABLES(p)
        cnt++;
    return cnt;
}

/**
 * hif_sack_add_excludes:
 * @sack: a #HifSack instance.
 * @pset: a #HifPackageSet or %NULL.
 *
 * Adds excludes to the sack.
 *
 * Since: 0.7.0
 */
void
hif_sack_add_excludes(HifSack *sack, HifPackageSet *pset)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = hif_sack_get_pool(sack);
    Map *excl = priv->pkg_excludes;
    Map *nexcl = hif_packageset_get_map(pset);

    if (excl == NULL) {
        excl = g_malloc0(sizeof(Map));
        map_init(excl, pool->nsolvables);
        priv->pkg_excludes = excl;
    }
    assert(excl->size >= nexcl->size);
    map_or(excl, nexcl);
    priv->considered_uptodate = FALSE;
}

/**
 * hif_sack_add_includes:
 * @sack: a #HifSack instance.
 * @pset: a #HifPackageSet or %NULL.
 *
 * Add includes to the sack.
 *
 * Since: 0.7.0
 */
void
hif_sack_add_includes(HifSack *sack, HifPackageSet *pset)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = hif_sack_get_pool(sack);
    Map *incl = priv->pkg_includes;
    Map *nincl = hif_packageset_get_map(pset);

    if (incl == NULL) {
        incl = g_malloc0(sizeof(Map));
        map_init(incl, pool->nsolvables);
        priv->pkg_includes = incl;
    }
    assert(incl->size >= nincl->size);
    map_or(incl, nincl);
    priv->considered_uptodate = FALSE;
}

/**
 * hif_sack_set_excludes:
 * @sack: a #HifSack instance.
 * @pset: a #HifPackageSet or %NULL.
 *
 * Set excludes for the sack.
 *
 * Since: 0.7.0
 */
void
hif_sack_set_excludes(HifSack *sack, HifPackageSet *pset)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    priv->pkg_excludes = free_map_fully(priv->pkg_excludes);

    if (pset) {
        Map *nexcl = hif_packageset_get_map(pset);

        priv->pkg_excludes = g_malloc0(sizeof(Map));
        map_init_clone(priv->pkg_excludes, nexcl);
    }
    priv->considered_uptodate = FALSE;
}

/**
 * hif_sack_set_includes:
 * @sack: a #HifSack instance.
 * @pset: a #HifPackageSet or %NULL.
 *
 * Set any sack includes.
 *
 * Since: 0.7.0
 */
void
hif_sack_set_includes(HifSack *sack, HifPackageSet *pset)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    priv->pkg_includes = free_map_fully(priv->pkg_includes);

    if (pset) {
        Map *nincl = hif_packageset_get_map(pset);
        priv->pkg_includes = g_malloc0(sizeof(Map));
        map_init_clone(priv->pkg_includes, nincl);
    }
    priv->considered_uptodate = FALSE;
}

/**
 * hif_sack_repo_enabled:
 * @sack: a #HifSack instance.
 * @reponame: a repo name.
 * @enabled: the enabled state.
 *
 * DOES SOMETHING.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
int
hif_sack_repo_enabled(HifSack *sack, const char *reponame, int enabled)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = hif_sack_get_pool(sack);
    Repo *repo = repo_by_name(sack, reponame);
    Map *excl = priv->repo_excludes;

    if (repo == NULL)
        return HIF_ERROR_INTERNAL_ERROR;
    if (excl == NULL) {
        excl = g_malloc0(sizeof(Map));
        map_init(excl, pool->nsolvables);
        priv->repo_excludes = excl;
    }
    repo->disabled = !enabled;
    priv->provides_ready = 0;

    Id p;
    Solvable *s;
    if (repo->disabled)
        FOR_REPO_SOLVABLES(repo, p, s)
            MAPSET(priv->repo_excludes, p);
    else
        FOR_REPO_SOLVABLES(repo, p, s)
            MAPCLR(priv->repo_excludes, p);
    priv->considered_uptodate = FALSE;
    return 0;
}

/**
 * hif_sack_load_system_repo:
 * @sack: a #HifSack instance.
 * @a_hrepo: a rpmdb repo.
 * @flags: what to load into the sack, e.g. %HIF_SACK_LOAD_FLAG_USE_FILELISTS.
 * @error: a #GError or %NULL.
 *
 * Loads the rpmdb into the sack.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
gboolean
hif_sack_load_system_repo(HifSack *sack, HyRepo a_hrepo, int flags, GError **error)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = hif_sack_get_pool(sack);
    char *cache_fn = hif_sack_give_cache_fn(sack, HY_SYSTEM_REPO_NAME, NULL);
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
        int flagsrpm = REPO_REUSE_REPODATA | RPM_ADD_WITH_HDRID | REPO_USE_ROOTDIR;
        rc = repo_add_rpmdb_reffp(repo, cache_fp, flagsrpm);
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
    priv->provides_ready = 0;

    const int build_cache = flags & HIF_SACK_LOAD_FLAG_BUILD_CACHE;
    if (hrepo->state_main == _HY_LOADED_FETCH && build_cache) {
        ret = write_main(sack, hrepo, 1, error);
        if (!ret)
            goto finish;
    }

    hrepo->main_nsolvables = repo->nsolvables;
    hrepo->main_nrepodata = repo->nrepodata;
    hrepo->main_end = repo->end;
    priv->considered_uptodate = FALSE;

 finish:
    if (cache_fp)
        fclose(cache_fp);
    if (a_hrepo == NULL)
        hy_repo_free(hrepo);
    return ret;
}

/**
 * hif_sack_load_repo:
 * @sack: a #HifSack instance.
 * @repo: a #HyRepo.
 * @flags: what to load into the sack, e.g. %HIF_SACK_LOAD_FLAG_USE_FILELISTS.
 * @error: a #GError, or %NULL.
 *
 * Loads a remote repo into the sack.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
gboolean
hif_sack_load_repo(HifSack *sack, HyRepo repo, int flags, GError **error)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    GError *error_local = NULL;
    const int build_cache = flags & HIF_SACK_LOAD_FLAG_BUILD_CACHE;
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
    if (flags & HIF_SACK_LOAD_FLAG_USE_FILELISTS) {
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
    if (flags & HIF_SACK_LOAD_FLAG_USE_PRESTO) {
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
    if (flags & HIF_SACK_LOAD_FLAG_USE_UPDATEINFO) {
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
    priv->considered_uptodate = FALSE;
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
rewrite_repos(HifSack *sack, Queue *addedfileprovides,
              Queue *addedfileprovides_inst)
{
    Pool *pool = hif_sack_get_pool(sack);
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
        if (!(hrepo->load_flags & HIF_SACK_LOAD_FLAG_BUILD_CACHE))
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

/**
 * hif_sack_make_provides_ready:
 * @sack: a #HifSack instance.
 *
 * Gets the sack ready for depsolving.
 *
 * Since: 0.7.0
 */
void
hif_sack_make_provides_ready(HifSack *sack)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);

    if (priv->provides_ready)
        return;
    Queue addedfileprovides;
    Queue addedfileprovides_inst;
    queue_init(&addedfileprovides);
    queue_init(&addedfileprovides_inst);
    pool_addfileprovides_queue(priv->pool, &addedfileprovides,
                               &addedfileprovides_inst);
    if (addedfileprovides.count || addedfileprovides_inst.count)
        rewrite_repos(sack, &addedfileprovides, &addedfileprovides_inst);
    queue_free(&addedfileprovides);
    queue_free(&addedfileprovides_inst);
    pool_createwhatprovides(priv->pool);
    priv->provides_ready = 1;
}

/**
 * hif_sack_running_kernel: (skip)
 * @sack: a #HifSack instance.
 *
 * Returns the ID of the running kernel.
 *
 * Returns: an #Id or 0
 *
 * Since: 0.7.0
 */
Id
hif_sack_running_kernel(HifSack *sack)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    if (priv->running_kernel_id >= 0)
        return priv->running_kernel_id;
    priv->running_kernel_id = priv->running_kernel_fn(sack);
    return priv->running_kernel_id;
}

/**
 * hif_sack_knows:
 * @sack: a #HifSack instance.
 * @name: a package name.
 * @version: a package version, or %NULL.
 * @flags: options to use, e.g. %HY_NAME_ONLY.
 *
 * Checks for a package in a sack.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
int
hif_sack_knows(HifSack *sack, const char *name, const char *version, int flags)
{
    Queue *q = g_malloc(sizeof(*q));
    int ret;
    int name_only = flags & HY_NAME_ONLY;

    assert((flags & ~(HY_ICASE|HY_NAME_ONLY|HY_GLOB)) == 0);
    queue_init(q);
    hif_sack_make_provides_ready(sack);
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

/**
 * hif_sack_get_pool: (skip)
 * @sack: a #HifSack instance.
 *
 * Gets the internal pool for the sack.
 *
 * Returns: The pool, which is always set
 *
 * Since: 0.7.0
 */
Pool *
hif_sack_get_pool(HifSack *sack)
{
    HifSackPrivate *priv = GET_PRIVATE(sack);
    return priv->pool;
}

/**********************************************************************/

static void
process_excludes(HifSack *sack, HifRepo *repo)
{
    gchar **excludes = hif_repo_get_exclude_packages(repo);
    gchar **iter;
    
    if (excludes == NULL)
        return;

    for (iter = excludes; *iter; iter++) {
        const char *name = *iter;
        HyQuery query;
        HifPackageSet *pkgset;

        query = hy_query_create(sack);
        hy_query_filter_latest_per_arch(query, TRUE);
        hy_query_filter(query, HY_PKG_REPONAME, HY_EQ, hif_repo_get_id(repo));
        hy_query_filter(query, HY_PKG_ARCH, HY_NEQ, "src");
        hy_query_filter(query, HY_PKG_NAME, HY_EQ, name);
        pkgset = hy_query_run_set(query);

        hif_sack_add_excludes(sack, pkgset);

        hy_query_free(query);
        g_object_unref(pkgset);
    }
}

/**
 * hif_sack_add_repo:
 */
gboolean
hif_sack_add_repo(HifSack *sack,
                    HifRepo *repo,
                    guint permissible_cache_age,
                    HifSackAddFlags flags,
                    HifState *state,
                    GError **error)
{
    gboolean ret = TRUE;
    GError *error_local = NULL;
    HifState *state_local;
    int flags_hy = HIF_SACK_LOAD_FLAG_BUILD_CACHE;

    /* set state */
    ret = hif_state_set_steps(state, error,
                   5, /* check repo */
                   95, /* load solv */
                   -1);
    if (!ret)
        return FALSE;

    /* check repo */
    state_local = hif_state_get_child(state);
    ret = hif_repo_check(repo,
                permissible_cache_age,
                state_local,
                &error_local);
    if (!ret) {
        g_debug("failed to check, attempting update: %s",
             error_local->message);
        g_clear_error(&error_local);
        hif_state_reset(state_local);
        ret = hif_repo_update(repo,
                                HIF_REPO_UPDATE_FLAG_FORCE,
                                state_local,
                                &error_local);
        if (!ret) {
            if (!hif_repo_get_required(repo) &&
                g_error_matches(error_local,
                                HIF_ERROR,
                                HIF_ERROR_CANNOT_FETCH_SOURCE)) {
                g_warning("Skipping refresh of %s: %s",
                          hif_repo_get_id(repo),
                          error_local->message);
                g_error_free(error_local);
                return hif_state_finished(state, error);
            }
            g_propagate_error(error, error_local);
            return FALSE;
        }
    }

    /* checking disabled the repo */
    if (hif_repo_get_enabled(repo) == HIF_REPO_ENABLED_NONE) {
        g_debug("Skipping %s as repo no longer enabled",
                hif_repo_get_id(repo));
        return hif_state_finished(state, error);
    }

    /* done */
    if (!hif_state_done(state, error))
        return FALSE;

    /* only load what's required */
    if ((flags & HIF_SACK_ADD_FLAG_FILELISTS) > 0)
        flags_hy |= HIF_SACK_LOAD_FLAG_USE_FILELISTS;
    if ((flags & HIF_SACK_ADD_FLAG_UPDATEINFO) > 0)
        flags_hy |= HIF_SACK_LOAD_FLAG_USE_UPDATEINFO;

    /* load solv */
    g_debug("Loading repo %s", hif_repo_get_id(repo));
    hif_state_action_start(state, HIF_STATE_ACTION_LOADING_CACHE, NULL);
    if (!hif_sack_load_repo(sack, hif_repo_get_repo(repo), flags_hy, error))
        return FALSE;

    process_excludes(sack, repo);

    /* done */
    return hif_state_done(state, error);
}

/**
 * hif_sack_add_repos:
 */
gboolean
hif_sack_add_repos(HifSack *sack,
                     GPtrArray *repos,
                     guint permissible_cache_age,
                     HifSackAddFlags flags,
                     HifState *state,
                     GError **error)
{
    gboolean ret;
    guint cnt = 0;
    guint i;
    HifRepo *repo;
    HifState *state_local;

    /* count the enabled repos */
    for (i = 0; i < repos->len; i++) {
        repo = g_ptr_array_index(repos, i);
        if (hif_repo_get_enabled(repo) != HIF_REPO_ENABLED_NONE)
            cnt++;
    }

    /* add each repo */
    hif_state_set_number_steps(state, cnt);
    for (i = 0; i < repos->len; i++) {
        repo = g_ptr_array_index(repos, i);
        if (hif_repo_get_enabled(repo) == HIF_REPO_ENABLED_NONE)
            continue;

        /* only allow metadata-only repos if FLAG_UNAVAILABLE is set */
        if (hif_repo_get_enabled(repo) == HIF_REPO_ENABLED_METADATA) {
            if ((flags & HIF_SACK_ADD_FLAG_UNAVAILABLE) == 0)
                continue;
        }

        state_local = hif_state_get_child(state);
        ret = hif_sack_add_repo(sack,
                                  repo,
                                  permissible_cache_age,
                                  flags,
                                  state_local,
                                  error);
        if (!ret)
            return FALSE;

        /* done */
        if (!hif_state_done(state, error))
            return FALSE;
    }

    /* success */
    return TRUE;
}
