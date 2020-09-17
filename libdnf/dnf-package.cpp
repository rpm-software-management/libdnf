/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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
 * SECTION:dnf-goal
 * @short_description: Helper methods for dealing with hawkey packages.
 * @include: libdnf.h
 * @stability: Unstable
 *
 * These methods make it easier to get and set extra data on a package.
 */


#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <assert.h>

#include <librepo/librepo.h>
#include <memory>

#include "catch-error.hpp"
#include "dnf-package.h"
#include "dnf-types.h"
#include "dnf-utils.h"
#include "hy-util.h"
#include "repo/solvable/Dependency.hpp"
#include "repo/solvable/DependencyContainer.hpp"
#include "utils/url-encode.hpp"

typedef struct {
    char            *checksum_str;
    gboolean         user_action;
    gchar           *filename;
    gchar           *origin;
    gchar           *package_id;
    DnfPackageInfo   info;
    DnfStateAction   action;
    DnfRepo         *repo;
} DnfPackagePrivate;

/**
 * dnf_package_destroy_func:
 **/
static void
dnf_package_destroy_func(void *userdata)
{
    DnfPackagePrivate *priv =(DnfPackagePrivate *) userdata;
    g_free(priv->filename);
    g_free(priv->origin);
    g_free(priv->package_id);
    g_free(priv->checksum_str);
    g_slice_free(DnfPackagePrivate, priv);
}

/**
 * dnf_package_get_priv:
 **/
static DnfPackagePrivate *
dnf_package_get_priv(DnfPackage *pkg)
{
    DnfPackagePrivate *priv;

    /* create private area */
    priv = (DnfPackagePrivate*)g_object_get_data(G_OBJECT(pkg), "DnfPackagePrivate");
    if (priv != NULL)
        return priv;

    priv = g_slice_new0(DnfPackagePrivate);
    g_object_set_data_full(G_OBJECT(pkg), "DnfPackagePrivate", priv, dnf_package_destroy_func);
    return priv;
}

/**
 * dnf_package_is_local:
 * @pkg: a #DnfPackage *instance.
 *
 * Returns: %TRUE if the pkg is a pkg on local or media filesystem
 *
 * Since: 0.38.2
 **/
gboolean
dnf_package_is_local(DnfPackage *pkg)
{
    DnfPackagePrivate *priv;
    priv = dnf_package_get_priv(pkg);

    assert(priv->repo);

    if (!dnf_repo_is_local(priv->repo))
        return FALSE;
     
    const gchar *url_location = dnf_package_get_baseurl(pkg);
    return (!url_location  || (url_location && g_str_has_prefix(url_location, "file:/")));
}

/**
 * dnf_package_get_local_baseurl:
 * @pkg: a #DnfPackage *instance.
 *
 * Returns package baseurl converted to a local filesystem path (i.e. "file://"
 * is stripped and the URL is decoded). In case the URL is not local, returns
 * %NULL.
 *
 * The returned string is newly allocated and ownership is transferred to the
 * caller.
 *
 * Returns: local filesystem baseurl or %NULL
 *
 * Since: 0.54.0
 **/
gchar *
dnf_package_get_local_baseurl(DnfPackage *pkg, GError **error)
{
    const gchar *baseurl = dnf_package_get_baseurl(pkg);

    if (!baseurl || !g_str_has_prefix(baseurl, "file://")) {
        return nullptr;
    }

    return g_strdup(libdnf::urlDecode(baseurl + 7).c_str());
}

