/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2013-2015 Richard Hughes <richard@hughsie.com>
 *
 * Most of this code was taken from Zif, libzif/zif-repos.c
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
 * SECTION:dnf-repo
 * @short_description: Object representing a remote repo.
 * @include: libdnf.h
 * @stability: Unstable
 *
 * Sources are remote repositories of packages.
 *
 * See also: #DnfRepo
 */


#include <strings.h>
#include <fcntl.h>
#include <glib/gstdio.h>
#include "hy-util.h"
#include <librepo/librepo.h>
#include <rpm/rpmts.h>

#include "dnf-keyring.h"
#include "dnf-package.h"
#include "dnf-repo.h"
#include "dnf-types.h"
#include "dnf-utils.h"

typedef struct
{
    DnfRepoEnabled   enabled;
    gboolean         required;
    gboolean         gpgcheck_md;
    gboolean         gpgcheck_pkgs;
    gchar           *gpgkey;
    gchar          **exclude_packages;
    guint            cost;
    gchar           *filename;      /* /etc/yum.repos.d/updates.repo */
    gchar           *id;
    gchar           *location;      /* /var/cache/PackageKit/metadata/fedora */
    gchar           *location_tmp;  /* /var/cache/PackageKit/metadata/fedora.tmp */
    gchar           *packages;      /* /var/cache/PackageKit/metadata/fedora/packages */
    gchar           *packages_tmp;  /* /var/cache/PackageKit/metadata/fedora.tmp/packages */
    gchar           *keyring;       /* /var/cache/PackageKit/metadata/fedora/gpgdir */
    gchar           *keyring_tmp;   /* /var/cache/PackageKit/metadata/fedora.tmp/gpgdir */
    gchar           *pubkey;        /* /var/cache/PackageKit/metadata/fedora/repomd.pub */
    gchar           *pubkey_tmp;    /* /var/cache/PackageKit/metadata/fedora.tmp/repomd.pub */
    gint64           timestamp_generated;   /* µs */
    gint64           timestamp_modified;    /* µs */
    GError          *last_check_error;
    GKeyFile        *keyfile;
    GHashTable      *filenames_md;          /* key:filename */
    DnfContext      *context;               /* weak reference */
    DnfRepoKind      kind;
    HyRepo           repo;
    LrHandle        *repo_handle;
    LrResult        *repo_result;
    LrUrlVars       *urlvars;
} DnfRepoPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(DnfRepo, dnf_repo, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (dnf_repo_get_instance_private (o))

/**
 * dnf_repo_finalize:
 **/
static void
dnf_repo_finalize(GObject *object)
{
    DnfRepo *repo = DNF_REPO(object);
    DnfRepoPrivate *priv = GET_PRIVATE(repo);

    g_free(priv->id);
    g_free(priv->filename);
    g_free(priv->gpgkey);
    g_strfreev(priv->exclude_packages);
    g_free(priv->location_tmp);
    g_free(priv->location);
    g_free(priv->packages);
    g_free(priv->packages_tmp);
    g_free(priv->keyring);
    g_free(priv->keyring_tmp);
    g_free(priv->pubkey);
    g_free(priv->pubkey_tmp);
    g_hash_table_unref(priv->filenames_md);
    g_clear_error(&priv->last_check_error);
    if (priv->repo_result != NULL)
        lr_result_free(priv->repo_result);
    if (priv->repo_handle != NULL)
        lr_handle_free(priv->repo_handle);
    if (priv->repo != NULL)
        hy_repo_free(priv->repo);
    if (priv->keyfile != NULL)
        g_key_file_unref(priv->keyfile);
    if (priv->context != NULL)
        g_object_remove_weak_pointer(G_OBJECT(priv->context),
                                     (void **) &priv->context);

    G_OBJECT_CLASS(dnf_repo_parent_class)->finalize(object);
}

/**
 * dnf_repo_init:
 **/
static void
dnf_repo_init(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    priv->cost = 1000;
    priv->repo_handle = lr_handle_init();
    priv->repo_result = lr_result_init();
    priv->filenames_md = g_hash_table_new_full(g_str_hash, g_str_equal,
                                               g_free, g_free);
    priv->required = FALSE;  /* This is the original default which we're
                              * keeping for compatibility.
                              */
}

/**
 * dnf_repo_class_init:
 **/
static void
dnf_repo_class_init(DnfRepoClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_repo_finalize;
}

/**
 * dnf_repo_get_id:
 * @repo: a #DnfRepo instance.
 *
 * Gets the repo ID.
 *
 * Returns: the repo ID, e.g. "fedora-updates"
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_repo_get_id(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return priv->id;
}

/**
 * dnf_repo_get_location:
 * @repo: a #DnfRepo instance.
 *
 * Gets the repo location.
 *
 * Returns: the repo location, e.g. "/var/cache/PackageKit/metadata/fedora"
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_repo_get_location(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return priv->location;
}

/**
 * dnf_repo_get_filename:
 * @repo: a #DnfRepo instance.
 *
 * Gets the repo filename.
 *
 * Returns: the repo filename, e.g. "/etc/yum.repos.d/updates.repo"
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_repo_get_filename(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return priv->filename;
}

/**
 * dnf_repo_get_packages:
 * @repo: a #DnfRepo instance.
 *
 * Gets the repo packages location.
 *
 * Returns: the repo packages location, e.g. "/var/cache/PackageKit/metadata/fedora/packages"
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_repo_get_packages(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return priv->packages;
}

/**
 * dnf_repo_substitute:
 */
static gchar *
dnf_repo_substitute(DnfRepo *repo, const gchar *url)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    char *tmp;
    gchar *substituted;

    /* do a little dance so we can use g_free() rather than lr_free() */
    tmp = lr_url_substitute(url, priv->urlvars);
    substituted = g_strdup(tmp);
    lr_free(tmp);

    return substituted;
}

/**
 * dnf_repo_get_description:
 * @repo: a #DnfRepo instance.
 *
 * Gets the repo description.
 *
 * Returns: the repo description, e.g. "Fedora 20 Updates"
 *
 * Since: 0.1.0
 **/
gchar *
dnf_repo_get_description(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    g_autofree gchar *tmp = NULL;

    /* is DVD */
    if (priv->kind == DNF_REPO_KIND_MEDIA) {
        tmp = g_key_file_get_string(priv->keyfile, "general", "name", NULL);
        if (tmp == NULL)
            return NULL;
    } else {
        tmp = g_key_file_get_string(priv->keyfile,
                         dnf_repo_get_id(repo),
                         "name",
                         NULL);
        if (tmp == NULL)
            return NULL;
    }

    /* have to substitute things like $releasever and $basearch */
    return dnf_repo_substitute(repo, tmp);
}

/**
 * dnf_repo_get_enabled:
 * @repo: a #DnfRepo instance.
 *
 * Gets if the repo is enabled.
 *
 * Returns: %DNF_REPO_ENABLED_PACKAGES if enabled
 *
 * Since: 0.1.0
 **/
