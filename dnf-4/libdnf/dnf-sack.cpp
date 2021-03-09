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
 * SECTION:dnf-sack
 * @short_description: A package sack
 * @include: libdnf.h
 * @stability: Unstable
 *
 * Sacks are repositories of packages.
 *
 * See also: #DnfContext
 */


#include <algorithm>
#include <assert.h>
#include <errno.h>
#include <functional>
#include <unistd.h>
#include <iostream>
#include <list>
#include <set>

extern "C" {
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
}

#include <cstring>
#include <sstream>

#include "catch-error.hpp"
#include "dnf-context.hpp"
#include "dnf-types.h"
#include "dnf-package.h"
#include "hy-iutil-private.hpp"
#include "hy-query.h"
#include "hy-repo-private.hpp"
#include "dnf-sack-private.hpp"
#include "hy-util.h"

#include "utils/bgettext/bgettext-lib.h"

#include "sack/query.hpp"
#include "nevra.hpp"
#include "conf/ConfigParser.hpp"
#include "conf/OptionBool.hpp"
#include "module/ModulePackageContainer.hpp"
#include "module/ModulePackage.hpp"
#include "repo/Repo-private.hpp"
#include "repo/solvable/DependencyContainer.hpp"
#include "utils/crypto/sha1.hpp"
#include "utils/File.hpp"
#include "utils/utils.hpp"
#include "log.hpp"


#define DEFAULT_CACHE_ROOT "/var/cache/hawkey"
#define DEFAULT_CACHE_USER "/var/tmp/hawkey"

typedef struct
{
    Id                   running_kernel_id;
    Map                 *pkg_excludes;
    Map                 *pkg_includes;
    Map                 *repo_excludes;
    Map                 *module_excludes;
    Map                 *module_includes;   /* To fast identify enabled modular packages */
    Map                 *pkg_solvables;     /* Map representing only solvable pkgs of query */
    int                  pool_nsolvables;   /* Number of nsolvables for creation of pkg_solvables*/
    Pool                *pool;
    Queue                installonly;
    Repo                *cmdline_repo;
    gboolean             considered_uptodate;
    gboolean             have_set_arch;
    gboolean             all_arch;
    gboolean             provides_ready;
    gchar               *cache_dir;
    char                *arch;
    dnf_sack_running_kernel_fn_t  running_kernel_fn;
    guint                installonly_limit;
    libdnf::ModulePackageContainer * moduleContainer;
} DnfSackPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(DnfSack, dnf_sack, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (static_cast<DnfSackPrivate *>(dnf_sack_get_instance_private (o)))


/**
 * dnf_sack_finalize:
 **/
static void
dnf_sack_finalize(GObject *object)
{
    DnfSack *sack = DNF_SACK(object);
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = priv->pool;
    Repo *repo;
    int i;

    FOR_REPOS(i, repo) {
        auto hrepo = static_cast<HyRepo>(repo->appdata);
        if (!hrepo)
            continue;
        libdnf::repoGetImpl(hrepo)->detachLibsolvRepo();
    }
    g_free(priv->cache_dir);
    g_free(priv->arch);
    queue_free(&priv->installonly);

    free_map_fully(priv->pkg_excludes);
    free_map_fully(priv->pkg_includes);
    free_map_fully(priv->repo_excludes);
    free_map_fully(priv->module_excludes);
    free_map_fully(priv->module_includes);
    free_map_fully(pool->considered);
    free_map_fully(priv->pkg_solvables);
    pool_free(priv->pool);
    if (priv->moduleContainer) {
        delete priv->moduleContainer;
    }

    G_OBJECT_CLASS(dnf_sack_parent_class)->finalize(object);
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
 * dnf_sack_init:
 **/
static void
dnf_sack_init(DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    priv->pool = pool_create();
    pool_set_flag(priv->pool, POOL_FLAG_WHATPROVIDESWITHDISABLED, 1);
    priv->running_kernel_id = -1;
    priv->running_kernel_fn = running_kernel;
    priv->considered_uptodate = TRUE;
    priv->cmdline_repo = NULL;
    queue_init(&priv->installonly);

    /* logging up after this*/
    pool_setdebugcallback(priv->pool, log_cb, sack);
    pool_setdebugmask(priv->pool,
                      SOLV_ERROR | SOLV_FATAL | SOLV_WARN | SOLV_DEBUG_RESULT |
                      HY_LL_INFO | HY_LL_ERROR);
}

/**
 * dnf_sack_class_init:
 **/
static void
dnf_sack_class_init(DnfSackClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_sack_finalize;
}

/**
 * dnf_sack_new:
 *
 * Creates a new #DnfSack.
 *
 * Returns:(transfer full): a #DnfSack
 *
 * Since: 0.7.0
 **/
DnfSack *
dnf_sack_new(void)
{
    return DNF_SACK(g_object_new(DNF_TYPE_SACK, NULL));
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
dnf_sack_set_running_kernel_fn (DnfSack *sack, dnf_sack_running_kernel_fn_t fn)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    priv->running_kernel_fn = fn;
}

void
dnf_sack_set_pkg_solvables(DnfSack *sack, Map *pkg_solvables, int pool_nsolvables)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);

    auto pkg_solvables_tmp = static_cast<Map *>(g_malloc(sizeof(Map)));
    if (priv->pkg_solvables)
        free_map_fully(priv->pkg_solvables);
    map_init_clone(pkg_solvables_tmp, pkg_solvables);
    priv->pkg_solvables = pkg_solvables_tmp;
    priv->pool_nsolvables = pool_nsolvables;
}

int
dnf_sack_get_pool_nsolvables(DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    return priv->pool_nsolvables;
}

libdnf::PackageSet *
dnf_sack_get_pkg_solvables(DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    return new libdnf::PackageSet(sack, priv->pkg_solvables);
}

/**
 * dnf_sack_last_solvable: (skip)
 * @sack: a #DnfSack instance.
 *
 * Returns an ID of last solvable in sack
 *
 * Returns: an #Id
 *
 * Since: 0.7.0
 */
Id
dnf_sack_last_solvable(DnfSack *sack)
{
    return dnf_sack_get_pool(sack)->nsolvables - 1;
}

void
dnf_sack_recompute_considered_map(DnfSack * sack, Map ** considered, libdnf::Query::ExcludeFlags flags)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = dnf_sack_get_pool(sack);
    if (!*considered) {
        if ((static_cast<bool>(flags & libdnf::Query::ExcludeFlags::IGNORE_REGULAR_EXCLUDES) ||
            (!priv->repo_excludes && !priv->pkg_excludes && !priv->pkg_includes))
            && (static_cast<bool>(flags & libdnf::Query::ExcludeFlags::IGNORE_MODULAR_EXCLUDES)
            || !priv->module_excludes)) {
            return;
        }
        *considered = static_cast<Map *>(g_malloc0(sizeof(Map)));
        map_init(*considered, pool->nsolvables);
    } else
        map_grow(*considered, pool->nsolvables);

    // considered = (all - repo_excludes - pkg_excludes) and
    //              (pkg_includes + all_from_repos_not_using_includes)
    map_setall(*considered);
    dnf_sack_make_provides_ready(sack);
    if (!static_cast<bool>(flags & libdnf::Query::ExcludeFlags::IGNORE_MODULAR_EXCLUDES)
        && priv->module_excludes)
        map_subtract(*considered, priv->module_excludes);
    if (!static_cast<bool>(flags & libdnf::Query::ExcludeFlags::IGNORE_REGULAR_EXCLUDES)) {
        if (priv->repo_excludes)
            map_subtract(*considered, priv->repo_excludes);
        if (priv->pkg_excludes)
            map_subtract(*considered, priv->pkg_excludes);
        if (priv->pkg_includes) {
            map_grow(priv->pkg_includes, pool->nsolvables);
            Map pkg_includes_tmp;
            map_init_clone(&pkg_includes_tmp, priv->pkg_includes);

            // Add all solvables from repositories which do not use "includes"
            Id repoid;
            Repo *repo;
            FOR_REPOS(repoid, repo) {
                auto hyrepo = static_cast<HyRepo>(repo->appdata);
                if (!hyrepo->getUseIncludes()) {
                    Id solvableid;
                    Solvable *solvable;
                    FOR_REPO_SOLVABLES(repo, solvableid, solvable)
                        MAPSET(&pkg_includes_tmp, solvableid);
                }
            }

            map_and(*considered, &pkg_includes_tmp);
            map_free(&pkg_includes_tmp);
        }
    }
}