/**
 * dnf_package_get_filename:
 * @pkg: a #DnfPackage *instance.
 *
 * Gets the package filename.
 *
 * Returns: absolute filename, or %NULL
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_package_get_filename(DnfPackage *pkg)
{
    DnfPackagePrivate *priv;

    priv = dnf_package_get_priv(pkg);
    if (!priv)
        return NULL;
    if (dnf_package_installed(pkg))
        return NULL;
    if (!priv->filename && !priv->repo)
        return NULL;

    /* default cache filename location */
    if (!priv->filename) {
        if (dnf_package_is_local(pkg)) {
            const gchar *url_location = dnf_package_get_baseurl(pkg);
            if (!url_location){
                url_location = dnf_repo_get_location(priv->repo);
            }
            priv->filename = g_build_filename(url_location,
                                              dnf_package_get_location(pkg),
                                              NULL);
        } else {
            /* set the filename to cachedir for non-local repos */
            g_autofree gchar *basename = NULL;
            basename = g_path_get_basename(dnf_package_get_location(pkg));
            priv->filename = g_build_filename(dnf_repo_get_packages(priv->repo),
                               basename,
                               NULL);
        }
        g_assert (priv->filename); /* Pacify static analysis */
    }

    /* remove file:// from filename */
    if (g_str_has_prefix(priv->filename, "file:///")){
        gchar *tmp = priv->filename;
        priv->filename = g_strdup(tmp + 7);
        g_free(tmp);
        goto out;
    }

    /* remove file: from filename */
    if (strlen(priv->filename) >= 7){
        if (g_str_has_prefix(priv->filename, "file:/")){
            if (priv->filename[6] != '/'){
                gchar *tmp = priv->filename;
                priv->filename = g_strdup(tmp + 5);
                g_free(tmp);
            }
        }
    }
out:
    return priv->filename;
}

/**
 * dnf_package_get_origin:
 * @pkg: a #DnfPackage *instance.
 *
 * Gets the package origin.
 *
 * Returns: the package origin, or %NULL
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_package_get_origin(DnfPackage *pkg)
{
    DnfPackagePrivate *priv;
    priv = dnf_package_get_priv(pkg);
    if (priv == NULL)
        return NULL;
    if (!dnf_package_installed(pkg))
        return NULL;
    return priv->origin;
}

/**
 * dnf_package_get_pkgid:
 * @pkg: a #DnfPackage *instance.
 *
 * Gets the pkgid, which is the SHA hash of the package header.
 *
 * Returns: pkgid string, or NULL
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_package_get_pkgid(DnfPackage *pkg)
{
    const unsigned char *checksum;
    DnfPackagePrivate *priv;
    int checksum_type;

    priv = dnf_package_get_priv(pkg);
    if (priv == NULL)
        return NULL;
    if (priv->checksum_str != NULL)
        goto out;

    /* calculate and cache */
    checksum = dnf_package_get_hdr_chksum(pkg, &checksum_type);
    if (checksum == NULL)
        goto out;
    priv->checksum_str = hy_chksum_str(checksum, checksum_type);
out:
    return priv->checksum_str;
}

/**
 * dnf_package_set_pkgid:
 * @pkg: a #DnfPackage *instance.
 * @pkgid: pkgid, e.g. "e6e3b2b10c1ef1033769147dbd1bf851c7de7699"
 *
 * Sets the package pkgid, which is the SHA hash of the package header.
 *
 * Since: 0.1.8
 **/
void
dnf_package_set_pkgid(DnfPackage *pkg, const gchar *pkgid)
{
    DnfPackagePrivate *priv;
    g_return_if_fail(pkgid != NULL);
    priv = dnf_package_get_priv(pkg);
    if (priv == NULL)
        return;
    g_free(priv->checksum_str);
    priv->checksum_str = strdup(pkgid);
}

/**
 * dnf_package_id_build:
 **/
static gchar *
dnf_package_id_build(const gchar *name,
              const gchar *version,
              const gchar *arch,
              const gchar *data)
{
    return g_strjoin(";", name,
              version != NULL ? version : "",
              arch != NULL ? arch : "",
              data != NULL ? data : "",
              NULL);
}