DnfRepoEnabled
dnf_repo_get_enabled(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return priv->enabled;
}

/**
 * dnf_repo_get_required:
 * @repo: a #DnfRepo instance.
 *
 * Returns: %TRUE if failure fetching this repo is fatal
 *
 * Since: 0.2.1
 **/
gboolean
dnf_repo_get_required(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return priv->required;
}

/**
 * dnf_repo_get_cost:
 * @repo: a #DnfRepo instance.
 *
 * Gets the repo cost.
 *
 * Returns: the repo cost, where 1000 is default
 *
 * Since: 0.1.0
 **/
guint
dnf_repo_get_cost(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return priv->cost;
}

/**
 * dnf_repo_get_kind:
 * @repo: a #DnfRepo instance.
 *
 * Gets the repo kind.
 *
 * Returns: the repo kind, e.g. %DNF_REPO_KIND_MEDIA
 *
 * Since: 0.1.0
 **/
DnfRepoKind
dnf_repo_get_kind(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return priv->kind;
}

/**
 * dnf_repo_get_exclude_packages:
 * @repo: a #DnfRepo instance.
 *
 * Returns:(transfer none)(array zero-terminated=1): Packages which should be excluded
 *
 * Since: 0.1.9
 **/
gchar **
dnf_repo_get_exclude_packages(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return priv->exclude_packages;
}

/**
 * dnf_repo_get_gpgcheck:
 * @repo: a #DnfRepo instance.
 *
 * Gets if the packages should be signed.
 *
 * Returns: %TRUE if packages should be signed
 *
 * Since: 0.1.0
 **/
gboolean
dnf_repo_get_gpgcheck(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return priv->gpgcheck_pkgs;
}

/**
 * dnf_repo_get_gpgcheck_md:
 * @repo: a #DnfRepo instance.
 *
 * Gets if the metadata should be signed.
 *
 * Returns: %TRUE if metadata should be signed
 *
 * Since: 0.1.7
 **/
gboolean
dnf_repo_get_gpgcheck_md(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return priv->gpgcheck_md;
}

/**
 * dnf_repo_get_repo:
 * @repo: a #DnfRepo instance.
 *
 * Gets the #HyRepo backing this repo.
 *
 * Returns: the #HyRepo
 *
 * Since: 0.1.0
 **/
HyRepo
dnf_repo_get_repo(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return priv->repo;
}

/**
 * dnf_repo_is_devel:
 * @repo: a #DnfRepo instance.
 *
 * Returns: %TRUE if the repo is a development repo
 *
 * Since: 0.1.0
 **/
gboolean
dnf_repo_is_devel(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    if (g_str_has_suffix(priv->id, "-debuginfo"))
        return TRUE;
    if (g_str_has_suffix(priv->id, "-debug"))
        return TRUE;
    if (g_str_has_suffix(priv->id, "-development"))
        return TRUE;
    return FALSE;
}

/**
 * dnf_repo_is_local:
 * @repo: a #DnfRepo instance.
 *
 * Returns: %TRUE if the repo is a repo on local or media filesystem
 *
 * Since: 0.1.6
 **/
gboolean
dnf_repo_is_local(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);

    /* media or local */
    if (priv->kind == DNF_REPO_KIND_MEDIA ||
        priv->kind == DNF_REPO_KIND_LOCAL)
        return TRUE;

    return FALSE;
}

/**
 * dnf_repo_is_repo:
 * @repo: a #DnfRepo instance.
 *
 * Returns: %TRUE if the repo is a repo repo
 *
 * Since: 0.1.0
 **/
gboolean
dnf_repo_is_repo(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    if (g_str_has_suffix(priv->id, "-repo"))
        return TRUE;
    return FALSE;
}

/**
 * dnf_repo_set_id:
 * @repo: a #DnfRepo instance.
 * @id: the ID, e.g. "fedora-updates"
 *
 * Sets the repo ID.
 *
 * Since: 0.1.0
 **/
void
dnf_repo_set_id(DnfRepo *repo, const gchar *id)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    g_free(priv->id);
    priv->id = g_strdup(id);
}

/**
 * dnf_repo_set_location:
 * @repo: a #DnfRepo instance.
 * @location: the path
 *
 * Sets the repo location and the default packages path.
 *
 * Since: 0.1.0
 **/
void
dnf_repo_set_location(DnfRepo *repo, const gchar *location)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    g_free(priv->location);
    g_free(priv->packages);
    g_free(priv->keyring);
    g_free(priv->pubkey);
    priv->location = dnf_repo_substitute(repo, location);
    priv->packages = g_build_filename(location, "packages", NULL);
    priv->keyring = g_build_filename(location, "gpgdir", NULL);
    priv->pubkey = g_build_filename(location, "repomd.pub", NULL);
}

/**
 * dnf_repo_set_location_tmp:
 * @repo: a #DnfRepo instance.
 * @location_tmp: the temp path
 *
 * Sets the repo location for temporary files and the default packages path.
 *
 * Since: 0.1.0
 **/
void
dnf_repo_set_location_tmp(DnfRepo *repo, const gchar *location_tmp)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    g_free(priv->location_tmp);
    g_free(priv->packages_tmp);
    g_free(priv->keyring_tmp);
    g_free(priv->pubkey_tmp);
    priv->location_tmp = dnf_repo_substitute(repo, location_tmp);
    priv->packages_tmp = g_build_filename(location_tmp, "packages", NULL);
    priv->keyring_tmp = g_build_filename(location_tmp, "gpgdir", NULL);
    priv->pubkey_tmp = g_build_filename(location_tmp, "repomd.pub", NULL);
}

/**
 * dnf_repo_set_filename:
 * @repo: a #DnfRepo instance.
 * @filename: the filename path
 *
 * Sets the repo backing filename.
 *
 * Since: 0.1.0
 **/
void
dnf_repo_set_filename(DnfRepo *repo, const gchar *filename)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);

    g_free(priv->filename);
    priv->filename = g_strdup(filename);
}

/**
 * dnf_repo_set_packages:
 * @repo: a #DnfRepo instance.
 * @packages: the path to the package files
 *
 * Sets the repo package location, typically ending in "Packages" or "packages".
 *
 * Since: 0.1.0
 **/
void
dnf_repo_set_packages(DnfRepo *repo, const gchar *packages)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    g_free(priv->packages);
    priv->packages = g_strdup(packages);
}

/**
 * dnf_repo_set_packages_tmp:
 * @repo: a #DnfRepo instance.
 * @packages_tmp: the path to the temporary package files
 *
 * Sets the repo package location for temporary files.
 *
 * Since: 0.1.0
 **/
void
dnf_repo_set_packages_tmp(DnfRepo *repo, const gchar *packages_tmp)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    g_free(priv->packages_tmp);
    priv->packages_tmp = g_strdup(packages_tmp);
}