/**
 * dnf_sack_recompute_considered:
 * @sack: a #DnfSack instance.
 *
 * Recompute considered packages for libsolv and queries.
 *
 * Since: 0.7.0
 */
void
dnf_sack_recompute_considered(DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = dnf_sack_get_pool(sack);
    if (priv->considered_uptodate)
        return;
    dnf_sack_recompute_considered_map(
        sack, &pool->considered, libdnf::Query::ExcludeFlags::APPLY_EXCLUDES);
    priv->considered_uptodate = TRUE;
}

static gboolean
load_ext(DnfSack *sack, HyRepo hrepo, _hy_repo_repodata which_repodata,
         const char *suffix, const char * which_filename,
         int (*cb)(Repo *, FILE *), GError **error)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    int ret = 0;
    auto repoImpl = libdnf::repoGetImpl(hrepo);
    Repo *repo = repoImpl->libsolvRepo;
    const char *name = repo->name;
    auto fn = hrepo->getMetadataPath(which_filename);
    FILE *fp;
    gboolean done = FALSE;

    /* nothing set */
    if (fn.empty()) {
        g_set_error (error,
                     DNF_ERROR,
                     DNF_ERROR_NO_CAPABILITY,
                     _("no %1$s string for %2$s"),
                     which_filename, name);
        return FALSE;
    }

    char *fn_cache =  dnf_sack_give_cache_fn(sack, name, suffix);
    fp = fopen(fn_cache, "r");
    assert(libdnf::repoGetImpl(hrepo)->checksum);
    if (can_use_repomd_cache(fp, libdnf::repoGetImpl(hrepo)->checksum)) {
        int flags = 0;
        /* the updateinfo is not a real extension */
        if (which_repodata != _HY_REPODATA_UPDATEINFO)
            flags |= REPO_EXTEND_SOLVABLES;
        /* do not pollute the main pool with directory component ids */
        if (which_repodata == _HY_REPODATA_FILENAMES || which_repodata == _HY_REPODATA_OTHER)
            flags |= REPO_LOCALPOOL;
        done = TRUE;
        g_debug("%s: using cache file: %s", __func__, fn_cache);
        ret = repo_add_solv(repo, fp, flags);
        if (ret) {
            g_set_error_literal (error,
                                 DNF_ERROR,
                                 DNF_ERROR_INTERNAL_ERROR,
                                 _("failed to add solv"));
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

    fp = solv_xfopen(fn.c_str(), "r");
    if (fp == NULL) {
        g_set_error (error,
                     DNF_ERROR,
                     DNF_ERROR_FILE_INVALID,
                     _("failed to open: %s"), fn.c_str());
        return FALSE;
    }
    g_debug("%s: loading: %s", __func__, fn.c_str());

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
        return DNF_ERROR_INTERNAL_ERROR;
    return 0;
}

static int
load_presto_cb(Repo *repo, FILE *fp)
{
    if (repo_add_deltainfoxml(repo, fp, 0))
        return DNF_ERROR_INTERNAL_ERROR;
    return 0;
}

static int
load_updateinfo_cb(Repo *repo, FILE *fp)
{
    if (repo_add_updateinfoxml(repo, fp, 0))
        return DNF_ERROR_INTERNAL_ERROR;
    return 0;
}

static int
load_other_cb(Repo *repo, FILE *fp)
{
    if (repo_add_rpmmd(repo, fp, 0, REPO_EXTEND_SOLVABLES))
        return DNF_ERROR_INTERNAL_ERROR;
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
write_main(DnfSack *sack, HyRepo hrepo, int switchtosolv, GError **error)
{
    auto repoImpl = libdnf::repoGetImpl(hrepo);
    Repo *repo = repoImpl->libsolvRepo;
    const char *name = repo->name;
    const char *chksum = pool_checksum_str(dnf_sack_get_pool(sack), repoImpl->checksum);
    char *fn = dnf_sack_give_cache_fn(sack, name, NULL);
    char *tmp_fn_templ = solv_dupjoin(fn, ".XXXXXX", NULL);
    int tmp_fd  = mkstemp(tmp_fn_templ);
    gboolean ret = TRUE;
    gint rc;

    g_debug("caching repo: %s (0x%s)", name, chksum);

    if (tmp_fd < 0) {
        ret = FALSE;
        g_set_error (error,
                     DNF_ERROR,
                     DNF_ERROR_FILE_INVALID,
                     _("cannot create temporary file: %s"),
                     tmp_fn_templ);
        goto done;
    } else {
        FILE *fp = fdopen(tmp_fd, "w+");
        if (!fp) {
            ret = FALSE;
            g_set_error (error,
                        DNF_ERROR,
                        DNF_ERROR_FILE_INVALID,
                        _("failed opening tmp file: %s"),
                        strerror(errno));
            goto done;
        }
        rc = repo_write(repo, fp);
        rc |= checksum_write(repoImpl->checksum, fp);
        rc |= fclose(fp);
        if (rc) {
            ret = FALSE;
            g_set_error (error,
                        DNF_ERROR,
                        DNF_ERROR_FILE_INVALID,
                        _("write_main() failed writing data: %i"), rc);
            goto done;
        }
    }
    if (switchtosolv && repo_is_one_piece(repo)) {
        /* switch over to written solv file activate paging */
        FILE *fp = fopen(tmp_fn_templ, "r");
        if (fp) {
            repo_empty(repo, 1);
            rc = repo_add_solv(repo, fp, 0);
            fclose(fp);
            if (rc) {
                /* this is pretty fatal */
                ret = FALSE;
                g_set_error_literal (error,
                                     DNF_ERROR,
                                     DNF_ERROR_FILE_INVALID,
                                     _("write_main() failed to re-load "
                                       "written solv file"));
                goto done;
            }
        }
    }

    ret = mv(tmp_fn_templ, fn, error);
    if (!ret)
        goto done;
    repoImpl->state_main = _HY_WRITTEN;

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
    auto data = static_cast<Repodata *>(kfdata);
    if (key->name == 1 && (int) key->size != data->repodataid)
        return -1;
    return repo_write_stdkeyfilter(repo, key, 0);
}

static int
write_ext_updateinfo(HyRepo hrepo, Repodata *data, FILE *fp)
{
    auto repoImpl = libdnf::repoGetImpl(hrepo);
    Repo *repo = repoImpl->libsolvRepo;
    int oldstart = repo->start;
    repo->start = repoImpl->main_end;
    repo->nsolvables -= repoImpl->main_nsolvables;
    int res = repo_write_filtered(repo, fp, write_ext_updateinfo_filter, data, 0);
    repo->start = oldstart;
    repo->nsolvables += repoImpl->main_nsolvables;
    return res;
}

static gboolean
write_ext(DnfSack *sack, HyRepo hrepo, _hy_repo_repodata which_repodata,
          const char *suffix, GError **error)
{
    auto repoImpl = libdnf::repoGetImpl(hrepo);
    Repo *repo = repoImpl->libsolvRepo;
    int ret = 0;
    const char *name = repo->name;

    Id repodata = repo_get_repodata(hrepo, which_repodata);
    assert(repodata);
    Repodata *data = repo_id2repodata(repo, repodata);
    char *fn = dnf_sack_give_cache_fn(sack, name, suffix);
    char *tmp_fn_templ = solv_dupjoin(fn, ".XXXXXX", NULL);
    int tmp_fd = mkstemp(tmp_fn_templ);
    gboolean success;
    if (tmp_fd < 0) {
        success = FALSE;
        g_set_error (error,
                     DNF_ERROR,
                     DNF_ERROR_FILE_INVALID,
                     _("can not create temporary file %s"),
                     tmp_fn_templ);
        goto done;
    } else {
        FILE *fp = fdopen(tmp_fd, "w+");

        g_debug("%s: storing %s to: %s", __func__, repo->name, tmp_fn_templ);
        if (which_repodata != _HY_REPODATA_UPDATEINFO)
            ret |= repodata_write(data, fp);
        else
            ret |= write_ext_updateinfo(hrepo, data, fp);
        ret |= checksum_write(repoImpl->checksum, fp);
        ret |= fclose(fp);
        if (ret) {
            success = FALSE;
            g_set_error (error,
                        DNF_ERROR,
                        DNF_ERROR_FAILED,
                        _("write_ext(%1$d) has failed: %2$d"),
                        which_repodata, ret);
            goto done;
        }
    }

    if (repo_is_one_piece(repo) && which_repodata != _HY_REPODATA_UPDATEINFO) {
        /* switch over to written solv file activate paging */
        FILE *fp = fopen(tmp_fn_templ, "r");
        if (fp) {
            int flags = REPO_USE_LOADING | REPO_EXTEND_SOLVABLES;
            /* do not pollute the main pool with directory component ids */
            if (which_repodata == _HY_REPODATA_FILENAMES || which_repodata == _HY_REPODATA_OTHER)
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
load_yum_repo(DnfSack *sack, HyRepo hrepo, GError **error)
{
    auto repoImpl = libdnf::repoGetImpl(hrepo);
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    gboolean retval = TRUE;
    Pool *pool = priv->pool;
    const char *name = hrepo->getId().c_str();
    Repo *repo = repo_create(pool, name);
    const char *fn_repomd = repoImpl->repomdFn.c_str();
    char *fn_cache = dnf_sack_give_cache_fn(sack, name, NULL);

    FILE *fp_primary = NULL;
    FILE *fp_repomd = NULL;
    FILE *fp_cache = fopen(fn_cache, "r");
    if (!fn_repomd) {
        g_set_error (error,
                     DNF_ERROR,
                     DNF_ERROR_FILE_INVALID,
                     _("null repo md file"));
        retval = FALSE;
        goto out;
    }
    fp_repomd = fopen(fn_repomd, "r");
    if (fp_repomd == NULL) {
        g_set_error (error,
                     DNF_ERROR,
                     DNF_ERROR_FILE_INVALID,
                     _("can not read file %1$s: %2$s"),
                     fn_repomd, strerror(errno));
        retval = FALSE;
        goto out;
    }
    checksum_fp(repoImpl->checksum, fp_repomd);

    if (can_use_repomd_cache(fp_cache, repoImpl->checksum)) {
        const char *chksum = pool_checksum_str(pool, repoImpl->checksum);
        g_debug("using cached %s (0x%s)", name, chksum);
        if (repo_add_solv(repo, fp_cache, 0)) {
            g_set_error (error,
                         DNF_ERROR,
                         DNF_ERROR_INTERNAL_ERROR,
                         _("repo_add_solv() has failed."));
            retval = FALSE;
            goto out;
        }
        repoImpl->state_main = _HY_LOADED_CACHE;
    } else {
        auto primary = hrepo->getMetadataPath(MD_TYPE_PRIMARY);
        if (primary.empty()) {
            // It could happen when repomd file has no "primary" data or they are in unsupported
            // format like zchunk
            g_set_error (error,
                         DNF_ERROR,
                         DNF_ERROR_INTERNAL_ERROR,
                         _("loading of MD_TYPE_PRIMARY has failed."));
            retval = FALSE;
            goto out;
        }
        fp_primary = solv_xfopen(primary.c_str(), "r");
        assert(fp_primary);

        g_debug("fetching %s", name);
        if (repo_add_repomdxml(repo, fp_repomd, 0) || \
            repo_add_rpmmd(repo, fp_primary, 0, 0)) {
            g_set_error (error,
                         DNF_ERROR,
                         DNF_ERROR_INTERNAL_ERROR,
                         _("repo_add_repomdxml/rpmmd() has failed."));
            retval = FALSE;
            goto out;
        }
        repoImpl->state_main = _HY_LOADED_FETCH;
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
        libdnf::repoGetImpl(hrepo)->attachLibsolvRepo(repo);
        priv->provides_ready = 0;
    } else
        repo_free(repo, 1);
    return retval;
}

/**
 * dnf_sack_set_cachedir:
 * @sack: a #DnfSack instance.
 * @value: a a filesystem path.
 *
 * Sets the location to store the metadata cache files.
 *
 * Since: 0.7.0
 */
void
dnf_sack_set_cachedir (DnfSack *sack, const gchar *value)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    g_free (priv->cache_dir);
    priv->cache_dir = g_strdup(value);
}

/**
 * dnf_sack_set_arch:
 * @sack: a #DnfSack instance.
 * @value: an architecture, e.g. "i386", or %NULL for autodetection.
 * @error: a #GError or %NULL.
 *
 * Sets the system architecture to use for the sack. Calling this
 * function is optional before a dnf_sack_setup() call.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
gboolean
dnf_sack_set_arch (DnfSack *sack, const gchar *value, GError **error) try
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = dnf_sack_get_pool(sack);
    const char *arch = value;
    g_autofree gchar *detected = NULL;

    /* autodetect */
    if (arch == NULL) {
        if (hy_detect_arch(&detected)) {
            g_set_error (error,
                         DNF_ERROR,
                         DNF_ERROR_INTERNAL_ERROR,
                         _("failed to auto-detect architecture"));
            return FALSE;
        }
        arch = detected;
    }

    g_debug("Architecture is: %s", arch);
    g_free (priv->arch);
    priv->arch = g_strdup(arch);
    pool_setdisttype(pool, DISTTYPE_RPM);
    pool_setarch(pool, arch);

    /* Since one of commits after 0.6.20 libsolv allowes custom arches
     * which means it will be 'newcoolarch' and 'noarch' always. */
    priv->have_set_arch = TRUE;
    return TRUE;
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_sack_set_all_arch:
 * @sack: a #DnfSack instance.
 * @all_arch: new value for all_arch property.
 *
 * This is used for controlling whether an arch needs to
 * be set within libsolv or not.
 *
 * Returns: Nothing.
 *
 * Since: 0.7.0
 */
void
dnf_sack_set_all_arch (DnfSack *sack, gboolean all_arch)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    priv->all_arch = all_arch;
}

/**
 * dnf_sack_get_all_arch
 * @sack: a #DnfSack instance.
 *
 * Returns: the state of all_arch.
 *
 * Since: 0.7.0
 */
gboolean
dnf_sack_get_all_arch (DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    return priv->all_arch;
}

/**
 * dnf_sack_get_arch
 * @sack: a #DnfSack instance.
 *
 * Returns: the state of all_arch.
 *
 * Since: 0.16.2
 */
const char *
dnf_sack_get_arch (DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    return priv->arch;
}

/**
 * dnf_sack_set_rootdir:
 * @sack: a #DnfSack instance.
 * @value: a directory path, or %NULL.
 *
 * Sets the install root location.
 *
 * Since: 0.7.0
 */
void
dnf_sack_set_rootdir (DnfSack *sack, const gchar *value)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    pool_set_rootdir(priv->pool, value);
    /* Don't look for running kernels if we're not operating live on
     * the current system.
     */
    if (g_strcmp0(value, "/") != 0)
        priv->running_kernel_fn = NULL;
}

/**
 * dnf_sack_setup:
 * @sack: a #DnfSack instance.
 * @flags: optional flags, e.g. %DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR.
 * @error: a #GError or %NULL.
 *
 * Sets up a new package sack, the fundamental hawkey structure.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
gboolean
dnf_sack_setup(DnfSack *sack, int flags, GError **error) try
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = dnf_sack_get_pool(sack);

    /* we never called dnf_sack_set_cachedir() */
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
    if (flags & DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR) {
        if (mkcachedir(priv->cache_dir)) {
            g_set_error (error,
                         DNF_ERROR,
                         DNF_ERROR_FILE_INVALID,
                         _("failed creating cachedir %s"),
                         priv->cache_dir);
            return FALSE;
        }
    }

    /* never called dnf_sack_set_arch(), so autodetect */
    if (!priv->have_set_arch && !priv->all_arch) {
        if (!dnf_sack_set_arch (sack, NULL, error))
            return FALSE;
    }
    return TRUE;
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_sack_evr_cmp:
 * @sack: a #DnfSack instance.
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
dnf_sack_evr_cmp(DnfSack *sack, const char *evr1, const char *evr2)
{
    g_autoptr(DnfSack) _sack = NULL;
    if (!sack)
        _sack = dnf_sack_new ();
    else
        _sack = static_cast<DnfSack *>(g_object_ref(sack));
    return pool_evrcmp_str(dnf_sack_get_pool(_sack), evr1, evr2, EVRCMP_COMPARE);
}

/**
 * dnf_sack_get_cache_dir:
 * @sack: a #DnfSack instance.
 *
 * Gets the cache directory.
 *
 * Returns: The user-set cache-dir, or %NULL for not set
 *
 * Since: 0.7.0
 */
const char *
dnf_sack_get_cache_dir(DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    return priv->cache_dir;
}

/**
 * dnf_sack_get_running_kernel:
 * @sack: a #DnfSack instance.
 *
 * Gets the running kernel.
 *
 * Returns: a #DnfPackage, or %NULL
 *
 * Since: 0.7.0
 */
DnfPackage *
dnf_sack_get_running_kernel(DnfSack *sack)
{
    Id id = dnf_sack_running_kernel(sack);
    if (id < 0)
        return NULL;
    return dnf_package_new(sack, id);
}

/**
 * dnf_sack_give_cache_fn:
 * @sack: a #DnfSack instance.
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
dnf_sack_give_cache_fn(DnfSack *sack, const char *reponame, const char *ext)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    assert(reponame);
    char *fn = solv_dupjoin(priv->cache_dir, "/", reponame);
    if (ext)
        return solv_dupappend(fn, ext, ".solvx");
    return solv_dupappend(fn, ".solv", NULL);
}

/**
 * dnf_sack_list_arches:
 * @sack: a #DnfSack instance.
 *
 * Creates list of architectures
 *
 * Returns: a list of architectures
 *
 * Since: 0.7.0
 */
const char **
dnf_sack_list_arches(DnfSack *sack)
{
    Pool *pool = dnf_sack_get_pool(sack);
    const int BLOCK_SIZE = 31;
    int c = 0;
    const char **ss = NULL;

    if (!(pool->id2arch && pool->lastarch))
        return NULL;

    for (Id id = 0; id <= pool->lastarch; ++id) {
        if (!pool->id2arch[id])
            continue;
        ss = static_cast<const char **>(solv_extend(ss, c, 1, sizeof(char*), BLOCK_SIZE));
        ss[c++] = pool_id2str(pool, id);
    }
    ss = static_cast<const char **>(solv_extend(ss, c, 1, sizeof(char*), BLOCK_SIZE));
    ss[c++] = NULL;
    return ss;
}

/**
 * dnf_sack_set_installonly_limit:
 * @sack: a #DnfSack instance.
 * @limit: a the number of packages.
 *
 * Sets the installonly limit.
 *
 * Since: 0.7.0
 */
void
dnf_sack_set_installonly_limit(DnfSack *sack, guint limit)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    priv->installonly_limit = limit;
}

/**
 * dnf_sack_get_installonly_limit:
 * @sack: a #DnfSack instance.
 *
 * Gets the installonly limit.
 *
 * Returns: integer value
 *
 * Since: 0.7.0
 */
guint
dnf_sack_get_installonly_limit(DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    return priv->installonly_limit;
}

/**
 * dnf_sack_count:
 * @sack: a #DnfSack instance.
 *
 * Gets the number of items in the sack.
 *
 * Returns: number of solvables.
 *
 * Since: 0.7.0
 */
int
dnf_sack_count(DnfSack *sack)
{
    int cnt = 0;
    Id p;
    Pool *pool = dnf_sack_get_pool(sack);

    FOR_PKG_SOLVABLES(p)
        cnt++;
    return cnt;
}

static void
dnf_sack_add_excludes_or_includes(DnfSack *sack, Map **dest, const DnfPackageSet *pkgset)
{
    Map *destmap = *dest;
    if (destmap == NULL) {
        destmap = static_cast<Map *>(g_malloc0(sizeof(Map)));
        Pool *pool = dnf_sack_get_pool(sack);
        map_init(destmap, pool->nsolvables);
        *dest = destmap;
    }

    auto pkgmap = pkgset->getMap();
    map_or(destmap, pkgmap);
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    priv->considered_uptodate = FALSE;
}

/**
 * dnf_sack_add_excludes:
 * @sack: a #DnfSack instance.
 * @pset: a #DnfPackageSet or %NULL.
 *
 * Adds excludes to the sack.
 *
 * Since: 0.7.0
 */
void
dnf_sack_add_excludes(DnfSack *sack, const DnfPackageSet *pset)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    dnf_sack_add_excludes_or_includes(sack, &priv->pkg_excludes, pset);
}