/**
 * dnf_package_get_package_id:
 * @pkg: a #DnfPackage *instance.
 *
 * Gets the package-id as used by PackageKit.
 *
 * Returns: the package_id string, or %NULL, e.g. "hal;2:0.3.4;i386;installed:fedora"
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_package_get_package_id(DnfPackage *pkg)
{
    DnfPackagePrivate *priv;
    const gchar *reponame;
    g_autofree gchar *reponame_tmp = NULL;

    priv = dnf_package_get_priv(pkg);
    if (priv == NULL)
        return NULL;
    if (priv->package_id != NULL)
        goto out;

    /* calculate and cache */
    reponame = dnf_package_get_reponame(pkg);
    if (g_strcmp0(reponame, HY_SYSTEM_REPO_NAME) == 0) {
        /* origin data to munge into the package_id data field */
        if (priv->origin != NULL) {
            reponame_tmp = g_strdup_printf("installed:%s", priv->origin);
            reponame = reponame_tmp;
        } else {
            reponame = "installed";
        }
    } else if (g_strcmp0(reponame, HY_CMDLINE_REPO_NAME) == 0) {
        reponame = "local";
    }
    priv->package_id = dnf_package_id_build(dnf_package_get_name(pkg),
                        dnf_package_get_evr(pkg),
                        dnf_package_get_arch(pkg),
                        reponame);
out:
    return priv->package_id;
}

/**
 * dnf_package_get_cost:
 * @pkg: a #DnfPackage *instance.
 *
 * Returns the cost of the repo that provided the package.
 *
 * Returns: the cost, where higher is more expensive, default 1000
 *
 * Since: 0.1.0
 **/
guint
dnf_package_get_cost(DnfPackage *pkg)
{
    DnfPackagePrivate *priv;
    priv = dnf_package_get_priv(pkg);
    if (priv->repo == NULL) {
        g_warning("no repo for %s", dnf_package_get_package_id(pkg));
        return G_MAXUINT;
    }
    return dnf_repo_get_cost(priv->repo);
}

/**
 * dnf_package_set_filename:
 * @pkg: a #DnfPackage *instance.
 * @filename: absolute filename.
 *
 * Sets the file on disk that matches the package repo.
 *
 * Since: 0.1.0
 **/
void
dnf_package_set_filename(DnfPackage *pkg, const gchar *filename)
{
    DnfPackagePrivate *priv;

    /* replace contents */
    priv = dnf_package_get_priv(pkg);
    if (priv == NULL)
        return;
    g_free(priv->filename);
    priv->filename = g_strdup(filename);
}

/**
 * dnf_package_set_origin:
 * @pkg: a #DnfPackage *instance.
 * @origin: origin, e.g. "fedora"
 *
 * Sets the package origin repo.
 *
 * Since: 0.1.0
 **/
void
dnf_package_set_origin(DnfPackage *pkg, const gchar *origin)
{
    DnfPackagePrivate *priv;
    priv = dnf_package_get_priv(pkg);
    if (priv == NULL)
        return;
    g_free(priv->origin);
    priv->origin = g_strdup(origin);
}

/**
 * dnf_package_set_repo:
 * @pkg: a #DnfPackage *instance.
 * @repo: a #DnfRepo.
 *
 * Sets the repo the package was created from.
 *
 * Since: 0.1.0
 **/
void
dnf_package_set_repo(DnfPackage *pkg, DnfRepo *repo)
{
    DnfPackagePrivate *priv;
    priv = dnf_package_get_priv(pkg);
    if (priv == NULL)
        return;
    priv->repo = repo;
}

/**
 * dnf_package_get_repo:
 * @pkg: a #DnfPackage *instance.
 *
 * Gets the repo the package was created from.
 *
 * Returns: a #DnfRepo or %NULL
 *
 * Since: 0.1.0
 **/
DnfRepo *
dnf_package_get_repo(DnfPackage *pkg)
{
    DnfPackagePrivate *priv;
    priv = dnf_package_get_priv(pkg);
    if (priv == NULL)
        return NULL;
    return priv->repo;
}

/**
 * dnf_package_get_info:
 * @pkg: a #DnfPackage *instance.
 *
 * Gets the info enum assigned to the package.
 *
 * Returns: #DnfPackageInfo value
 *
 * Since: 0.1.0
 **/
DnfPackageInfo
dnf_package_get_info(DnfPackage *pkg)
{
    DnfPackagePrivate *priv;
    priv = dnf_package_get_priv(pkg);
    if (priv == NULL)
        return DNF_PACKAGE_INFO_UNKNOWN;
    return priv->info;
}