/**
 * dnf_repo_set_enabled:
 * @repo: a #DnfRepo instance.
 * @enabled: if the repo is enabled
 *
 * Sets the repo enabled state.
 *
 * Since: 0.1.0
 **/
void
dnf_repo_set_enabled(DnfRepo *repo, DnfRepoEnabled enabled)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);

    priv->enabled = enabled;

    /* packages implies metadata */
    if (priv->enabled & DNF_REPO_ENABLED_PACKAGES)
        priv->enabled |= DNF_REPO_ENABLED_METADATA;
}

/**
 * dnf_repo_set_required:
 * @repo: a #DnfRepo instance.
 * @required: if the repo is required
 *
 * Sets whether failure to retrieve the repo is fatal; by default it
 * is not.
 *
 * Since: 0.2.1
 **/
void
dnf_repo_set_required(DnfRepo *repo, gboolean required)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);

    priv->required = required;
}

/**
 * dnf_repo_set_cost:
 * @repo: a #DnfRepo instance.
 * @cost: the repo cost
 *
 * Sets the repo cost, where 1000 is the default.
 *
 * Since: 0.1.0
 **/
void
dnf_repo_set_cost(DnfRepo *repo, guint cost)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    priv->cost = cost;
}

/**
 * dnf_repo_set_kind:
 * @repo: a #DnfRepo instance.
 * @kind: the #DnfRepoKind
 *
 * Sets the repo kind.
 *
 * Since: 0.1.0
 **/
void
dnf_repo_set_kind(DnfRepo *repo, DnfRepoKind kind)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    priv->kind = kind;
}

/**
 * dnf_repo_set_gpgcheck:
 * @repo: a #DnfRepo instance.
 * @gpgcheck_pkgs: if the repo packages should be signed
 *
 * Sets the repo signed status.
 *
 * Since: 0.1.0
 **/
void
dnf_repo_set_gpgcheck(DnfRepo *repo, gboolean gpgcheck_pkgs)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    priv->gpgcheck_pkgs = gpgcheck_pkgs;
}

/**
 * dnf_repo_set_gpgcheck_md:
 * @repo: a #DnfRepo instance.
 * @gpgcheck_md: if the repo metadata should be signed
 *
 * Sets the metadata signed status.
 *
 * Since: 0.1.7
 **/
void
dnf_repo_set_gpgcheck_md(DnfRepo *repo, gboolean gpgcheck_md)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    priv->gpgcheck_md = gpgcheck_md;
}

/**
 * dnf_repo_set_keyfile:
 * @repo: a #DnfRepo instance.
 * @keyfile: the #GKeyFile
 *
 * Sets the repo keyfile used to create the repo.
 *
 * Since: 0.1.0
 **/
void
dnf_repo_set_keyfile(DnfRepo *repo, GKeyFile *keyfile)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    if (priv->keyfile != NULL)
        g_key_file_unref(priv->keyfile);
    priv->keyfile = g_key_file_ref(keyfile);
}

/**
 * dnf_repo_get_username_password_string:
 */
static gchar *
dnf_repo_get_username_password_string(const gchar *user, const gchar *pass)
{
    if (user == NULL && pass == NULL)
        return NULL;
    if (user != NULL && pass == NULL)
        return g_strdup(user);
    if (user == NULL && pass != NULL)
        return g_strdup_printf(":%s", pass);
    return g_strdup_printf("%s:%s", user, pass);
}

/**
 * dnf_repo_set_keyfile_data:
 */
static gboolean
dnf_repo_set_keyfile_data(DnfRepo *repo, GError **error)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    guint cost;
    g_autofree gchar *metalink = NULL;
    g_autofree gchar *mirrorlist = NULL;
    g_autofree gchar *proxy = NULL;
    g_autofree gchar *exclude_string = NULL;
    g_autofree gchar *pwd = NULL;
    g_autofree gchar *usr = NULL;
    g_autofree gchar *usr_pwd = NULL;
    g_autofree gchar *usr_pwd_proxy = NULL;
    g_auto(GStrv) baseurls;

    g_debug("setting keyfile data for %s", priv->id);

    /* cost is optional */
    cost = g_key_file_get_integer(priv->keyfile, priv->id, "cost", NULL);
    if (cost != 0)
        dnf_repo_set_cost(repo, cost);

    /* baseurl is optional */
    baseurls = g_key_file_get_string_list(priv->keyfile, priv->id, "baseurl", NULL, NULL);
    if (baseurls && !lr_handle_setopt(priv->repo_handle, error, LRO_URLS, baseurls))
        return FALSE;

    /* mirrorlist is optional */
    mirrorlist = g_key_file_get_string(priv->keyfile, priv->id, "mirrorlist", NULL);
    if (mirrorlist && !lr_handle_setopt(priv->repo_handle, error, LRO_MIRRORLIST, mirrorlist))
        return FALSE;

    /* metalink is optional */
    metalink = g_key_file_get_string(priv->keyfile, priv->id, "metalink", NULL);
    if (metalink && !lr_handle_setopt(priv->repo_handle, error, LRO_METALINKURL, metalink))
        return FALSE;

    /* file:// */
    if (baseurls != NULL && baseurls[0] != NULL &&
        mirrorlist == NULL && metalink == NULL) {
        g_autofree gchar *url = NULL;
        url = lr_prepend_url_protocol(baseurls[0]);
        if (url != NULL && strncasecmp(url, "file://", 7) == 0) {
            if (g_strstr_len(url, -1, "$testdatadir") == NULL)
                priv->kind = DNF_REPO_KIND_LOCAL;
            dnf_repo_set_location(repo, url + 7);
        }
    }

    /* set location if currently unset */
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_LOCAL, 0L))
        return FALSE;
    if (priv->location == NULL) {
        g_autofree gchar *tmp = NULL;
        tmp = g_build_filename(dnf_context_get_cache_dir(priv->context),
                    priv->id, NULL);
        dnf_repo_set_location(repo, tmp);
    }

    /* set temp location for remote repos */
    if (priv->kind == DNF_REPO_KIND_REMOTE) {
        g_autoptr(GString) tmp = NULL;
        tmp = g_string_new(priv->location);
        if (tmp->len > 0 && tmp->str[tmp->len - 1] == '/')
            g_string_truncate(tmp, tmp->len - 1);
        g_string_append(tmp, ".tmp");
        dnf_repo_set_location_tmp(repo, tmp->str);
    }

    /* gpgkey is optional for gpgcheck=1, but required for repo_gpgcheck=1 */
    g_free(priv->gpgkey);
    priv->gpgkey = g_key_file_get_string(priv->keyfile, priv->id, "gpgkey", NULL);
    priv->gpgcheck_pkgs = g_key_file_get_boolean(priv->keyfile, priv->id,
                              "gpgcheck", NULL);
    priv->gpgcheck_md = g_key_file_get_boolean(priv->keyfile, priv->id,
                              "repo_gpgcheck", NULL);
    if (priv->gpgcheck_md && priv->gpgkey == NULL) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_FILE_INVALID,
                            "gpgkey not set, yet repo_gpgcheck=1");
        return FALSE;
    }

    /* XXX: setopt() expects a long, so we need a long on the stack */
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_GPGCHECK, (long)priv->gpgcheck_md))
        return FALSE;

    exclude_string = g_key_file_get_string(priv->keyfile, priv->id, "exclude", NULL);
    if (exclude_string) {
        priv->exclude_packages = g_strsplit_set(exclude_string, " ,", -1);
    }

    /* proxy is optional */
    proxy = g_key_file_get_string(priv->keyfile, priv->id, "proxy", NULL);
    if (proxy == NULL)
        proxy = g_strdup(dnf_context_get_http_proxy(priv->context));
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_PROXY, proxy))
        return FALSE;

    /* both parts of the proxy auth are optional */
    usr = g_key_file_get_string(priv->keyfile, priv->id, "proxy_username", NULL);
    pwd = g_key_file_get_string(priv->keyfile, priv->id, "proxy_password", NULL);
    usr_pwd_proxy = dnf_repo_get_username_password_string(usr, pwd);
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_PROXYUSERPWD, usr_pwd_proxy))
        return FALSE;

    /* both parts of the HTTP auth are optional */
    usr = g_key_file_get_string(priv->keyfile, priv->id, "username", NULL);
    pwd = g_key_file_get_string(priv->keyfile, priv->id, "password", NULL);
    usr_pwd = dnf_repo_get_username_password_string(usr, pwd);
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_USERPWD, usr_pwd))
        return FALSE;
    return TRUE;
}