/**
 * dnf_sack_add_module_excludes:
 * @sack: a #DnfSack instance.
 * @pset: a #DnfPackageSet or %NULL.
 *
 * Adds excludes to the sack.
 *
 * Since: 0.13.4
 */
void
dnf_sack_add_module_excludes(DnfSack *sack, const DnfPackageSet *pset)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    dnf_sack_add_excludes_or_includes(sack, &priv->module_excludes, pset);
}

/**
 * dnf_sack_add_includes:
 * @sack: a #DnfSack instance.
 * @pset: a #DnfPackageSet or %NULL.
 *
 * Add includes to the sack.
 *
 * Since: 0.7.0
 */
void
dnf_sack_add_includes(DnfSack *sack, const DnfPackageSet *pset)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    dnf_sack_add_excludes_or_includes(sack, &priv->pkg_includes, pset);
}

static void
dnf_sack_remove_excludes_or_includes(DnfSack *sack, Map *from, const DnfPackageSet *pkgset)
{
    if (from == NULL)
        return;
    auto pkgmap = pkgset->getMap();
    map_subtract(from, pkgmap);
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    priv->considered_uptodate = FALSE;
}

/**
 * dnf_sack_remove_excludes:
 * @sack: a #DnfSack instance.
 * @pset: a #DnfPackageSet or %NULL.
 *
 * Removes excludes from the sack.
 *
 * Since: 0.9.4
 */