/**
 * dnf_package_get_action:
 * @pkg: a #DnfPackage *instance.
 *
 * Gets the action assigned to the package, i.e. what is going to be performed.
 *
 * Returns: a #DnfStateAction
 *
 * Since: 0.1.0
 */
DnfStateAction
dnf_package_get_action(DnfPackage *pkg)
{
    DnfPackagePrivate *priv;
    priv = dnf_package_get_priv(pkg);
    if (priv == NULL)
        return DNF_STATE_ACTION_UNKNOWN;
    return priv->action;
}

/**
 * dnf_package_set_info:
 * @pkg: a #DnfPackage *instance.
 * @info: the info flags.
 *
 * Sets the info flags for the package.
 *
 * Since: 0.1.0
 **/
void
dnf_package_set_info(DnfPackage *pkg, DnfPackageInfo info)
{
    DnfPackagePrivate *priv;
    priv = dnf_package_get_priv(pkg);
    if (priv == NULL)
        return;
    priv->info = info;
}

/**
 * dnf_package_set_action:
 * @pkg: a #DnfPackage *instance.
 * @action: the #DnfStateAction for the package.
 *
 * Sets the action for the package, i.e. what is going to be performed.
 *
 * Since: 0.1.0
 */
void
dnf_package_set_action(DnfPackage *pkg, DnfStateAction action)
{
    DnfPackagePrivate *priv;
    priv = dnf_package_get_priv(pkg);
    if (priv == NULL)
        return;
    priv->action = action;
}

/**
 * dnf_package_get_user_action:
 * @pkg: a #DnfPackage *instance.
 *
 * Gets if the package was installed or removed as the user action.
 *
 * Returns: %TRUE if the package was explicitly requested
 *
 * Since: 0.1.0
 **/
gboolean
dnf_package_get_user_action(DnfPackage *pkg)
{
    DnfPackagePrivate *priv;
    priv = dnf_package_get_priv(pkg);
    if (priv == NULL)
        return FALSE;
    return priv->user_action;
}

/**
 * dnf_package_set_user_action:
 * @pkg: a #DnfPackage *instance.
 * @user_action: %TRUE if the package was explicitly requested.
 *
 * Sets if the package was installed or removed as the user action.
 *
 * Since: 0.1.0
 **/
void
dnf_package_set_user_action(DnfPackage *pkg, gboolean user_action)
{
    DnfPackagePrivate *priv;
    priv = dnf_package_get_priv(pkg);
    if (priv == NULL)
        return;
    priv->user_action = user_action;
}

/**
 * dnf_package_is_gui:
 * @pkg: a #DnfPackage *instance.
 *
 * Returns: %TRUE if the package is a GUI package
 *
 * Since: 0.1.0
 **/
gboolean
dnf_package_is_gui(DnfPackage *pkg)
{
    gboolean ret = FALSE;
    const gchar *tmp;
    gint idx;
    gint size;

    /* find if the package depends on GTK or KDE */
    std::unique_ptr<DnfReldepList> reldep_list(dnf_package_get_requires(pkg));
    size = reldep_list->count();
    for (idx = 0; idx < size && !ret; idx++) {
        auto reldep = reldep_list->get(idx);
        tmp = reldep->toString();
        if (g_strstr_len(tmp, -1, "libgtk") != NULL ||
            g_strstr_len(tmp, -1, "libQt5Gui.so") != NULL ||
            g_strstr_len(tmp, -1, "libQtGui.so") != NULL ||
            g_strstr_len(tmp, -1, "libqt-mt.so") != NULL) {
            ret = TRUE;
        }
    }

    return ret;
}

/**
 * dnf_package_is_devel:
 * @pkg: a #DnfPackage *instance.
 *
 * Returns: %TRUE if the package is a development package
 *
 * Since: 0.1.0
 **/
gboolean
dnf_package_is_devel(DnfPackage *pkg)
{
    const gchar *name;
    name = dnf_package_get_name(pkg);
    if (g_str_has_suffix(name, "-debuginfo"))
        return TRUE;
    if (g_str_has_suffix(name, "-devel"))
        return TRUE;
    if (g_str_has_suffix(name, "-static"))
        return TRUE;
    if (g_str_has_suffix(name, "-libs"))
        return TRUE;
    return FALSE;
}