/**
 * dnf_repo_setup:
 * @repo: a #DnfRepo instance.
 * @error: a #GError or %NULL
 *
 * Sets up the repo ready for use.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.1.0
 **/
gboolean
dnf_repo_setup(DnfRepo *repo, GError **error)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    g_autofree gchar *basearch = NULL;
    g_autofree gchar *release = NULL;
    g_autofree gchar *testdatadir = NULL;
    g_autofree gchar *user_agent = NULL;
    g_autofree gchar *sslcacert = NULL;
    g_autofree gchar *sslclientcert = NULL;
    g_autofree gchar *sslclientkey = NULL;
    gboolean sslverify = TRUE;

    basearch = g_key_file_get_string(priv->keyfile, "general", "arch", NULL);
    if (basearch == NULL)
        basearch = g_strdup(dnf_context_get_base_arch(priv->context));
    if (basearch == NULL) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_INTERNAL_ERROR,
                            "basearch not set");
        return FALSE;
    }
    release = g_key_file_get_string(priv->keyfile, "general", "version", NULL);
    if (release == NULL)
        release = g_strdup(dnf_context_get_release_ver(priv->context));
    if (release == NULL) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_INTERNAL_ERROR,
                            "releasever not set");
        return FALSE;
    }
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_USERAGENT, dnf_context_get_user_agent(priv->context)))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_REPOTYPE, LR_YUMREPO))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_INTERRUPTIBLE, 0L))
        return FALSE;
    priv->urlvars = lr_urlvars_set(priv->urlvars, "releasever", release);
    priv->urlvars = lr_urlvars_set(priv->urlvars, "basearch", basearch);
    testdatadir = dnf_realpath(TESTDATADIR);
    priv->urlvars = lr_urlvars_set(priv->urlvars, "testdatadir", testdatadir);
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_VARSUB, priv->urlvars))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_GNUPGHOMEDIR, priv->keyring))
        return FALSE;

    if (g_key_file_has_key(priv->keyfile, priv->id, "sslverify", NULL))
        sslverify = g_key_file_get_boolean(priv->keyfile, priv->id, "sslverify", NULL);

    /* XXX: setopt() expects a long, so we need a long on the stack */
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_SSLVERIFYPEER, (long)sslverify))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_SSLVERIFYHOST, (long)sslverify))
        return FALSE;

    sslcacert = g_key_file_get_string(priv->keyfile, priv->id, "sslcacert", NULL);
    if (sslcacert != NULL) {
        if (!lr_handle_setopt(priv->repo_handle, error, LRO_SSLCACERT, sslcacert))
            return FALSE;
    }

    sslclientcert = g_key_file_get_string(priv->keyfile, priv->id, "sslclientcert", NULL);
    if (sslclientcert != NULL) {
        if (!lr_handle_setopt(priv->repo_handle, error, LRO_SSLCLIENTCERT, sslclientcert))
            return FALSE;
    }

    sslclientkey = g_key_file_get_string(priv->keyfile, priv->id, "sslclientkey", NULL);
    if (sslclientkey != NULL) {
        if (!lr_handle_setopt(priv->repo_handle, error, LRO_SSLCLIENTKEY, sslclientkey))
            return FALSE;
    }

    return dnf_repo_set_keyfile_data(repo, error);
}

/**
 * dnf_repo_update_state_cb:
 */
static int
dnf_repo_update_state_cb(void *user_data,
                         gdouble total_to_download,
                         gdouble now_downloaded)
{
    gboolean ret;
    gdouble percentage;
    DnfState *state =(DnfState *) user_data;

    /* abort */
    if (!dnf_state_check(state, NULL))
        return -1;

    /* the number of files has changed */
    if (total_to_download <= 0.01 && now_downloaded <= 0.01) {
        dnf_state_reset(state);
        return 0;
    }

    /* nothing sensible */
    if (total_to_download < 0)
        return 0;

    /* set percentage */
    percentage = 100.0f * now_downloaded / total_to_download;
    ret = dnf_state_set_percentage(state, percentage);
    if (ret) {
        g_debug("update state %.0f/%.0f",
                now_downloaded,
                total_to_download);
    }

    return 0;
}

/**
 * dnf_repo_set_timestamp_modified:
 */
static gboolean
dnf_repo_set_timestamp_modified(DnfRepo *repo, GError **error)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    g_autofree gchar *filename = NULL;
    g_autoptr(GFile) file = NULL;
    g_autoptr(GFileInfo) info = NULL;

    filename = g_build_filename(priv->location, "repodata", "repomd.xml", NULL);
    file = g_file_new_for_path(filename);
    info = g_file_query_info(file,
                             G_FILE_ATTRIBUTE_TIME_MODIFIED ","
                             G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC,
                             G_FILE_QUERY_INFO_NONE,
                             NULL,
                             error);
    if (info == NULL)
        return FALSE;
    priv->timestamp_modified = g_file_info_get_attribute_uint64(info,
                                    G_FILE_ATTRIBUTE_TIME_MODIFIED) * G_USEC_PER_SEC;
    priv->timestamp_modified += g_file_info_get_attribute_uint32(info,
                                    G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC);
    return TRUE;
}