void
dnf_sack_remove_excludes(DnfSack *sack, const DnfPackageSet *pset)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    dnf_sack_remove_excludes_or_includes(sack, priv->pkg_excludes, pset);
}

/**
 * dnf_sack_remove_module_excludes:
 * @sack: a #DnfSack instance.
 * @pset: a #DnfPackageSet or %NULL.
 *
 * Removes excludes from the sack.
 *
 * Since: 0.13.4
 */
void
dnf_sack_remove_module_excludes(DnfSack *sack, const DnfPackageSet *pset)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    dnf_sack_remove_excludes_or_includes(sack, priv->module_excludes, pset);
}

/**
 * dnf_sack_remove_includes:
 * @sack: a #DnfSack instance.
 * @pset: a #DnfPackageSet or %NULL.
 *
 * Removes includes from the sack.
 *
 * Since: 0.9.4
 */
void
dnf_sack_remove_includes(DnfSack *sack, const DnfPackageSet *pset)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    dnf_sack_remove_excludes_or_includes(sack, priv->pkg_includes, pset);
}

static void
dnf_sack_set_excludes_or_includes(DnfSack *sack, Map **dest, const DnfPackageSet *pkgset)
{
    if (*dest == NULL && pkgset == NULL)
        return;

    *dest = free_map_fully(*dest);
    if (pkgset) {
        *dest = static_cast<Map *>(g_malloc0(sizeof(Map)));
        auto pkgmap = pkgset->getMap();
        map_init_clone(*dest, pkgmap);
    }
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    priv->considered_uptodate = FALSE;
}

void
dnf_sack_set_module_includes(DnfSack *sack, const DnfPackageSet *pset)
{
    if (!pset) {
        return;
    }
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    free_map_fully(priv->module_includes);
    priv->module_includes = static_cast<Map *>(g_malloc0(sizeof(Map)));
    auto pkgmap = pset->getMap();
    map_init_clone(priv->module_includes, pkgmap);
}

/**
 * dnf_sack_set_excludes:
 * @sack: a #DnfSack instance.
 * @pset: a #DnfPackageSet or %NULL.
 *
 * Set excludes for the sack.
 *
 * Since: 0.7.0
 */
void
dnf_sack_set_excludes(DnfSack *sack, const DnfPackageSet *pset)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    dnf_sack_set_excludes_or_includes(sack, &priv->pkg_excludes, pset);
}

/**
 * dnf_sack_set_module_excludes:
 * @sack: a #DnfSack instance.
 * @pset: a #DnfPackageSet or %NULL.
 *
 * Set excludes for the sack.
 *
 * Since: 0.13.4
 */
void
dnf_sack_set_module_excludes(DnfSack *sack, const DnfPackageSet *pset)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    dnf_sack_set_excludes_or_includes(sack, &priv->module_excludes, pset);
}