/**
 * dnf_package_is_downloaded:
 * @pkg: a #DnfPackage *instance.
 *
 * Returns: %TRUE if the package is already downloaded
 *
 * Since: 0.1.0
 **/
gboolean
dnf_package_is_downloaded(DnfPackage *pkg)
{
    const gchar *filename;

    if (dnf_package_installed(pkg))
        return FALSE;
    filename = dnf_package_get_filename(pkg);
    if (filename == NULL) {
        g_warning("Failed to get cache filename for %s",
               dnf_package_get_name(pkg));
        return FALSE;
    }
    return g_file_test(filename, G_FILE_TEST_EXISTS);
}

/**
 * dnf_package_is_installonly:
 * @pkg: a #DnfPackage *instance.
 *
 * Returns: %TRUE if the package can be installed more than once
 *
 * Since: 0.1.0
 */
gboolean
dnf_package_is_installonly(DnfPackage *pkg)
{
    const gchar **installonly_pkgs;
    const gchar *pkg_name;
    guint i;

    installonly_pkgs = dnf_context_get_installonly_pkgs(NULL);
    pkg_name = dnf_package_get_name(pkg);
    for (i = 0; installonly_pkgs[i] != NULL; i++) {
        if (g_strcmp0(pkg_name, installonly_pkgs[i]) == 0)
            return TRUE;
    }
    return FALSE;
}

/**
 * dnf_repo_checksum_hy_to_lr:
 **/
static LrChecksumType
dnf_repo_checksum_hy_to_lr(GChecksumType checksum)
{
    if (checksum == G_CHECKSUM_MD5)
        return LR_CHECKSUM_MD5;
    if (checksum == G_CHECKSUM_SHA1)
        return LR_CHECKSUM_SHA1;
    if (checksum == G_CHECKSUM_SHA256)
        return LR_CHECKSUM_SHA256;
    if (checksum == G_CHECKSUM_SHA384)
        return LR_CHECKSUM_SHA384;
    return LR_CHECKSUM_SHA512;
}

/**
 * dnf_package_check_filename:
 * @pkg: a #DnfPackage *instance.
 * @valid: Set to %TRUE if the package is valid.
 * @error: a #GError or %NULL..
 *
 * Checks the package is already downloaded and valid.
 *
 * Returns: %TRUE if the package was checked successfully
 *
 * Since: 0.1.0
 **/
gboolean
dnf_package_check_filename(DnfPackage *pkg, gboolean *valid, GError **error) try
{
    LrChecksumType checksum_type_lr;
    char *checksum_valid = NULL;
    const gchar *path;
    const unsigned char *checksum;
    gboolean ret = TRUE;
    int checksum_type_hy;
    int fd;

    /* check if the file does not exist */
    path = dnf_package_get_filename(pkg);
    g_debug("checking if %s already exists...", path);
    if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
        *valid = FALSE;

        /* a missing file in a local repo is an error, unless it is remote via base:url,
         * since we can't download it */
        if (dnf_package_is_local(pkg)) {
            ret = FALSE;
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_INTERNAL_ERROR,
                        "File missing in local repository %s", path);
        }

        goto out;
    }

    /* check the checksum */
    checksum = dnf_package_get_chksum(pkg, &checksum_type_hy);
    checksum_valid = hy_chksum_str(checksum, checksum_type_hy);
    checksum_type_lr = dnf_repo_checksum_hy_to_lr((GChecksumType)checksum_type_hy);
    fd = g_open(path, O_RDONLY, 0);
    if (fd < 0) {
        ret = FALSE;
        g_set_error(error,
                 DNF_ERROR,
                 DNF_ERROR_INTERNAL_ERROR,
                 "Failed to open %s", path);
        goto out;
    }
    ret = lr_checksum_fd_cmp(checksum_type_lr,
                 fd,
                 checksum_valid,
                 TRUE, /* use xattr value */
                 valid,
                 error);
    if (!ret) {
        g_close(fd, NULL);
        goto out;
    }
    ret = g_close(fd, error);
    if (!ret)
        goto out;

    /* A checksum mismatch for a package in a local repository is an
       error.  We can't repair it by downloading a corrected version,
       so let's fail here. */
    if (!*valid && dnf_repo_is_local (dnf_package_get_repo (pkg))) {
        ret = FALSE;
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "Checksum mismatch in local repository %s", path);
        goto out;
    }