static gboolean
dnf_repo_check_internal(DnfRepo *repo,
                        guint permissible_cache_age,
                        DnfState *state,
                        GError **error)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    const gchar *download_list[] = {
        "primary",
        "filelists",
        "group",
        "updateinfo",
        "appstream",
        "appstream-icons",
        NULL};
    const gchar *tmp;
    gboolean ret;
    LrYumRepo *yum_repo;
    const gchar *urls[] = { "", NULL };
    gint64 age_of_data; /* in seconds */
    g_autoptr(GError) error_local = NULL;

    /* has the media repo vanished? */
    if (priv->kind == DNF_REPO_KIND_MEDIA &&
        !g_file_test(priv->location, G_FILE_TEST_EXISTS)) {
        priv->enabled = DNF_REPO_ENABLED_NONE;
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_REPO_NOT_AVAILABLE,
                    "%s was not found", priv->location);
        return FALSE;
    }

    /* has the local repo vanished? */
    if (priv->kind == DNF_REPO_KIND_LOCAL &&
        !g_file_test(priv->location, G_FILE_TEST_EXISTS)) {
        priv->enabled = DNF_REPO_ENABLED_NONE;
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_REPO_NOT_AVAILABLE,
                    "%s was not found", priv->location);
        return FALSE;
    }

    /* Yum metadata */
    dnf_state_action_start(state, DNF_STATE_ACTION_LOADING_CACHE, NULL);
    urls[0] = priv->location;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_URLS, urls))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_DESTDIR, priv->location))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_LOCAL, 1L))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_CHECKSUM, 1L))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_YUMDLIST, download_list))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_MIRRORLIST, NULL))
        return FALSE;
    lr_result_clear(priv->repo_result);
    if (!lr_handle_perform(priv->repo_handle, priv->repo_result, &error_local)) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_REPO_NOT_AVAILABLE,
                    "repodata %s was not complete: %s",
                    priv->id, error_local->message);
        return FALSE;
    }

    /* get the metadata file locations */
    if (!lr_result_getinfo(priv->repo_result, &error_local, LRR_YUM_REPO, &yum_repo)) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "failed to get yum-repo: %s",
                    error_local->message);
        return FALSE;
    }

    /* get timestamp */
    ret = lr_result_getinfo(priv->repo_result, &error_local,
                 LRR_YUM_TIMESTAMP, &priv->timestamp_generated);
    if (!ret) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "failed to get timestamp: %s",
                    error_local->message);
        return FALSE;
    }

    /* check metadata age for non-local repos */
    if (priv->kind != DNF_REPO_KIND_LOCAL && permissible_cache_age != G_MAXUINT) {
        if (!dnf_repo_set_timestamp_modified(repo, error))
            return FALSE;
        age_of_data =(g_get_real_time() - priv->timestamp_modified) / G_USEC_PER_SEC;
        if (age_of_data > permissible_cache_age) {
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_INTERNAL_ERROR,
                        "cache too old: %"G_GINT64_FORMAT" > %i",
                        age_of_data, permissible_cache_age);
            return FALSE;
        }
    }

    /* create a HyRepo */
    priv->repo = hy_repo_create(priv->id);
    hy_repo_set_string(priv->repo, HY_REPO_MD_FN, yum_repo->repomd);
    tmp = lr_yum_repo_path(yum_repo, "primary");
    if (tmp != NULL) {
        hy_repo_set_string(priv->repo, HY_REPO_PRIMARY_FN, tmp);
        g_hash_table_insert(priv->filenames_md,
                            g_strdup("primary"),
                            g_strdup(tmp));
    }
    tmp = lr_yum_repo_path(yum_repo, "filelists");
    if (tmp != NULL) {
        hy_repo_set_string(priv->repo, HY_REPO_FILELISTS_FN, tmp);
        g_hash_table_insert(priv->filenames_md,
                            g_strdup("filelists"),
                            g_strdup(tmp));
    }
    tmp = lr_yum_repo_path(yum_repo, "updateinfo");
    if (tmp != NULL) {
        hy_repo_set_string(priv->repo, HY_REPO_UPDATEINFO_FN, tmp);
        g_hash_table_insert(priv->filenames_md,
                            g_strdup("updateinfo"),
                            g_strdup(tmp));
    }
    tmp = lr_yum_repo_path(yum_repo, "group");
    if (tmp != NULL) {
        g_hash_table_insert(priv->filenames_md,
                            g_strdup("group"),
                            g_strdup(tmp));
    }
    tmp = lr_yum_repo_path(yum_repo, "appstream");
    if (tmp != NULL) {
        g_hash_table_insert(priv->filenames_md,
                            g_strdup("appstream"),
                            g_strdup(tmp));
    }
    tmp = lr_yum_repo_path(yum_repo, "appstream-icons");
    if (tmp != NULL) {
        g_hash_table_insert(priv->filenames_md,
                            g_strdup("appstream-icons"),
                            g_strdup(tmp));
    }

    /* ensure we reset the values from the keyfile */
    if (!dnf_repo_set_keyfile_data(repo, error))
        return FALSE;

    return TRUE;
}

/**
 * dnf_repo_check:
 * @repo: a #DnfRepo instance.
 * @permissible_cache_age: The oldest cache age allowed in seconds(wall clock time); Pass %G_MAXUINT to ignore
 * @state: a #DnfState instance.
 * @error: a #GError or %NULL.
 *
 * Checks the repo.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_repo_check(DnfRepo *repo,
               guint permissible_cache_age,
               DnfState *state,
               GError **error)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    g_clear_error(&priv->last_check_error);
    if (!dnf_repo_check_internal(repo, permissible_cache_age, state,
                                   &priv->last_check_error)) {
        if (error)
            *error = g_error_copy(priv->last_check_error);
        return FALSE;
    }
    return TRUE;
}


/**
 * dnf_repo_get_filename_md:
 * @repo: a #DnfRepo instance.
 * @md_kind: The file kind, e.g. "primary" or "updateinfo"
 *
 * Gets the filename used for a repo data kind.
 *
 * Returns: the full path to the data file, %NULL otherwise
 *
 * Since: 0.1.7
 **/
const gchar *
dnf_repo_get_filename_md(DnfRepo *repo, const gchar *md_kind)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    g_return_val_if_fail(md_kind != NULL, NULL);
    return g_hash_table_lookup(priv->filenames_md, md_kind);
}

/**
 * dnf_repo_clean:
 * @repo: a #DnfRepo instance.
 * @error: a #GError or %NULL.
 *
 * Cleans the repo by deleting all the location contents.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_repo_clean(DnfRepo *repo, GError **error)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);

    /* do not clean media or local repos */
    if (priv->kind == DNF_REPO_KIND_MEDIA ||
        priv->kind == DNF_REPO_KIND_LOCAL)
        return TRUE;

    if (!g_file_test(priv->location, G_FILE_TEST_EXISTS))
        return TRUE;
    if (!dnf_remove_recursive(priv->location, error))
        return FALSE;
    return TRUE;
}

/**
 * dnf_repo_add_public_key:
 *
 * This imports a repodata public key into the default librpm keyring
 **/