/**
 * dnf_sack_set_includes:
 * @sack: a #DnfSack instance.
 * @pset: a #DnfPackageSet or %NULL.
 *
 * Set any sack includes.
 *
 * Since: 0.7.0
 */
void
dnf_sack_set_includes(DnfSack *sack, const DnfPackageSet *pset)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    dnf_sack_set_excludes_or_includes(sack, &priv->pkg_includes, pset);
}

/**
 * dnf_sack_reset_excludes:
 * @sack: a #DnfSack instance.
 *
 * Reset excludes (remove excludes map from memory).
 *
 * Since: 0.9.4
 */
void
dnf_sack_reset_excludes(DnfSack *sack)
{
    dnf_sack_set_excludes(sack, NULL);
}

/**
 * dnf_sack_reset_module_excludes:
 * @sack: a #DnfSack instance.
 *
 * Reset excludes (remove excludes map from memory).
 *
 * Since: 0.13.4
 */
void
dnf_sack_reset_module_excludes(DnfSack *sack)
{
    dnf_sack_set_module_excludes(sack, NULL);
}

/**
 * dnf_sack_reset_includes:
 * @sack: a #DnfSack instance.
 *
 * Reset includes (remove includes map from memory).
 *
 * Since: 0.9.4
 */
void
dnf_sack_reset_includes(DnfSack *sack)
{
    dnf_sack_set_includes(sack, NULL);
}

/**
 * dnf_sack_get_excludes:
 * @sack: a #DnfSack instance.
 * @pset: a #DnfPackageSet or %NULL.
 *
 * Gets sack excludes.
 *
 * Since: 0.9.4
 */
DnfPackageSet *
dnf_sack_get_excludes(DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    Map *excl = priv->pkg_excludes;
    return excl ? dnf_packageset_from_bitmap(sack, excl) : NULL;
}

/**
 * dnf_sack_get_module_excludes:
 * @sack: a #DnfSack instance.
 * @pset: a #DnfPackageSet or %NULL.
 *
 * Gets sack excludes.
 *
 * Since: 0.13.4
 */
DnfPackageSet *
dnf_sack_get_module_excludes(DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    Map *excl = priv->module_excludes;
    return excl ? dnf_packageset_from_bitmap(sack, excl) : NULL;
}

/**
 * dnf_sack_get_module_includes:
 * @sack: a #DnfSack instance.
 * @pset: a #DnfPackageSet or %NULL.
 *
 * Since: 0.16.1
 */
DnfPackageSet *
dnf_sack_get_module_includes(DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    Map *excl = priv->module_includes;
    return excl ? dnf_packageset_from_bitmap(sack, excl) : NULL;
}

/**
 * dnf_sack_get_includes:
 * @sack: a #DnfSack instance.
 *
 * Gets sack includes.
 *
 * Since: 0.9.4
 */
DnfPackageSet *
dnf_sack_get_includes(DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    Map *incl = priv->pkg_includes;
    return incl ? dnf_packageset_from_bitmap(sack, incl) : NULL;
}

/**
 * dnf_sack_set_use_includes:
 * @sack: a #DnfSack instance.
 * @repo_name: a name of repo or %NULL for all repos.
 * @enabled: a use includes for a repo or all repos.
 *
 * Enable/disable usage of includes for repo/all-repos.
 *
 * Returns: FALSE if error occurred (unknown reponame) else TRUE.
 *
 * Since: 0.9.4
 */
gboolean
dnf_sack_set_use_includes(DnfSack *sack, const char *reponame, gboolean enabled)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = dnf_sack_get_pool(sack);

    if (reponame) {
        HyRepo hyrepo = hrepo_by_name(sack, reponame);
        if (!hyrepo)
            return FALSE;
        if (hyrepo->getUseIncludes() != enabled)
        {
            hyrepo->setUseIncludes(enabled);
            priv->considered_uptodate = FALSE;
        }
    } else {
        Id repoid;
        Repo *repo;
        FOR_REPOS(repoid, repo) {
            auto hyrepo = static_cast<HyRepo>(pool->repos[repoid]->appdata);
            if (hyrepo->getUseIncludes() != enabled)
            {
                hyrepo->setUseIncludes(enabled);
                priv->considered_uptodate = FALSE;
            }
        }
    }
    return TRUE;
}

/**
 * dnf_sack_set_provides_not_ready:
 * @sack: a #DnfSack instance.
 *
 * Mark sack to recompute provides
 *
 * Since: 0.16.3
 */
void
dnf_sack_set_provides_not_ready(DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    priv->provides_ready = FALSE;
}

/**
 * dnf_sack_set_considered_to_update:
 * @sack: a #DnfSack instance.
 *
 * Mark sack to recompute considered
 *
 * Since: 0.16.3
 */
void
dnf_sack_set_considered_to_update(DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    priv->considered_uptodate = FALSE;
}

/**
 * dnf_sack_get_use_includes:
 * @sack: a #DnfSack instance.
 * @repo_name: a name of repo or %NULL for all repos.
 * @enabled: a returned state of includes for repo
 *
 * Enable/disable usage of includes for repo/all-repos.
 *
 * Returns: FALSE if error occurred (unknown reponame) else TRUE.
 *
 * Since: 0.9.4
 */
gboolean
dnf_sack_get_use_includes(DnfSack *sack, const char *reponame, gboolean *enabled)
{
    assert(reponame);
    HyRepo hyrepo = hrepo_by_name(sack, reponame);
    if (!hyrepo)
        return FALSE;
    *enabled = hyrepo->getUseIncludes();
    return TRUE;
}