out:
    g_free(checksum_valid);
    return ret;
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_package_download:
 * @pkg: a #DnfPackage *instance.
 * @directory: destination directory, or %NULL for the cachedir.
 * @state: the #DnfState.
 * @error: a #GError or %NULL..
 *
 * Downloads the package.
 *
 * Returns: the complete filename of the downloaded file
 *
 * Since: 0.1.0
 **/
gchar *
dnf_package_download(DnfPackage *pkg,
              const gchar *directory,
              DnfState *state,
              GError **error) try
{
    DnfRepo *repo;
    repo = dnf_package_get_repo(pkg);
    if (repo == NULL) {
        g_set_error_literal(error,
                     DNF_ERROR,
                     DNF_ERROR_INTERNAL_ERROR,
                     "package repo is unset");
        return NULL;
    }
    return dnf_repo_download_package(repo, pkg, directory, state, error);
} CATCH_TO_GERROR(NULL)

/**
 * dnf_package_array_download:
 * @packages: an array of packages.
 * @directory: destination directory, or %NULL for the cachedir.
 * @state: the #DnfState.
 * @error: a #GError or %NULL..
 *
 * Downloads an array of packages.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.1.0
 */
gboolean
dnf_package_array_download(GPtrArray *packages,
                const gchar *directory,
                DnfState *state,
                GError **error) try
{
    DnfState *state_local;
    GHashTableIter hiter;
    gpointer key, value;
    guint i;
    g_autoptr(GHashTable) repo_to_packages = NULL;

    /* map packages to repos */
    repo_to_packages = g_hash_table_new_full(NULL, NULL, NULL, (GDestroyNotify)g_ptr_array_unref);
    for (i = 0; i < packages->len; i++) {
        DnfPackage *pkg = (DnfPackage*)g_ptr_array_index(packages, i);
        DnfRepo *repo;
        GPtrArray *repo_packages;

        repo = dnf_package_get_repo(pkg);
        if (repo == NULL) {
            g_set_error_literal(error,
                                DNF_ERROR,
                                DNF_ERROR_INTERNAL_ERROR,
                                "package repo is unset");
            return FALSE;
        }
        repo_packages = (GPtrArray*)g_hash_table_lookup(repo_to_packages, repo);
        if (repo_packages == NULL) {
            repo_packages = g_ptr_array_new();
            g_hash_table_insert(repo_to_packages, repo, repo_packages);
        }
        g_ptr_array_add(repo_packages, pkg);
    }

    /* set steps according to the number of repos we are going to download from */
    dnf_state_set_number_steps(state, g_hash_table_size(repo_to_packages));

    /* download all packages from each repo in one go */
    g_hash_table_iter_init(&hiter, repo_to_packages);
    while (g_hash_table_iter_next(&hiter, &key, &value)) {
        DnfRepo *repo = (DnfRepo*)key;
        GPtrArray *repo_packages = (GPtrArray*)value;

        state_local = dnf_state_get_child(state);
        if (!dnf_repo_download_packages(repo, repo_packages, directory, state_local, error))
            return FALSE;

        /* done */
        if (!dnf_state_done(state, error))
            return FALSE;
    }
    return TRUE;
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_package_array_get_download_size:
 * @packages: an array of packages.
 *
 * Gets the download size for an array of packages.
 *
 * Returns: the download size
 *
 * Since: 0.2.3
 */
guint64
dnf_package_array_get_download_size(GPtrArray *packages)
{
    guint i;
    guint64 download_size = 0;

    for (i = 0; i < packages->len; i++) {
        DnfPackage *pkg = (DnfPackage*)g_ptr_array_index(packages, i);

        download_size += dnf_package_get_downloadsize(pkg);
    }

    return download_size;
}