static gboolean
dnf_repo_add_public_key(DnfRepo *repo, GError **error)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    gboolean ret;
    rpmKeyring keyring;
    rpmts ts;

    /* then import to rpmdb */
    ts = rpmtsCreate();
    keyring = rpmtsGetKeyring(ts, 1);
    ret = dnf_keyring_add_public_key(keyring, priv->pubkey_tmp, error);
    rpmKeyringFree(keyring);
    rpmtsFree(ts);
    return ret;
}

/**
 * dnf_repo_download_import_public_key:
 **/
static gboolean
dnf_repo_download_import_public_key(DnfRepo *repo, GError **error)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    int fd;

    /* does this file already exist */
    if (g_file_test(priv->pubkey_tmp, G_FILE_TEST_EXISTS))
        return lr_gpg_import_key(priv->pubkey_tmp, priv->keyring_tmp, error);

    /* create the gpgdir location */
    if (g_mkdir_with_parents(priv->keyring_tmp, 0755) != 0) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "Failed to create %s",
                    priv->keyring_tmp);
        return FALSE;
    }

    /* set the keyring location */
    if (!lr_handle_setopt(priv->repo_handle, error,
                          LRO_GNUPGHOMEDIR, priv->keyring_tmp))
        return FALSE;

    /* download to known location */
    g_debug("Downloading %s to %s", priv->gpgkey, priv->pubkey_tmp);
    fd = g_open(priv->pubkey_tmp, O_CLOEXEC | O_CREAT | O_RDWR, 0774);
    if (fd < 0) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "Failed to open %s: %i",
                    priv->pubkey_tmp, fd);
        return FALSE;
    }
    if (!lr_download_url(priv->repo_handle, priv->gpgkey, fd, error)) {
        g_close(fd, NULL);
        return FALSE;
    }
    if (!g_close(fd, error))
        return FALSE;

    /* import found key */
    return lr_gpg_import_key(priv->pubkey_tmp, priv->keyring_tmp, error);
}

/**
 * dnf_repo_update:
 * @repo: a #DnfRepo instance.
 * @flags: #DnfRepoUpdateFlags, e.g. %DNF_REPO_UPDATE_FLAG_FORCE
 * @state: a #DnfState instance.
 * @error: a #%GError or %NULL.
 *
 * Updates the repo.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_repo_update(DnfRepo *repo,
                DnfRepoUpdateFlags flags,
                DnfState *state,
                GError **error)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    DnfState *state_local;
    gboolean ret;
    gint rc;
    gint64 timestamp_new = 0;
    g_autoptr(GError) error_local = NULL;

    /* cannot change DVD contents */
    if (priv->kind == DNF_REPO_KIND_MEDIA) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_REPO_NOT_AVAILABLE,
                            "Cannot update read-only repo");
        return FALSE;
    }

    /* Just verify existence for local */
    if (priv->kind == DNF_REPO_KIND_LOCAL) {
        if (priv->last_check_error) {
            if (error) 
                *error = g_error_copy(priv->last_check_error);
            return FALSE;
        }
        /* If we didn't have an error in check, don't refresh
           local repos */
        return TRUE;
    }

    /* this needs to be set */
    if (priv->location_tmp == NULL) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "location_tmp not set for %s",
                    priv->id);
        return FALSE;
    }

    /* ensure we set the values from the keyfile */
    if (!dnf_repo_set_keyfile_data(repo, error))
        return FALSE;

    /* take lock */
    ret = dnf_state_take_lock(state,
                              DNF_LOCK_TYPE_METADATA,
                              DNF_LOCK_MODE_PROCESS,
                              error);
    if (!ret)
        goto out;

    /* set state */
    ret = dnf_state_set_steps(state, error,
                              95, /* download */
                              5, /* check */
                              -1);
    if (!ret)
        goto out;

    /* remove the temporary space if it already exists */
    if (g_file_test(priv->location_tmp, G_FILE_TEST_EXISTS)) {
        ret = dnf_remove_recursive(priv->location_tmp, error);
        if (!ret)
            goto out;
    }

    /* ensure exists */
    rc = g_mkdir_with_parents(priv->location_tmp, 0755);
    if (rc != 0) {
        ret = FALSE;
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_CANNOT_WRITE_REPO_CONFIG,
                    "Failed to create %s", priv->location_tmp);
        goto out;
    }

    /* download and import public key */
    if (priv->gpgcheck_md &&
       (g_str_has_prefix(priv->gpgkey, "https://") ||
         g_str_has_prefix(priv->gpgkey, "file://"))) {
        g_debug("importing public key %s", priv->gpgkey);
        ret = dnf_repo_download_import_public_key(repo, error);
        if (!ret)
            goto out;
    }

    /* do we autoimport this into librpm */
    if (priv->gpgcheck_md &&
       (flags & DNF_REPO_UPDATE_FLAG_IMPORT_PUBKEY) > 0) {
        ret = dnf_repo_add_public_key(repo, error);
        if (!ret)
            goto out;
    }

    g_debug("Attempting to update %s", priv->id);
    ret = lr_handle_setopt(priv->repo_handle, error,
                           LRO_LOCAL, 0L);
    if (!ret)
        goto out;
    ret = lr_handle_setopt(priv->repo_handle, error,
                           LRO_DESTDIR, priv->location_tmp);
    if (!ret)
        goto out;

    /* Callback to display progress of downloading */
    state_local = dnf_state_get_child(state);
    ret = lr_handle_setopt(priv->repo_handle, error,
                           LRO_PROGRESSDATA, state_local);
    if (!ret)
        goto out;
    ret = lr_handle_setopt(priv->repo_handle, error,
                           LRO_PROGRESSCB, dnf_repo_update_state_cb);
    if (!ret)
        goto out;
    lr_result_clear(priv->repo_result);
    dnf_state_action_start(state_local,
                DNF_STATE_ACTION_DOWNLOAD_METADATA, NULL);
    ret = lr_handle_perform(priv->repo_handle,
                            priv->repo_result,
                            &error_local);
    if (!ret) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_CANNOT_FETCH_SOURCE,
                    "cannot update repo '%s': %s",
                    dnf_repo_get_id(repo),
                    error_local->message);
        goto out;
    }

    /* check the newer metadata is newer */
    ret = lr_result_getinfo(priv->repo_result, &error_local,
                 LRR_YUM_TIMESTAMP, &timestamp_new);
    if (!ret) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "failed to get timestamp: %s",
                    error_local->message);
        goto out;
    }
    if ((flags & DNF_REPO_UPDATE_FLAG_FORCE) == 0 &&
        timestamp_new < priv->timestamp_generated) {
        g_debug("fresh metadata was older than what we have, ignoring");
        if (!dnf_state_finished(state, error))
            return FALSE;
        goto out;
    }

    /* only simulate */
    if (flags & DNF_REPO_UPDATE_FLAG_SIMULATE) {
        g_debug("simulating, so not switching to new metadata");
        ret = dnf_remove_recursive(priv->location_tmp, error);
        goto out;
    }

    /* move the packages directory from the old cache to the new cache */
    if (g_file_test(priv->packages, G_FILE_TEST_EXISTS)) {
        rc = g_rename(priv->packages, priv->packages_tmp);
        if (rc != 0) {
            ret = FALSE;
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_CANNOT_FETCH_SOURCE,
                        "cannot move %s to %s",
                        priv->packages, priv->packages_tmp);
            goto out;
        }
    }

    /* delete old /var/cache/PackageKit/metadata/$REPO/ */
    ret = dnf_repo_clean(repo, error);
    if (!ret)
        goto out;

    /* rename .tmp actual name */
    rc = g_rename(priv->location_tmp, priv->location);
    if (rc != 0) {
        ret = FALSE;
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_CANNOT_FETCH_SOURCE,
                    "cannot move %s to %s",
                    priv->location_tmp, priv->location);
        goto out;
    }
    ret = lr_handle_setopt(priv->repo_handle, error,
                           LRO_DESTDIR, priv->location);
    if (!ret)
        goto out;
    ret = lr_handle_setopt(priv->repo_handle, error,
                           LRO_GNUPGHOMEDIR, priv->keyring);
    if (!ret)
        goto out;

    /* done */
    ret = dnf_state_done(state, error);
    if (!ret)
        goto out;

    /* now setup internal hawkey stuff */
    state_local = dnf_state_get_child(state);
    ret = dnf_repo_check(repo, G_MAXUINT, state_local, error);
    if (!ret)
        goto out;

    /* signal that the vendor platform data is not resyned */
    dnf_context_invalidate_full(priv->context, "updated repo cache",
                                DNF_CONTEXT_INVALIDATE_FLAG_ENROLLMENT);

    /* done */
    ret = dnf_state_done(state, error);
    if (!ret)
        goto out;