/**
 * dnf_sack_repo_enabled:
 * @sack: a #DnfSack instance.
 * @reponame: a repo name.
 * @enabled: the enabled state.
 *
 * Enable repository
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
int
dnf_sack_repo_enabled(DnfSack *sack, const char *reponame, int enabled)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = dnf_sack_get_pool(sack);
    Repo *repo = repo_by_name(sack, reponame);
    Map *excl = priv->repo_excludes;

    if (repo == NULL)
        return DNF_ERROR_INTERNAL_ERROR;
    if (excl == NULL) {
        excl = static_cast<Map *>(g_malloc0(sizeof(Map)));
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
 * dnf_sack_load_system_repo:
 * @sack: a #DnfSack instance.
 * @a_hrepo: a rpmdb repo.
 * @flags: what to load into the sack, e.g. %DNF_SACK_LOAD_FLAG_USE_FILELISTS.
 * @error: a #GError or %NULL.
 *
 * Loads the rpmdb into the sack.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
gboolean
dnf_sack_load_system_repo(DnfSack *sack, HyRepo a_hrepo, int flags, GError **error) try
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    Pool *pool = dnf_sack_get_pool(sack);
    char *cache_fn = dnf_sack_give_cache_fn(sack, HY_SYSTEM_REPO_NAME, NULL);
    FILE *cache_fp = fopen(cache_fn, "r");
    int rc = 1;
    gboolean ret = TRUE;
    HyRepo hrepo = a_hrepo;
    Repo *repo;
    const int build_cache = flags & DNF_SACK_LOAD_FLAG_BUILD_CACHE;

    g_free(cache_fn);
    if (hrepo) {
        auto repoImpl = libdnf::repoGetImpl(hrepo);
        repoImpl->id = HY_SYSTEM_REPO_NAME;
        repoImpl->conf->name().set(libdnf::Option::Priority::RUNTIME, HY_SYSTEM_REPO_NAME);
    } else
        hrepo = hy_repo_create(HY_SYSTEM_REPO_NAME);
    auto repoImpl = libdnf::repoGetImpl(hrepo);

    repoImpl->load_flags = flags;
    // no need to calculate anything when libsolv cache is absent
    if (cache_fp) {
        rc = current_rpmdb_checksum(pool, repoImpl->checksum);
        if (rc) {
            g_warning(_("failed calculating RPMDB checksum"));
        }
    }

    repo = repo_create(pool, HY_SYSTEM_REPO_NAME);
    if (rc == 0 && can_use_rpmdb_cache(cache_fp, repoImpl->checksum)) {
        const char *chksum = pool_checksum_str(pool, repoImpl->checksum);
        g_debug("using cached rpmdb (0x%s)", chksum);
        rc = repo_add_solv(repo, cache_fp, 0);
        if (!rc)
            repoImpl->state_main = _HY_LOADED_CACHE;
    } else {
        g_debug("fetching rpmdb");
        int flagsrpm = REPO_REUSE_REPODATA | RPM_ADD_WITH_HDRID | REPO_USE_ROOTDIR;
        rc = repo_add_rpmdb_reffp(repo, cache_fp, flagsrpm);
        if (!rc)
            repoImpl->state_main = _HY_LOADED_FETCH;
    }
    if (rc) {
        repo_free(repo, 1);
        ret = FALSE;
        g_set_error (error,
                     DNF_ERROR,
                     DNF_ERROR_FILE_INVALID,
                     _("failed loading RPMDB"));
        goto finish;
    }

    libdnf::repoGetImpl(hrepo)->attachLibsolvRepo(repo);
    pool_set_installed(pool, repo);
    priv->provides_ready = 0;

    if (repoImpl->state_main == _HY_LOADED_FETCH && build_cache) {
        ret = write_main(sack, hrepo, 1, error);
        if (!ret)
            goto finish;
    }

    repoImpl->main_nsolvables = repo->nsolvables;
    repoImpl->main_nrepodata = repo->nrepodata;
    repoImpl->main_end = repo->end;
    priv->considered_uptodate = FALSE;

 finish:
    if (cache_fp)
        fclose(cache_fp);
    if (a_hrepo == NULL)
        hy_repo_free(hrepo);
    return ret;
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_sack_load_repo:
 * @sack: a #DnfSack instance.
 * @repo: a #HyRepo.
 * @flags: what to load into the sack, e.g. %DNF_SACK_LOAD_FLAG_USE_FILELISTS.
 * @error: a #GError, or %NULL.
 *
 * Loads a remote repo into the sack.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
gboolean
dnf_sack_load_repo(DnfSack *sack, HyRepo repo, int flags, GError **error) try
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    auto repoImpl = libdnf::repoGetImpl(repo);
    GError *error_local = NULL;
    const int build_cache = flags & DNF_SACK_LOAD_FLAG_BUILD_CACHE;
    gboolean retval;
    if (!load_yum_repo(sack, repo, error))
        return FALSE;
    repoImpl->load_flags = flags;
    if (repoImpl->state_main == _HY_LOADED_FETCH && build_cache) {
        if (!write_main(sack, repo, 1, error))
            return FALSE;
    }
    repoImpl->main_nsolvables = repoImpl->libsolvRepo->nsolvables;
    repoImpl->main_nrepodata = repoImpl->libsolvRepo->nrepodata;
    repoImpl->main_end = repoImpl->libsolvRepo->end;
    if (flags & DNF_SACK_LOAD_FLAG_USE_FILELISTS) {
        retval = load_ext(sack, repo, _HY_REPODATA_FILENAMES,
                          HY_EXT_FILENAMES, MD_TYPE_FILELISTS,
                          load_filelists_cb, &error_local);
        /* allow missing files */
        if (!retval) {
            if (g_error_matches (error_local,
                                 DNF_ERROR,
                                 DNF_ERROR_NO_CAPABILITY)) {
                g_debug("no filelists metadata available for %s", repoImpl->conf->name().getValue().c_str());
                g_clear_error (&error_local);
            } else {
                g_propagate_error (error, error_local);
                return FALSE;
            }
        }
        if (repoImpl->state_filelists == _HY_LOADED_FETCH && build_cache) {
            if (!write_ext(sack, repo,
                           _HY_REPODATA_FILENAMES,
                           HY_EXT_FILENAMES, error))
                return FALSE;
        }
    }
    if (flags & DNF_SACK_LOAD_FLAG_USE_OTHER) {
        retval = load_ext(sack, repo, _HY_REPODATA_OTHER,
                          HY_EXT_OTHER, MD_TYPE_OTHER,
                          load_other_cb, &error_local);
        /* allow missing files */
        if (!retval) {
            if (g_error_matches (error_local,
                                 DNF_ERROR,
                                 DNF_ERROR_NO_CAPABILITY)) {
                g_debug("no other metadata available for %s", repoImpl->conf->name().getValue().c_str());
                g_clear_error (&error_local);
            } else {
                g_propagate_error (error, error_local);
                return FALSE;
            }
        }
        if (repoImpl->state_other == _HY_LOADED_FETCH && build_cache) {
            if (!write_ext(sack, repo,
                           _HY_REPODATA_OTHER,
                           HY_EXT_OTHER, error))
                return FALSE;
        }
    }
    if (flags & DNF_SACK_LOAD_FLAG_USE_PRESTO) {
        retval = load_ext(sack, repo, _HY_REPODATA_PRESTO,
                          HY_EXT_PRESTO, MD_TYPE_PRESTODELTA,
                          load_presto_cb, &error_local);
        if (!retval) {
            if (g_error_matches (error_local,
                                 DNF_ERROR,
                                 DNF_ERROR_NO_CAPABILITY)) {
                g_debug("no presto metadata available for %s", repoImpl->conf->name().getValue().c_str());
                g_clear_error (&error_local);
            } else {
                g_propagate_error (error, error_local);
                return FALSE;
            }
        }
        if (repoImpl->state_presto == _HY_LOADED_FETCH && build_cache)
            if (!write_ext(sack, repo, _HY_REPODATA_PRESTO, HY_EXT_PRESTO, error))
                return FALSE;
    }
    /* updateinfo must come *after* all other extensions, as it is not a real
       extension, but contains a new set of packages */
    if (flags & DNF_SACK_LOAD_FLAG_USE_UPDATEINFO) {
        retval = load_ext(sack, repo, _HY_REPODATA_UPDATEINFO,
                          HY_EXT_UPDATEINFO, MD_TYPE_UPDATEINFO,
                          load_updateinfo_cb, &error_local);
        /* allow missing files */
        if (!retval) {
            if (g_error_matches (error_local,
                                 DNF_ERROR,
                                 DNF_ERROR_NO_CAPABILITY)) {
                g_debug("no updateinfo available for %s", repoImpl->conf->name().getValue().c_str());
                g_clear_error (&error_local);
            } else {
                g_propagate_error (error, error_local);
                return FALSE;
            }
        }
        if (repoImpl->state_updateinfo == _HY_LOADED_FETCH && build_cache)
            if (!write_ext(sack, repo, _HY_REPODATA_UPDATEINFO, HY_EXT_UPDATEINFO, error))
                return FALSE;
    }
    priv->considered_uptodate = FALSE;
    return TRUE;
} CATCH_TO_GERROR(FALSE)

// internal to hawkey

/**
 * dnf_sack_running_kernel: (skip)
 * @sack: a #DnfSack instance.
 *
 * Returns the ID of the running kernel.
 *
 * Returns: an #Id or 0
 *
 * Since: 0.7.0
 */
Id
dnf_sack_running_kernel(DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    if (priv->running_kernel_id >= 0)
        return priv->running_kernel_id;
    if (priv->running_kernel_fn)
        priv->running_kernel_id = priv->running_kernel_fn(sack);
    return priv->running_kernel_id;
}

/**
 * dnf_sack_get_pool: (skip)
 * @sack: a #DnfSack instance.
 *
 * Gets the internal pool for the sack.
 *
 * Returns: The pool, which is always set
 *
 * Since: 0.7.0
 */
Pool *
dnf_sack_get_pool(DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    return priv->pool;
}

/**
 * dnf_sack_set_module_container: (skip)
 * @sack: a #DnfSack instance.
 *
 * Sets the internal ModulePackageContainer.
 *
 * Returns: The old ModulePackageContainer, that has to be freed manually.
 *
 * Since: 0.31.0
 */
libdnf::ModulePackageContainer *
dnf_sack_set_module_container(DnfSack *sack, libdnf::ModulePackageContainer * newConteiner)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    auto oldConteiner = priv->moduleContainer;
    priv->moduleContainer = newConteiner;
    return oldConteiner;
}

/**
 * dnf_sack_get_module_container: (skip)
 * @sack: a #DnfSack instance.
 *
 * Gets the internal ModulePackageContainer.
 *
 * Returns: The ModulePackageContainer, that can be not set
 *
 * Since: 0.16.3
 */
libdnf::ModulePackageContainer *
dnf_sack_get_module_container(DnfSack *sack)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    return priv->moduleContainer;
}

/**********************************************************************/