out:
    dnf_state_release_locks(state);
    lr_handle_setopt(priv->repo_handle, NULL, LRO_PROGRESSCB, NULL);
    lr_handle_setopt(priv->repo_handle, NULL, LRO_PROGRESSDATA, 0xdeadbeef);
    return ret;
}

/**
 * dnf_repo_set_data:
 * @repo: a #DnfRepo instance.
 * @parameter: the UTF8 key.
 * @value: the UTF8 value.
 * @error: A #GError or %NULL
 *
 * Sets data on the repo.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_repo_set_data(DnfRepo *repo,
                  const gchar *parameter,
                  const gchar *value,
                  GError **error)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    g_key_file_set_string(priv->keyfile, priv->id, parameter, value);
    return TRUE;
}

/**
 * dnf_repo_commit:
 * @repo: a #DnfRepo instance.
 * @error: A #GError or %NULL
 *
 * Commits data on the repo, which involves saving a new .repo file.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.4
 **/
gboolean
dnf_repo_commit(DnfRepo *repo, GError **error)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    g_autofree gchar *data = NULL;

    /* cannot change DVD contents */
    if (priv->kind == DNF_REPO_KIND_MEDIA) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_CANNOT_WRITE_REPO_CONFIG,
                            "Cannot commit to read-only media");
        return FALSE;
    }

    /* dump updated file to disk */
    data = g_key_file_to_data(priv->keyfile, NULL, error);
    if (data == NULL)
        return FALSE;
    return g_file_set_contents(priv->filename, data, -1, error);
}

/**
 * dnf_repo_checksum_hy_to_lr:
 **/
static LrChecksumType
dnf_repo_checksum_hy_to_lr(int checksum_hy)
{
    if (checksum_hy == G_CHECKSUM_MD5)
        return LR_CHECKSUM_MD5;
    if (checksum_hy == G_CHECKSUM_SHA1)
        return LR_CHECKSUM_SHA1;
    if (checksum_hy == G_CHECKSUM_SHA256)
        return LR_CHECKSUM_SHA256;
    return LR_CHECKSUM_UNKNOWN;
}

/**
 * dnf_repo_copy_progress_cb:
 **/
static void
dnf_repo_copy_progress_cb(goffset current, goffset total, gpointer user_data)
{
    DnfState *state = DNF_STATE(user_data);
    dnf_state_set_percentage(state, 100.0f * current / total);
}

/**
 * dnf_repo_copy_package:
 **/
static gboolean
dnf_repo_copy_package(DnfPackage *pkg,
                      const gchar *directory,
                      DnfState *state,
                      GError **error)
{
    g_autofree gchar *basename = NULL;
    g_autofree gchar *dest = NULL;
    g_autoptr(GFile) file_dest = NULL;
    g_autoptr(GFile) file_repo = NULL;

    /* copy the file with progress */
    file_repo = g_file_new_for_path(dnf_package_get_filename(pkg));
    basename = g_path_get_basename(dnf_package_get_location(pkg));
    dest = g_build_filename(directory, basename, NULL);
    file_dest = g_file_new_for_path(dest);
    return g_file_copy(file_repo, file_dest, G_FILE_COPY_NONE,
                       dnf_state_get_cancellable(state),
                       dnf_repo_copy_progress_cb, state, error);
}

typedef struct
{
    gchar *last_mirror_url;
    gchar *last_mirror_failure_message;
    guint64 downloaded;
    guint64 download_size;
} GlobalDownloadData;

typedef struct
{
    DnfPackage *pkg;
    DnfState *state;
    guint64 downloaded;
    GlobalDownloadData *global_download_data;
} PackageDownloadData;

static int
package_download_update_state_cb(void *user_data,
                                 gdouble total_to_download,
                                 gdouble now_downloaded)
{
    PackageDownloadData *data = user_data;
    GlobalDownloadData *global_data = data->global_download_data;
    gboolean ret;
    gdouble percentage;
    guint64 previously_downloaded;

    /* abort */
    if (!dnf_state_check(data->state, NULL))
        return -1;

    /* nothing sensible */
    if (total_to_download < 0 || now_downloaded < 0)
        return 0;

    dnf_state_action_start(data->state,
                           DNF_STATE_ACTION_DOWNLOAD_PACKAGES,
                           dnf_package_get_package_id(data->pkg));

    previously_downloaded = data->downloaded;
    data->downloaded = now_downloaded;

    global_data->downloaded += (now_downloaded - previously_downloaded);

    /* set percentage */
    percentage = 100.0f * global_data->downloaded / global_data->download_size;
    ret = dnf_state_set_percentage(data->state, percentage);
    if (ret) {
        g_debug("update state %d/%d",
               (int)global_data->downloaded,
               (int)global_data->download_size);
    }

    return 0;
}

static int
package_download_end_cb(void *user_data,
                        LrTransferStatus status,
                        const char *msg)
{
    PackageDownloadData *data = user_data;

    g_slice_free(PackageDownloadData, data);

    return LR_CB_OK;
}