static void
process_excludes(DnfSack *sack, GPtrArray *enabled_repos)
{
    auto & mainConf = libdnf::getGlobalMainConfig();
    auto & disabled = mainConf.disable_excludes().getValue();

    if (std::find(disabled.begin(), disabled.end(), "all") != disabled.end()) {
        return;
    }

    libdnf::PackageSet repoIncludes(sack);
    libdnf::PackageSet repoExcludes(sack);
    bool includesExist = false;

    for (guint i = 0; i < enabled_repos->len; i++) {
        auto dnfRepo = static_cast<DnfRepo *>(enabled_repos->pdata[i]);
        auto repo = dnf_repo_get_repo(dnfRepo);
        if (std::find(disabled.begin(), disabled.end(), repo->getId()) != disabled.end()) {
            continue;
        }

        libdnf::Query repoQuery(sack);
        repoQuery.addFilter(HY_PKG_REPONAME, HY_EQ, repo->getId().c_str());
        repoQuery.apply();

        for (const auto & name : repo->getConfig()->includepkgs().getValue()) {
            libdnf::Query query(repoQuery);
            auto ret = query.filterSubject(name.c_str(), nullptr, false, true, false, false);
            if (ret.first) {
                repoIncludes += *query.runSet();
                includesExist = true;
                repo->setUseIncludes(true);
            }
        }

        for (const auto & name : repo->getConfig()->excludepkgs().getValue()) {
            libdnf::Query query(repoQuery);
            auto ret = query.filterSubject(name.c_str(), nullptr, false, true, false, false);
            if (ret.first) {
                repoExcludes += *query.runSet();
            }
        }
    }

    if (std::find(disabled.begin(), disabled.end(), "main") == disabled.end()) {
        bool useGlobalIncludes = false;
        for (const auto & name : mainConf.includepkgs().getValue()) {
            libdnf::Query query(sack);
            auto ret = query.filterSubject(name.c_str(), nullptr, false, true, false, false);
            if (ret.first) {
                repoIncludes += *query.runSet();
                includesExist = true;
                useGlobalIncludes = true;
            }
        }

        for (const auto & name : mainConf.excludepkgs().getValue()) {
            libdnf::Query query(sack);
            auto ret = query.filterSubject(name.c_str(), nullptr, false, true, false, false);
            if (ret.first) {
                repoExcludes += *query.runSet();
            }
        }
        
        if (useGlobalIncludes) {
            dnf_sack_set_use_includes(sack, nullptr, true);
        }
    }

    if (includesExist) {
        dnf_sack_add_includes(sack, &repoIncludes);
    }
    dnf_sack_add_excludes(sack, &repoExcludes);
}

/**
 * dnf_sack_add_repo:
 */
gboolean
dnf_sack_add_repo(DnfSack *sack,
                    DnfRepo *repo,
                    guint permissible_cache_age,
                    DnfSackAddFlags flags,
                    DnfState *state,
                    GError **error) try
{
    gboolean ret = TRUE;
    GError *error_local = NULL;
    DnfState *state_local;
    int flags_hy = DNF_SACK_LOAD_FLAG_BUILD_CACHE;

    /* set state */
    ret = dnf_state_set_steps(state, error,
                   5, /* check repo */
                   95, /* load solv */
                   -1);
    if (!ret)
        return FALSE;

    /* check repo */
    state_local = dnf_state_get_child(state);
    ret = dnf_repo_check(repo,
                         permissible_cache_age,
                         state_local,
                         &error_local);
    if (!ret) {
        g_debug("failed to check, attempting update: %s",
                error_local->message);
        g_clear_error(&error_local);
        dnf_state_reset(state_local);
        ret = dnf_repo_update(repo,
                              DNF_REPO_UPDATE_FLAG_FORCE,
                              state_local,
                              &error_local);
        if (!ret) {
            if (!dnf_repo_get_required(repo) &&
                (g_error_matches(error_local,
                                 DNF_ERROR,
                                 DNF_ERROR_CANNOT_FETCH_SOURCE) ||
                 g_error_matches(error_local,
                                 DNF_ERROR,
                                 DNF_ERROR_REPO_NOT_AVAILABLE))) {
                g_warning("Skipping refresh of %s: %s",
                          dnf_repo_get_id(repo),
                          error_local->message);
                g_error_free(error_local);
                return dnf_state_finished(state, error);
            }
            g_propagate_error(error, error_local);
            return FALSE;
        }
    }

    /* checking disabled the repo */
    if (dnf_repo_get_enabled(repo) == DNF_REPO_ENABLED_NONE) {
        g_debug("Skipping %s as repo no longer enabled",
                dnf_repo_get_id(repo));
        return dnf_state_finished(state, error);
    }

    /* done */
    if (!dnf_state_done(state, error))
        return FALSE;

    /* only load what's required */
    if ((flags & DNF_SACK_ADD_FLAG_FILELISTS) > 0)
        flags_hy |= DNF_SACK_LOAD_FLAG_USE_FILELISTS;
    if ((flags & DNF_SACK_ADD_FLAG_OTHER) > 0)
        flags_hy |= DNF_SACK_LOAD_FLAG_USE_OTHER;
    if ((flags & DNF_SACK_ADD_FLAG_UPDATEINFO) > 0)
        flags_hy |= DNF_SACK_LOAD_FLAG_USE_UPDATEINFO;

    /* load solv */
    g_debug("Loading repo %s", dnf_repo_get_id(repo));
    dnf_state_action_start(state, DNF_STATE_ACTION_LOADING_CACHE, NULL);
    if (!dnf_sack_load_repo(sack, dnf_repo_get_repo(repo), flags_hy, error))
        return FALSE;

    /* done */
    return dnf_state_done(state, error);
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_sack_add_repos:
 */
gboolean
dnf_sack_add_repos(DnfSack *sack,
                     GPtrArray *repos,
                     guint permissible_cache_age,
                     DnfSackAddFlags flags,
                     DnfState *state,
                     GError **error) try
{
    gboolean ret;
    guint cnt = 0;
    guint i;
    DnfRepo *repo;
    DnfState *state_local;
    g_autoptr(GPtrArray) enabled_repos = g_ptr_array_new();

    /* count the enabled repos */
    for (i = 0; i < repos->len; i++) {
        repo = static_cast<DnfRepo *>(g_ptr_array_index(repos, i));
        if (dnf_repo_get_enabled(repo) == DNF_REPO_ENABLED_NONE)
            continue;

        /* only allow metadata-only repos if FLAG_UNAVAILABLE is set */
        if (dnf_repo_get_enabled(repo) == DNF_REPO_ENABLED_METADATA) {
            if ((flags & DNF_SACK_ADD_FLAG_UNAVAILABLE) == 0)
                continue;
        }

        cnt++;
    }

    /* add each repo */
    dnf_state_set_number_steps(state, cnt);
    for (i = 0; i < repos->len; i++) {
        repo = static_cast<DnfRepo *>(g_ptr_array_index(repos, i));
        if (dnf_repo_get_enabled(repo) == DNF_REPO_ENABLED_NONE)
            continue;

        /* only allow metadata-only repos if FLAG_UNAVAILABLE is set */
        if (dnf_repo_get_enabled(repo) == DNF_REPO_ENABLED_METADATA) {
            if ((flags & DNF_SACK_ADD_FLAG_UNAVAILABLE) == 0)
                continue;
        }

        state_local = dnf_state_get_child(state);
        ret = dnf_sack_add_repo(sack,
                                  repo,
                                  permissible_cache_age,
                                  flags,
                                  state_local,
                                  error);
        if (!ret)
            return FALSE;

        g_ptr_array_add(enabled_repos, repo);

        /* done */
        if (!dnf_state_done(state, error))
            return FALSE;
    }

    process_excludes(sack, enabled_repos);

    /* success */
    return TRUE;
} CATCH_TO_GERROR(FALSE)

namespace {
void readModuleMetadataFromRepo(DnfSack * sack, libdnf::ModulePackageContainer * modulePackages,
    const char * platformModule)
{
    modulePackages->add(sack);
    modulePackages->loadFailSafeData();
    if (!modulePackages->empty()) {
        modulePackages->createConflictsBetweenStreams();
        // TODO remove hard-coded path
        try {
            std::vector<std::string> paths{"/etc/os-release", "/usr/lib/os-release"};
            modulePackages->addPlatformPackage(sack, paths, platformModule);
        } catch (const std::exception & except) {
            auto logger(libdnf::Log::getLogger());
            logger->critical("Detection of Platform Module failed: " + std::string(except.what()));
        }
    }
}

static std::tuple<std::vector<std::string>, std::vector<std::string>>
collectNevraForInclusionExclusion(libdnf::ModulePackageContainer &modulePackageContainer)
{
    std::vector<std::string> includeNEVRAs;
    std::vector<std::string> excludeNEVRAs;
    // TODO: turn into std::vector<const char *> to prevent unnecessary conversion?
    for (const auto &module : modulePackageContainer.getModulePackages()) {
        auto artifacts = module->getArtifacts();
        // TODO use Goal::listInstalls() to not requires filtering out Platform
        if (modulePackageContainer.isModuleActive(module->getId())) {
            copy(std::begin(artifacts), std::end(artifacts), std::back_inserter(includeNEVRAs));
        } else {
            copy(std::begin(artifacts), std::end(artifacts), std::back_inserter(excludeNEVRAs));
        }
    }

    return std::tuple<std::vector<std::string>,
    std::vector<std::string>>{includeNEVRAs, excludeNEVRAs};
}

static void
setModuleExcludes(DnfSack *sack, const char ** hotfixRepos,
    const std::vector<std::string> &includeNEVRAs, const std::vector<std::string> &excludeNEVRAs)
{
    dnf_sack_set_module_excludes(sack, nullptr);
    std::vector<std::string> names;
    libdnf::DependencyContainer nameDependencies{sack};
    libdnf::Nevra nevra;
    for (const auto &rpm : includeNEVRAs) {
        if (nevra.parse(rpm.c_str(), HY_FORM_NEVRA)) {
            names.push_back(nevra.getName());
            nameDependencies.addReldep(nevra.getName().c_str());
        }
    }

    std::vector<const char *> namesCString(names.size() + 1);
    std::vector<const char *> excludeNEVRAsCString(excludeNEVRAs.size() + 1);
    std::vector<const char *> includeNEVRAsCString(includeNEVRAs.size() + 1);

    transform(names.begin(), names.end(), namesCString.begin(), std::mem_fn(&std::string::c_str));
    transform(excludeNEVRAs.begin(), excludeNEVRAs.end(), excludeNEVRAsCString.begin(),
              std::mem_fn(&std::string::c_str));
    transform(includeNEVRAs.begin(), includeNEVRAs.end(), includeNEVRAsCString.begin(),
              std::mem_fn(&std::string::c_str));

    libdnf::Query keepPackages{sack};
    const char *keepRepo[] = {HY_CMDLINE_REPO_NAME, HY_SYSTEM_REPO_NAME, nullptr};
    keepPackages.addFilter(HY_PKG_REPONAME, HY_NEQ, keepRepo);
    keepPackages.addFilter(HY_PKG_REPONAME, HY_NEQ, hotfixRepos);

    libdnf::Query includeQuery{sack};
    libdnf::Query excludeQuery{keepPackages};
    libdnf::Query excludeProvidesQuery{keepPackages};
    libdnf::Query excludeNamesQuery(keepPackages);
    includeQuery.addFilter(HY_PKG_NEVRA_STRICT, HY_EQ, includeNEVRAsCString.data());

    excludeQuery.addFilter(HY_PKG_NEVRA_STRICT, HY_EQ, excludeNEVRAsCString.data());
    excludeQuery.queryDifference(includeQuery);

    // Exclude packages by their Provides
    excludeProvidesQuery.addFilter(HY_PKG_PROVIDES, &nameDependencies);
    excludeProvidesQuery.queryDifference(includeQuery);

    // Requred to filtrate out source packages and packages with incompatible architectures
    excludeNamesQuery.addFilter(HY_PKG_NAME, HY_EQ, namesCString.data());
    excludeNamesQuery.queryDifference(includeQuery);

    dnf_sack_set_module_excludes(sack, excludeQuery.getResultPset());
    dnf_sack_add_module_excludes(sack, excludeProvidesQuery.getResultPset());
    dnf_sack_add_module_excludes(sack, excludeNamesQuery.getResultPset());
    dnf_sack_set_module_includes(sack, includeQuery.getResultPset());
}

}

std::vector<libdnf::ModulePackage *> requiresModuleEnablement(
    DnfSack * sack, const libdnf::PackageSet * installSet)
{
    DnfSackPrivate *priv = GET_PRIVATE(sack);
    if (!priv->module_includes || !priv->moduleContainer) {
        return {};
    }
    libdnf::PackageSet tmp(sack);
    tmp += *const_cast<libdnf::PackageSet*>(installSet);
    tmp /= priv->module_includes;

    if (tmp.empty()) {
        return {};
    }
    auto toEnable = priv->moduleContainer->requiresModuleEnablement(*installSet);
    return toEnable;
}

void dnf_sack_filter_modules(DnfSack *sack, GPtrArray *repos, const char *install_root,
    const char * platformModule)
{
    std::vector<const char *> hotfixRepos;
    // don't filter RPMs from repos with the 'module_hotfixes' flag set
    for (unsigned int i = 0; i < repos->len; i++) {
        auto repo = static_cast<DnfRepo *>(g_ptr_array_index(repos, i));
        if (dnf_repo_get_module_hotfixes(repo)) {
            hotfixRepos.push_back(dnf_repo_get_id(repo));
        }
    }
    hotfixRepos.push_back(nullptr);
    dnf_sack_filter_modules_v2(sack, nullptr, hotfixRepos.data(), install_root, platformModule,
                               false, false);
}

std::pair<std::vector<std::vector<std::string>>, libdnf::ModulePackageContainer::ModuleErrorType> dnf_sack_filter_modules_v2(
    DnfSack * sack, DnfModulePackageContainer * moduleContainer, const char ** hotfixRepos,
    const char * install_root, const char * platformModule, bool updateOnly, bool debugSolver)
{
    if (!updateOnly) {
        if (!install_root) {
            throw std::runtime_error("Installroot not provided");
        }
        DnfSackPrivate *priv = GET_PRIVATE(sack);
        if (!moduleContainer) {
            if (priv->moduleContainer) {
                delete priv->moduleContainer;
            }
            priv->moduleContainer = new libdnf::ModulePackageContainer(dnf_sack_get_all_arch(sack),
                install_root, dnf_sack_get_arch(sack));
            moduleContainer = priv->moduleContainer;
        }
        readModuleMetadataFromRepo(sack, moduleContainer, platformModule);
        moduleContainer->addDefaultsFromDisk();

        try {
            moduleContainer->moduleDefaultsResolve();
        } catch (libdnf::ModulePackageContainer::ResolveException & exception) {
            auto logger(libdnf::Log::getLogger());
            logger->debug(_("No module defaults found"));
        }
    }

    if (!moduleContainer) {
        throw std::runtime_error("ModuleContainer not provided");
    }
    auto ret = moduleContainer->resolveActiveModulePackages(debugSolver);
    auto nevraTuple = collectNevraForInclusionExclusion(*moduleContainer);

    setModuleExcludes(sack, hotfixRepos, std::get<0>(nevraTuple), std::get<1>(nevraTuple));
    return ret;
}

std::string dnf_sack_get_rpmdb_version(DnfSack *sack) {
    // collect all sha1hdr checksums
    // they are sufficiently unique IDs that represent installed RPMs
    std::vector<std::string> checksums;

    // iterate all @System repo RPMs (rpmdb records)
    libdnf::Query query{sack, libdnf::Query::ExcludeFlags::IGNORE_EXCLUDES};
    query.addFilter(HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);

    auto pset = query.getResultPset();
    Id id = -1;
    while(true) {
        id = pset->next(id);
        if (id == -1) {
            break;
        }
        DnfPackage *pkg = dnf_package_new(sack, id);
        // store pkgid (equals to sha1hdr)
        checksums.push_back(libdnf::string::fromCstring(dnf_package_get_pkgid(pkg)));
        g_object_unref(pkg);
    }

    // sort checksums to compute the output checksum always the same
    std::sort(checksums.begin(), checksums.end());

    SHA1Hash h;
    for (auto & checksum : checksums) {
        h.update(checksum.c_str());
    }

    // build <count>:<hash> output
    std::ostringstream result;
    result << checksums.size();
    result << ":";
    result << h.hexdigest();

    return result.str();
}