static int
mirrorlist_failure_cb(void *user_data,
                      const char *message,
                      const char *url)
{
    PackageDownloadData *data = user_data;
    GlobalDownloadData *global_data = data->global_download_data;

    if (global_data->last_mirror_url)
        goto out;

    global_data->last_mirror_url = g_strdup(url);
    global_data->last_mirror_failure_message = g_strdup(message);
out:
    return LR_CB_OK;
}

LrHandle *
dnf_repo_get_lr_handle(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE (repo);
    return priv->repo_handle;
}

LrResult *
dnf_repo_get_lr_result(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE (repo);
    return priv->repo_result;
}

/**
 * dnf_repo_download_package:
 * @repo: a #DnfRepo instance.
 * @pkg: a #DnfPackage *instance.
 * @directory: the destination directory.
 * @state: a #DnfState.
 * @error: a #GError or %NULL..
 *
 * Downloads a package from a repo.
 *
 * Returns: the complete filename of the downloaded file
 *
 * Since: 0.1.0
 **/
gchar *
dnf_repo_download_package(DnfRepo *repo,
                          DnfPackage *pkg,
                          const gchar *directory,
                          DnfState *state,
                          GError **error)
{
    g_autoptr(GPtrArray) packages = g_ptr_array_new();
    g_autofree gchar *basename = NULL;

    g_ptr_array_add(packages, pkg);

    if (!dnf_repo_download_packages(repo, packages, directory, state, error))
        return NULL;

    /* build return value */
    basename = g_path_get_basename(dnf_package_get_location(pkg));
    return g_build_filename(directory, basename, NULL);
}

/**
 * dnf_repo_download_packages:
 * @repo: a #DnfRepo instance.
 * @packages: (element-type DnfPackage): an array of packages, must be from this repo
 * @directory: the destination directory.
 * @state: a #DnfState.
 * @error: a #GError or %NULL.
 *
 * Downloads multiple packages from a repo. The target filename will be
 * equivalent to `g_path_get_basename (dnf_package_get_location (pkg))`.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.2.3
 **/
gboolean
dnf_repo_download_packages(DnfRepo *repo,
                           GPtrArray *packages,
                           const gchar *directory,
                           DnfState *state,
                           GError **error)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    char *checksum_str = NULL;
    const unsigned char *checksum;
    gboolean ret = FALSE;
    guint i;
    int checksum_type;
    LrPackageTarget *target = NULL;
    GSList *package_targets = NULL;
    GlobalDownloadData global_data = { 0, };
    g_autoptr(GError) error_local = NULL;
    g_autofree gchar *directory_slash = NULL;

    /* ensure we reset the values from the keyfile */
    if (!dnf_repo_set_keyfile_data(repo, error))
        goto out;

    /* if nothing specified then use cachedir */
    if (directory == NULL) {
        directory_slash = g_build_filename(priv->packages, "/", NULL);
        if (!g_file_test(directory_slash, G_FILE_TEST_EXISTS)) {
            if (g_mkdir(directory_slash, 0755) != 0) {
                g_set_error(error,
                            DNF_ERROR,
                            DNF_ERROR_INTERNAL_ERROR,
                            "Failed to create %s",
                            directory_slash);
                goto out;
            }
        }
    } else {
        /* librepo uses the GNU basename() function to find out if the
         * output directory is fully specified as a filename, but
         * basename needs a trailing '/' to detect it's not a filename */
        directory_slash = g_build_filename(directory, "/", NULL);
    }

    /* is a local repo, i.e. we just need to copy */
    if (dnf_repo_is_local(repo)) {
        /* the number of packages to copy */
        dnf_state_set_number_steps(state, packages->len);

        for (i = 0; i < packages->len; i++) {
            DnfPackage *pkg = packages->pdata[i];
            DnfState *state_loop = dnf_state_get_child(state);

            dnf_package_set_repo(pkg, repo);
            if (!dnf_repo_copy_package(pkg, directory, state_loop, error))
                goto out;
            if (!dnf_state_done(state, error))
                goto out;
        }
        goto done;
    }

    global_data.download_size = dnf_package_array_get_download_size(packages);
    for (i = 0; i < packages->len; i++) {
        DnfPackage *pkg = packages->pdata[i];
        PackageDownloadData *data;

        g_debug("downloading %s to %s",
                dnf_package_get_location(pkg),
                directory_slash);

        data = g_slice_new0(PackageDownloadData);
        data->pkg = pkg;
        data->state = state;
        data->global_download_data = &global_data;

        checksum = dnf_package_get_chksum(pkg, &checksum_type);
        checksum_str = hy_chksum_str(checksum, checksum_type);

        target = lr_packagetarget_new_v2(priv->repo_handle,
                                         dnf_package_get_location(pkg),
                                         directory_slash,
                                         dnf_repo_checksum_hy_to_lr(checksum_type),
                                         checksum_str,
                                         dnf_package_get_downloadsize(pkg),
                                         dnf_package_get_baseurl(pkg),
                                         TRUE,
                                         package_download_update_state_cb,
                                         data,
                                         package_download_end_cb,
                                         mirrorlist_failure_cb,
                                         error);
        if (target == NULL)
            goto out;

        package_targets = g_slist_prepend(package_targets, target);
    }

    ret = lr_download_packages(package_targets, LR_PACKAGEDOWNLOAD_FAILFAST, &error_local);
    if (!ret) {
        if (g_error_matches(error_local,
                     LR_PACKAGE_DOWNLOADER_ERROR,
                     LRE_ALREADYDOWNLOADED)) {
            /* ignore */
            g_clear_error(&error_local);
        } else {
            if (global_data.last_mirror_failure_message) {
                g_autofree gchar *orig_message = error_local->message;
                error_local->message = g_strconcat(orig_message, "; Last error: ", global_data.last_mirror_failure_message, NULL);
            }
            g_propagate_error(error, error_local);
            error_local = NULL;
            goto out;
        }
    } 

done:
    ret = TRUE;
out:
    lr_handle_setopt(priv->repo_handle, NULL, LRO_PROGRESSCB, NULL);
    lr_handle_setopt(priv->repo_handle, NULL, LRO_PROGRESSDATA, 0xdeadbeef);
    g_free(global_data.last_mirror_failure_message);
    g_free(global_data.last_mirror_url);
    g_slist_free_full(package_targets, (GDestroyNotify)lr_packagetarget_free);
    g_free(checksum_str);
    return ret;
}

/**
 * dnf_repo_new:
 * @context: A #DnfContext instance
 *
 * Creates a new #DnfRepo.
 *
 * Returns:(transfer full): a #DnfRepo
 *
 * Since: 0.1.0
 **/
DnfRepo *
dnf_repo_new(DnfContext *context)
{
    DnfRepo *repo;
    DnfRepoPrivate *priv;
    repo = g_object_new(DNF_TYPE_REPO, NULL);
    priv = GET_PRIVATE(repo);
    priv->context = context;
    g_object_add_weak_pointer(G_OBJECT(priv->context),(void **) &priv->context);
    return DNF_REPO(repo);
}
