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

#include "conf/OptionBool.hpp"

#include "dnf-context.hpp"
#include "hy-repo-private.hpp"
#include "hy-iutil-private.hpp"

#include <strings.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <glib/gstdio.h>
#include "hy-util.h"
#include <librepo/librepo.h>
#include <rpm/rpmts.h>
#include <librepo/yum.h>

#include "catch-error.hpp"
#include "dnf-keyring.h"
#include "dnf-package.h"
#include "dnf-repo.hpp"
#include "dnf-types.h"
#include "dnf-utils.h"
#include "utils/File.hpp"
#include "utils/url-encode.hpp"
#include "utils/utils.hpp"

#include <set>
#include <string>
#include <vector>

typedef struct
{
    DnfRepoEnabled   enabled;
    gchar          **exclude_packages;
    gchar           *filename;      /* /etc/yum.repos.d/updates.repo */
    gchar           *location;      /* /var/cache/PackageKit/metadata/fedora */
    gchar           *location_tmp;  /* /var/cache/PackageKit/metadata/fedora.tmp */
    gchar           *packages;      /* /var/cache/PackageKit/metadata/fedora/packages */
    gchar           *packages_tmp;  /* /var/cache/PackageKit/metadata/fedora.tmp/packages */
    gchar           *keyring;       /* /var/cache/PackageKit/metadata/fedora/gpgdir */
    gchar           *keyring_tmp;   /* /var/cache/PackageKit/metadata/fedora.tmp/gpgdir */
    gint64           timestamp_generated;   /* µs */
    gint64           timestamp_modified;    /* µs */
    GError          *last_check_error;
    GKeyFile        *keyfile;
    DnfContext      *context;               /* weak reference */
    DnfRepoKind      kind;
    libdnf::Repo    *repo;
    LrHandle        *repo_handle;
    LrResult        *repo_result;
    LrUrlVars       *urlvars;
    bool            unit_test_mode;  /* ugly hack for unit tests */
} DnfRepoPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(DnfRepo, dnf_repo, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (static_cast<DnfRepoPrivate *>(dnf_repo_get_instance_private (o)))

/**
 * dnf_repo_finalize:
 **/
static void
dnf_repo_finalize(GObject *object)
{
    DnfRepo *repo = DNF_REPO(object);
    DnfRepoPrivate *priv = GET_PRIVATE(repo);

    g_free(priv->filename);
    g_strfreev(priv->exclude_packages);
    g_free(priv->location_tmp);
    g_free(priv->location);
    g_free(priv->packages);
    g_free(priv->packages_tmp);
    g_free(priv->keyring);
    g_free(priv->keyring_tmp);
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
    priv->repo = hy_repo_create("<preinit>");
    priv->repo_handle = lr_handle_init();
    priv->repo_result = lr_result_init();
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
    return libdnf::repoGetImpl(priv->repo)->id.c_str();
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
 * dnf_repo_get_public_keys:
 * @repo: a #DnfRepo instance.
 *
 * Gets the public key location.
 *
 * Returns: (transfer full)(array zero-terminated=1): The repo public key location, e.g. "/var/cache/PackageKit/metadata/fedora/repomd.pub"
 *
 * Since: 0.8.2
 **/
gchar **
dnf_repo_get_public_keys(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    const auto & keys = priv->repo->getConfig()->gpgkey().getValue();
    gchar **ret = g_new0(gchar *, keys.size() + 1);
    for (size_t i = 0; i < keys.size(); ++i) {
        g_autofree gchar *key_bn = g_path_get_basename(keys[i].c_str());
        ret[i] = g_build_filename(priv->location, key_bn, NULL);
    }
    return ret;
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
 * dnf_repo_get_timestamp_generated:
 * @repo: a #DnfRepo instance.
 *
 * Returns: Time in seconds since the epoch (UTC)
 **/
guint64
dnf_repo_get_timestamp_generated (DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return priv->timestamp_generated;
}

/**
 * dnf_repo_get_n_solvables:
 * @repo: a #DnfRepo instance.
 *
 * Returns: Number of packages in the repo
 **/
guint
dnf_repo_get_n_solvables (DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return libdnf::repoGetImpl(priv->repo)->libsolvRepo->nsolvables;
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
    return !priv->repo->getConfig()->skip_if_unavailable().getValue();
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
    return repoGetImpl(priv->repo)->conf->cost().getValue();
}

/**
 * dnf_repo_get_module_hotfixes:
 * @repo: a #DnfRepo instance.
 *
 * Gets the repo hotfixes flag for module RPMs filtering.
 *
 * Returns: the repo module_hotfixes flag, where false is default
 *
 * Since: 0.16.1
 **/
gboolean
dnf_repo_get_module_hotfixes(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return priv->repo->getConfig()->module_hotfixes().getValue();
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
    return priv->repo->getConfig()->gpgcheck().getValue();
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
    return priv->repo->getConfig()->repo_gpgcheck().getValue();
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
 * dnf_repo_get_metadata_expire:
 * @repo: a #DnfRepo instance.
 *
 * Gets the metadata_expire time for the repo
 *
 * Returns: the metadata_expire time in seconds
 */
guint
dnf_repo_get_metadata_expire(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    return priv->repo->getConfig()->metadata_expire().getValue();
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
    auto repoId = priv->repo->getId().c_str();
    if (g_str_has_suffix(repoId, "-debuginfo"))
        return TRUE;
    if (g_str_has_suffix(repoId, "-debug"))
        return TRUE;
    if (g_str_has_suffix(repoId, "-development"))
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
 * dnf_repo_is_source:
 * @repo: a #DnfRepo instance.
 *
 * Returns: %TRUE if the repo is a source repo
 *
 * Since: 0.1.0
 **/
gboolean
dnf_repo_is_source(DnfRepo *repo)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    if (g_str_has_suffix(priv->repo->getId().c_str(), "-source"))
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
    libdnf::repoGetImpl(priv->repo)->id = id;
    libdnf::repoGetImpl(priv->repo)->conf->name().set(libdnf::Option::Priority::RUNTIME, id);
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
    priv->location = dnf_repo_substitute(repo, location);
    priv->packages = g_build_filename(location, "packages", NULL);
    priv->keyring = g_build_filename(location, "gpgdir", NULL);
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
    priv->location_tmp = dnf_repo_substitute(repo, location_tmp);
    priv->packages_tmp = g_build_filename(location_tmp, "packages", NULL);
    priv->keyring_tmp = g_build_filename(location_tmp, "gpgdir", NULL);
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
    priv->repo->getConfig()->skip_if_unavailable().set(libdnf::Option::Priority::RUNTIME, !required);
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
    repoGetImpl(priv->repo)->conf->cost().set(libdnf::Option::Priority::RUNTIME, cost);
}

/**
 * dnf_repo_set_module_hotfixes:
 * @repo: a #DnfRepo instance.
 * @module_hotfixes: hotfixes flag for module RPMs filtering
 *
 * Sets the repo hotfixes flag for module RPMs filtering.
 *
 * Since: 0.16.1
 **/
void
dnf_repo_set_module_hotfixes(DnfRepo *repo, gboolean module_hotfixes)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    priv->repo->getConfig()->module_hotfixes().set(libdnf::Option::Priority::RUNTIME, module_hotfixes);
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
    priv->repo->getConfig()->gpgcheck().set(libdnf::Option::Priority::RUNTIME, gpgcheck_pkgs);
}

/**
 * dnf_repo_set_skip_if_unavailiable:
 * @repo: a #DnfRepo instance.
 * @skip_if_unavailable: whether the repo should be skipped if unavailable
 *
 * Sets the repo skip_if_unavailable status
 *
 * Since: 0.7.3
 **/
void
dnf_repo_set_skip_if_unavailable(DnfRepo *repo, gboolean skip_if_unavailable)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    priv->repo->getConfig()->skip_if_unavailable().set(libdnf::Option::Priority::RUNTIME, skip_if_unavailable);
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
    priv->repo->getConfig()->repo_gpgcheck().set(libdnf::Option::Priority::RUNTIME, gpgcheck_md);
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
 * dnf_repo_set_metadata_expire:
 * @repo: a #DnfRepo instance.
 * @metadata_expire: the expected expiry time for metadata
 *
 * Sets the metadata_expire time, which defaults to 48h.
 **/
void
dnf_repo_set_metadata_expire(DnfRepo *repo, guint metadata_expire)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    priv->repo->getConfig()->metadata_expire().set(libdnf::Option::Priority::RUNTIME, metadata_expire);
}

/**
* @brief Format user password string
*
* Returns user and password in user:password form. If encode is True,
* special characters in user and password are URL encoded.
*
* @param user Username
* @param passwd Password
* @param encode If quote is True, special characters in user and password are URL encoded.
* @return User and password in user:password form
*/
static std::string formatUserPassString(const std::string & user, const std::string & passwd, bool encode)
{
    if (encode)
        return libdnf::urlEncode(user) + ":" + libdnf::urlEncode(passwd);
    else
        return user + ":" + passwd;
}

/* Resets repository configuration options previously readed from repository
 * configuration file to initial state. */
static void
dnf_repo_conf_reset(libdnf::ConfigRepo &config)
{
    for (auto & item : config.optBinds()) {
        auto & itemOption = item.second;
        if (itemOption.getPriority() == libdnf::Option::Priority::REPOCONFIG) {
            itemOption.getOption().reset();
        }
    }
}

/* Loads repository configuration from GKeyFile */
static void
dnf_repo_conf_from_gkeyfile(DnfRepo *repo, const char *repoId, GKeyFile *gkeyFile)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    auto & config = *priv->repo->getConfig();

    // Reset to the initial state before reloading the configuration.
    dnf_repo_conf_reset(config);

    g_autoptr(GError) error_local = NULL;
    g_auto(GStrv) keys = g_key_file_get_keys(gkeyFile, repoId, NULL, &error_local);
    if (keys == NULL) {
        g_debug("Failed to load configuration for repo id \"%s\": %s", repoId, error_local->message);
        return;
    }

    for (auto it = keys; *it != NULL; ++it) {
        auto key = *it;
        g_autofree gchar *str = g_key_file_get_value(gkeyFile, repoId, key, NULL);
        if (str) {
            auto value = libdnf::string::trim(str);
            if (value.length() > 1 && value.front() == value.back() &&
                (value.front() == '"' || value.front() == '\'')) {
                value.erase(--value.end());
                value.erase(value.begin());
            }
            try {
                auto & optionItem = config.optBinds().at(key);

                if (dynamic_cast<libdnf::OptionStringList*>(&optionItem.getOption()) ||
                    dynamic_cast<libdnf::OptionChild<libdnf::OptionStringList>*>(&optionItem.getOption())
                ) {

                    // reload list option from gKeyFile using g_key_file_get_string_list()
                    // g_key_file_get_value () is problematic for multiline lists
                    g_auto(GStrv) list = g_key_file_get_string_list(gkeyFile, repoId, key, NULL, NULL);
                    if (list) {
                        // list can be ['value1', 'value2, value3'] therefore we first join
                        // to have 'value1, value2, value3'
                        g_autofree gchar * tmp_strval = g_strjoinv(",", list);

                        // Substitute vars.
                        g_autofree gchar *subst_value = dnf_repo_substitute(repo, tmp_strval);

                        if (strcmp(key, "baseurl") == 0 && strstr(tmp_strval, "file://$testdatadir") != NULL) {
                            priv->unit_test_mode = true;
                        }

                        try {
                            optionItem.newString(libdnf::Option::Priority::REPOCONFIG, subst_value);
                        } catch (const std::exception & ex) {
                            g_debug("Invalid configuration value: %s = %s in %s; %s", key, subst_value, repoId, ex.what());
                        }
                    }

                } else {
                    // process other (non list) options

                    // Substitute vars.
                    g_autofree gchar *subst_value = dnf_repo_substitute(repo, value.c_str());

                    try {
                        optionItem.newString(libdnf::Option::Priority::REPOCONFIG, subst_value);
                    } catch (const std::exception & ex) {
                        g_debug("Invalid configuration value: %s = %s in %s; %s", key, subst_value, repoId, ex.what());
                    }

                }

            } catch (const std::exception &) {
                g_debug("Unknown configuration option: %s = %s in %s", key, value.c_str(), repoId);
            }
        }
    }
}

static void
dnf_repo_apply_setopts(libdnf::ConfigRepo &config, const char *repoId)
{
    // apply repository setopts
    auto & optBinds = config.optBinds();
    for (const auto & setopt : libdnf::getGlobalSetopts()) {
        auto last_dot_pos = setopt.key.rfind('.');
        if (last_dot_pos == std::string::npos) {
            continue;
        }
        auto repo_pattern = setopt.key.substr(0, last_dot_pos);
        if (fnmatch(repo_pattern.c_str(), repoId, FNM_CASEFOLD) == 0) {
            auto key = setopt.key.substr(last_dot_pos+1);
            try {
                auto & optionItem = optBinds.at(key);
                try {
                    optionItem.newString(setopt.priority, setopt.value);
                } catch (const std::exception & ex) {
                    g_warning("dnf_repo_apply_setopt: Invalid configuration value: %s = %s; %s", setopt.key.c_str(), setopt.value.c_str(), ex.what());
                }
            } catch (const std::exception &) {
                g_warning("dnf_repo_apply_setopt: Unknown configuration option: %s in %s = %s", key.c_str(), setopt.key.c_str(), setopt.value.c_str());
            }
        }
    }
}

/* Initialize (or potentially reset) repo & LrHandle from keyfile values. */
static gboolean
dnf_repo_set_keyfile_data(DnfRepo *repo, gboolean reloadFromGKeyFile, GError **error)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    std::string tmp_str;
    const char *tmp_cstr;

    auto repoId = priv->repo->getId().c_str();
    g_debug("setting keyfile data for %s", repoId);

    auto conf = priv->repo->getConfig();

    // Reload repository configuration from keyfile.
    if (reloadFromGKeyFile) {
        dnf_repo_conf_from_gkeyfile(repo, repoId, priv->keyfile);
        dnf_repo_apply_setopts(*conf, repoId);
    }

    if (dnf_context_get_cache_dir(priv->context))
        conf->basecachedir().set(libdnf::Option::Priority::REPOCONFIG, dnf_context_get_cache_dir(priv->context));

    /* baseurl is optional; if missing, unset it */
    g_auto(GStrv) baseurls = NULL;
    auto & repoBaseurls = conf->baseurl().getValue();
    if (!repoBaseurls.empty()){
        auto len = repoBaseurls.size();
        baseurls = g_new0(char *, len + 1);
        for (size_t i = 0; i < len; ++i) {
            baseurls[i] = g_strdup(repoBaseurls[i].c_str());
        }
    }
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_URLS, baseurls))
        return FALSE;

    const char *mirrorlisturl = NULL;
    const char *metalinkurl = NULL;

    /* the "mirrorlist" entry could be either a real mirrorlist, or a metalink entry */
    tmp_cstr = conf->mirrorlist().empty() ? NULL : conf->mirrorlist().getValue().c_str();
    if (tmp_cstr) {
        if (strstr(tmp_cstr, "metalink"))
            metalinkurl = tmp_cstr;
        else /* it really is a mirrorlist */
            mirrorlisturl = tmp_cstr;
    }

    /* let "metalink" entry override metalink-as-mirrorlist entry */
    tmp_cstr = conf->metalink().empty() ? NULL : conf->metalink().getValue().c_str();
    if (tmp_cstr)
        metalinkurl = tmp_cstr;

    /* now set the final values (or unset them) */
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_MIRRORLISTURL, mirrorlisturl))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_METALINKURL, metalinkurl))
        return FALSE;

    /* file:// */
    if (baseurls != NULL && baseurls[0] != NULL &&
        mirrorlisturl == NULL && metalinkurl == NULL) {
        g_autofree gchar *url = NULL;
        url = lr_prepend_url_protocol(baseurls[0]);
        if (url != NULL && strncasecmp(url, "file://", 7) == 0) {
            if (!priv->unit_test_mode) {
                priv->kind = DNF_REPO_KIND_LOCAL;
            }
            g_free(priv->location);
            g_free(priv->keyring);
            priv->location = dnf_repo_substitute(repo, url + 7);
            priv->keyring = g_build_filename(url + 7, "gpgdir", NULL);
        }
    }

    /* set location if currently unset */
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_LOCAL, 0L))
        return FALSE;

    /* set repos cache dir */
    g_autofree gchar *cache_path = NULL;
    /* make each repo's cache directory name has releasever and basearch as its suffix */
    g_autofree gchar *cache_file_name  = g_strjoin("-", repoId,
                                                   dnf_context_get_release_ver(priv->context),
                                                   dnf_context_get_base_arch(priv->context), NULL);

    cache_path = g_build_filename(dnf_context_get_cache_dir(priv->context), cache_file_name, NULL);
    if (priv->packages == NULL) {
	    g_autofree gchar *packages_path = g_build_filename(cache_path, "packages", NULL);
	    dnf_repo_set_packages(repo, packages_path);
    }
    if (priv->location == NULL) {
        dnf_repo_set_location(repo, cache_path);
    }

    /* set temp location used for updating remote repos */
    if (priv->kind == DNF_REPO_KIND_REMOTE) {
        g_autoptr(GString) tmp = NULL;
        tmp = g_string_new(priv->location);
        if (tmp->len > 0 && tmp->str[tmp->len - 1] == '/')
            g_string_truncate(tmp, tmp->len - 1);
        g_string_append(tmp, ".tmp");
        dnf_repo_set_location_tmp(repo, tmp->str);
    }

    /* gpgkey is optional for gpgcheck=1, but required for repo_gpgcheck=1 */
    auto repo_gpgcheck = conf->repo_gpgcheck().getValue();
    if (repo_gpgcheck && conf->gpgkey().getValue().empty()) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_FILE_INVALID,
                            "gpgkey not set, yet repo_gpgcheck=1");
        return FALSE;
    }

    /* XXX: setopt() expects a long, so we need a long on the stack */
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_GPGCHECK, (long)repo_gpgcheck))
        return FALSE;

    // Sync priv->exclude_packages
    g_strfreev(priv->exclude_packages);
    auto & repoExcludepkgs = conf->excludepkgs().getValue();
    if (!repoExcludepkgs.empty()) {
        auto len = repoExcludepkgs.size();
        priv->exclude_packages = g_new0(char *, len + 1);
        for (size_t i = 0; i < len; ++i) {
            priv->exclude_packages[i] = g_strdup(repoExcludepkgs[i].c_str());
        }
    } else {
        priv->exclude_packages = NULL;
    }

    auto minrate = conf->minrate().getValue();
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_LOWSPEEDLIMIT, static_cast<long>(minrate)))
        return FALSE;

    auto maxspeed = conf->throttle().getValue();
    if (maxspeed > 0 && maxspeed <= 1)
        maxspeed *= conf->bandwidth().getValue();
    if (maxspeed != 0 && maxspeed < minrate) {
        g_set_error_literal(error, DNF_ERROR, DNF_ERROR_FILE_INVALID,
                            "Maximum download speed is lower than minimum. "
                            "Please change configuration of minrate or throttle");
        return FALSE;
    }
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_MAXSPEED, static_cast<int64_t>(maxspeed)))
        return FALSE;

    long timeout = conf->timeout().getValue();
    if (timeout > 0) {
        if (!lr_handle_setopt(priv->repo_handle, error, LRO_CONNECTTIMEOUT, timeout))
            return FALSE;
        if (!lr_handle_setopt(priv->repo_handle, error, LRO_LOWSPEEDTIME, timeout))
            return FALSE;
    } else {
        if (!lr_handle_setopt(priv->repo_handle, error, LRO_CONNECTTIMEOUT, LRO_CONNECTTIMEOUT_DEFAULT))
            return FALSE;
        if (!lr_handle_setopt(priv->repo_handle, error, LRO_LOWSPEEDTIME, LRO_LOWSPEEDTIME_DEFAULT))
            return FALSE;
    }

    tmp_str = conf->proxy().getValue();
    tmp_cstr = tmp_str.empty() ? dnf_context_get_http_proxy(priv->context) : tmp_str.c_str();
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_PROXY, tmp_cstr))
        return FALSE;

    // setup proxy authorization method
    auto proxyAuthMethods = libdnf::Repo::Impl::stringToProxyAuthMethods(conf->proxy_auth_method().getValue());
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_PROXYAUTHMETHODS, static_cast<long>(proxyAuthMethods)))
        return FALSE;

    // setup proxy username and password
    tmp_cstr = NULL;
    if (!conf->proxy_username().empty()) {
        tmp_str = conf->proxy_username().getValue();
        if (!tmp_str.empty()) {
            if (conf->proxy_password().empty()) {
                g_set_error(error, DNF_ERROR, DNF_ERROR_FILE_INVALID,
                            "repo '%s': 'proxy_username' is set but not 'proxy_password'", repoId);
                return FALSE;
            }
            tmp_str = formatUserPassString(tmp_str, conf->proxy_password().getValue(), true);
            tmp_cstr = tmp_str.c_str();
        }
    }
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_PROXYUSERPWD, tmp_cstr))
        return FALSE;

    // setup username and password
    tmp_cstr = NULL;
    tmp_str = conf->username().getValue();
    if (!tmp_str.empty()) {
        // TODO Use URL encoded form, needs support in librepo
        tmp_str = formatUserPassString(tmp_str, conf->password().getValue(), false);
        tmp_cstr = tmp_str.c_str();
    }
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_USERPWD, tmp_cstr))
        return FALSE;

    auto proxy_sslverify = conf->proxy_sslverify().getValue();
    /* XXX: setopt() expects a long, so we need a long on the stack */
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_PROXY_SSLVERIFYPEER, (long)proxy_sslverify))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_PROXY_SSLVERIFYHOST, (long)proxy_sslverify))
        return FALSE;

    auto & proxy_sslcacert = conf->proxy_sslcacert().getValue();
    if (!proxy_sslcacert.empty()) {
        if (!lr_handle_setopt(priv->repo_handle, error, LRO_PROXY_SSLCACERT, proxy_sslcacert.c_str()))
            return FALSE;
    }

    auto & proxy_sslclientcert = conf->proxy_sslclientcert().getValue();
    if (!proxy_sslclientcert.empty()) {
        if (!lr_handle_setopt(priv->repo_handle, error, LRO_PROXY_SSLCLIENTCERT, proxy_sslclientcert.c_str()))
            return FALSE;
    }

    auto & proxy_sslclientkey = conf->proxy_sslclientkey().getValue();
    if (!proxy_sslclientkey.empty()) {
        if (!lr_handle_setopt(priv->repo_handle, error, LRO_PROXY_SSLCLIENTKEY, proxy_sslclientkey.c_str()))
            return FALSE;
    }

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
dnf_repo_setup(DnfRepo *repo, GError **error) try
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    DnfRepoEnabled enabled = DNF_REPO_ENABLED_NONE;
    g_autofree gchar *basearch = NULL;
    g_autofree gchar *release = NULL;

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

    /* Call libdnf::dnf_context_load_vars(priv->context); only when values not in cache.
     * But what about if variables on disk change during long running programs (PackageKit daemon)?
     * if (!libdnf::dnf_context_get_vars_cached(priv->context))
     */
    libdnf::dnf_context_load_vars(priv->context);
    for (const auto & item : libdnf::dnf_context_get_vars(priv->context))
        priv->urlvars = lr_urlvars_set(priv->urlvars, item.first.c_str(), item.second.c_str());

    if (!lr_handle_setopt(priv->repo_handle, error, LRO_VARSUB, priv->urlvars))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_GNUPGHOMEDIR, priv->keyring))
        return FALSE;

    auto repoId = priv->repo->getId().c_str();

    auto conf = priv->repo->getConfig();
    dnf_repo_conf_from_gkeyfile(repo, repoId, priv->keyfile);
    dnf_repo_apply_setopts(*conf, repoId);

    auto sslverify = conf->sslverify().getValue();
    /* XXX: setopt() expects a long, so we need a long on the stack */
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_SSLVERIFYPEER, (long)sslverify))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_SSLVERIFYHOST, (long)sslverify))
        return FALSE;

    auto & sslcacert = conf->sslcacert().getValue();
    if (!sslcacert.empty()) {
        if (!lr_handle_setopt(priv->repo_handle, error, LRO_SSLCACERT, sslcacert.c_str()))
            return FALSE;
    }

    auto & sslclientcert = conf->sslclientcert().getValue();
    if (!sslclientcert.empty()) {
        if (!lr_handle_setopt(priv->repo_handle, error, LRO_SSLCLIENTCERT, sslclientcert.c_str()))
            return FALSE;
    }

    auto & sslclientkey = conf->sslclientkey().getValue();
    if (!sslclientkey.empty()) {
        if (!lr_handle_setopt(priv->repo_handle, error, LRO_SSLCLIENTKEY, sslclientkey.c_str()))
            return FALSE;
    }

    if (!lr_handle_setopt(priv->repo_handle, error, LRO_SSLVERIFYSTATUS, conf->sslverifystatus().getValue() ? 1L : 0L))
        return FALSE;

#ifdef LRO_SUPPORTS_CACHEDIR
    /* Set cache dir */
    if (dnf_context_get_zchunk(priv->context)) {
        if(!lr_handle_setopt(priv->repo_handle, error, LRO_CACHEDIR, dnf_context_get_cache_dir(priv->context)))
           return FALSE;
    }
#endif

    if (conf->enabled().getValue())
        enabled |= DNF_REPO_ENABLED_PACKAGES;

    /* enabled_metadata is optional */
    if (conf->enabled_metadata().getPriority() != libdnf::Option::Priority::DEFAULT) {
        try {
            if (libdnf::OptionBool(false).fromString(conf->enabled_metadata().getValue()))
                enabled |= DNF_REPO_ENABLED_METADATA;
        } catch (const std::exception & ex) {
            g_warning("Config error in section \"%s\" key \"%s\": %s", repoId, "enabled_metadata", ex.what());
        }
    } else {
        g_autofree gchar *basename = g_path_get_basename(priv->filename);
        /* special case the satellite and subscription manager repo */
        if (g_strcmp0(basename, "redhat.repo") == 0)
            enabled |= DNF_REPO_ENABLED_METADATA;
    }

    dnf_repo_set_enabled(repo, enabled);

    return dnf_repo_set_keyfile_data(repo, FALSE, error);
} CATCH_TO_GERROR(FALSE)

typedef struct
{
    DnfState *state;
    gchar *last_mirror_url;
    gchar *last_mirror_failure_message;
} RepoUpdateData;

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
    auto updatedata = static_cast<RepoUpdateData *>(user_data);
    DnfState *state = updatedata->state;

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
    auto repoImpl = libdnf::repoGetImpl(priv->repo);

    std::vector<const char *> download_list = {"primary", "group", "updateinfo", "appstream",
        "appstream-icons", "modules"};
    /* This one is huge, and at least rpm-ostree jigdo mode doesn't require it.
     * https://github.com/projectatomic/rpm-ostree/issues/1127
     */
    if (dnf_context_get_enable_filelists(priv->context))
        download_list.push_back("filelists");
    for (const auto & item : repoImpl->additionalMetadata) {
        download_list.push_back(item.c_str());
    }
    download_list.push_back(NULL);
    gboolean ret;
    LrYumRepo *yum_repo;
    const gchar *urls[] = { "", NULL };
    gint64 age_of_data; /* in seconds */
    g_autoptr(GError) error_local = NULL;
    guint metadata_expire;
    guint valid_time_allowed;

    /* has the media repo vanished? */
    if (priv->kind == DNF_REPO_KIND_MEDIA &&
        !g_file_test(priv->location, G_FILE_TEST_EXISTS)) {
        if (!dnf_repo_get_required(repo))
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
        if (!dnf_repo_get_required(repo))
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
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_YUMDLIST, download_list.data()))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_MIRRORLISTURL, NULL))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_METALINKURL, NULL))
        return FALSE;
    if (!lr_handle_setopt(priv->repo_handle, error, LRO_GNUPGHOMEDIR, priv->keyring))
        return FALSE;
    lr_result_clear(priv->repo_result);
    if (!lr_handle_perform(priv->repo_handle, priv->repo_result, &error_local)) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_REPO_NOT_AVAILABLE,
                    "repodata %s was not complete: %s",
                    priv->repo->getId().c_str(), error_local->message);
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
    repoImpl->repomdFn = yum_repo->repomd;

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

        /* choose a lower value between cache and metadata_expire for expired checking */
        metadata_expire = dnf_repo_get_metadata_expire(repo);
        valid_time_allowed = (metadata_expire <= permissible_cache_age ? metadata_expire : permissible_cache_age);

        if (age_of_data > valid_time_allowed) {
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_INTERNAL_ERROR,
                        "cache too old: %" G_GINT64_FORMAT" > %i",
                        age_of_data, valid_time_allowed);
            return FALSE;
        }
    }

    /* init newRepo */
    auto newRepo = hy_repo_create(priv->repo->getId().c_str());
    auto newRepoImpl = libdnf::repoGetImpl(newRepo);

    // "additionalMetadata" are not part of the config. They are filled during runtime
    // (eg. by plugins) and must be kept.
    newRepoImpl->additionalMetadata = repoImpl->additionalMetadata;

    hy_repo_free(priv->repo);
    priv->repo = newRepo;
    newRepoImpl->repomdFn = yum_repo->repomd;
    for (auto *elem = yum_repo->paths; elem; elem = g_slist_next(elem)) {
        if (elem->data) {
            auto yumrepopath = static_cast<LrYumRepoPath *>(elem->data);
            newRepoImpl->metadataPaths[yumrepopath->type] = yumrepopath->path;
        }
    }
    /* ensure we reset the values from the keyfile */
    if (!dnf_repo_set_keyfile_data(repo, TRUE, error))
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
               GError **error) try
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
} CATCH_TO_GERROR(FALSE)


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
    if (priv->repo) {
        auto repoImpl = libdnf::repoGetImpl(priv->repo);
        auto & filename = repoImpl->getMetadataPath(md_kind);
        return filename.empty() ? nullptr : filename.c_str();
    } else
        return nullptr;
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
dnf_repo_clean(DnfRepo *repo, GError **error) try
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
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_repo_add_public_key:
 *
 * This imports a repodata public key into the default librpm keyring
 **/
static gboolean
dnf_repo_add_public_key(DnfRepo *repo,
                        const char *tmp_path,
                        GError **error)
{
    gboolean ret;
    rpmKeyring keyring;
    rpmts ts;

    /* then import to rpmdb */
    ts = rpmtsCreate();
    keyring = rpmtsGetKeyring(ts, 1);
    ret = dnf_keyring_add_public_key(keyring, tmp_path, error);
    rpmKeyringFree(keyring);
    rpmtsFree(ts);
    return ret;
}

/**
 * dnf_repo_download_import_public_keys:
 **/
static gboolean
dnf_repo_download_import_public_key(DnfRepo *repo,
                                    const char *gpgkey,
                                    const char *key_tmp,
                                    GError **error)
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    int fd;
    g_autoptr(GError) error_local = NULL;

    /* does this file already exist */
    if (g_file_test(key_tmp, G_FILE_TEST_EXISTS))
        return lr_gpg_import_key(key_tmp, priv->keyring_tmp, error);

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
    g_debug("Downloading %s to %s", gpgkey, key_tmp);
    fd = g_open(key_tmp, O_CLOEXEC | O_CREAT | O_RDWR, 0774);
    if (fd < 0) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "Failed to open %s: %i",
                    key_tmp, fd);
        return FALSE;
    }
    if (!lr_download_url(priv->repo_handle, gpgkey, fd, &error_local)) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_CANNOT_FETCH_SOURCE,
                    "Failed to download gpg key for repo '%s': %s",
                    dnf_repo_get_id(repo),
                    error_local->message);
        g_close(fd, NULL);
        return FALSE;
    }
    if (!g_close(fd, error))
        return FALSE;

    /* import found key */
    return lr_gpg_import_key(key_tmp, priv->keyring_tmp, error);
}

static int
repo_mirrorlist_failure_cb(void *user_data,
                           const char *message,
                           const char *url,
                           const char *metadata)
{
    auto data = static_cast<RepoUpdateData *>(user_data);

    if (data->last_mirror_url)
        goto out;

    data->last_mirror_url = g_strdup(url);
    data->last_mirror_failure_message = g_strdup(message);
 out:
    return LR_CB_OK;
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
                GError **error) try
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    DnfState *state_local;
    gboolean ret;
    gint rc;
    gint64 timestamp_new = 0;
    g_autoptr(GError) error_local = NULL;
    RepoUpdateData updatedata = { 0, };

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
                    priv->repo->getId().c_str());
        return FALSE;
    }

    /* ensure we set the values from the keyfile */
    if (!dnf_repo_set_keyfile_data(repo, TRUE, error))
        return FALSE;

    auto repoImpl = libdnf::repoGetImpl(priv->repo);

    /* countme support */
    repoImpl->addCountmeFlag(priv->repo_handle);

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

    state_local = dnf_state_get_child(state);
    dnf_state_action_start(state_local,
                           DNF_STATE_ACTION_DOWNLOAD_METADATA, NULL);

    try {
        if (repoImpl->isInSync()) {
            /* reset timestamp */
            ret = utimes(repoImpl->repomdFn.c_str(), NULL);
            if (ret != 0)
                g_debug("Failed to reset timestamp on repomd.xml");
            ret = dnf_state_done(state, error);
            if (!ret)
                goto out;

            /* skip check */
            ret = dnf_state_finished(state, error);
            goto out;
        }
    } catch (std::exception & ex) {
        g_debug("Failed to verify if metadata is in sync: \"%s\". Proceding with download", ex.what());
    }

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

    {
        const auto & gpgkeys = priv->repo->getConfig()->gpgkey().getValue();
        if (!gpgkeys.empty() &&
            (priv->repo->getConfig()->repo_gpgcheck().getValue() || priv->repo->getConfig()->gpgcheck().getValue())) {
            for (const auto & key : gpgkeys) {
                const char *gpgkey = key.c_str();
                g_autofree char *gpgkey_name = g_path_get_basename(gpgkey);
                g_autofree char *key_tmp = g_build_filename(priv->location_tmp, gpgkey_name, NULL);

                /* download and import public key */
                if ((g_str_has_prefix(gpgkey, "https://") ||
                    g_str_has_prefix(gpgkey, "file://"))) {
                    g_debug("importing public key %s", gpgkey);

                    ret = dnf_repo_download_import_public_key(repo, gpgkey, key_tmp, error);
                    if (!ret)
                        goto out;
                }

                /* do we autoimport this into librpm? */
                if ((flags & DNF_REPO_UPDATE_FLAG_IMPORT_PUBKEY) > 0) {
                    ret = dnf_repo_add_public_key(repo, key_tmp, error);
                    if (!ret)
                        goto out;
                }
            }
        }
    }

    g_debug("Attempting to update %s", priv->repo->getId().c_str());
    ret = lr_handle_setopt(priv->repo_handle, error,
                           LRO_LOCAL, 0L);
    if (!ret)
        goto out;
    ret = lr_handle_setopt(priv->repo_handle, error,
                           LRO_DESTDIR, priv->location_tmp);
    if (!ret)
        goto out;

    /* Callback to display progress of downloading */
    updatedata.state = state_local;
    ret = lr_handle_setopt(priv->repo_handle, error,
                           LRO_PROGRESSDATA, &updatedata);
    if (!ret)
        goto out;
    ret = lr_handle_setopt(priv->repo_handle, error,
                           LRO_PROGRESSCB, dnf_repo_update_state_cb);
    if (!ret)
        goto out;
    /* Note this uses the same user data as PROGRESSDATA */
    ret = lr_handle_setopt(priv->repo_handle, error,
                           LRO_HMFCB, repo_mirrorlist_failure_cb);
    if (!ret)
        goto out;

    /* see dnf_repo_check_internal */
    if (!dnf_context_get_enable_filelists(priv->context)) {
        const gchar *excluded_metadata_types[] = { "filelists", NULL };
        ret = lr_handle_setopt(priv->repo_handle, error,
                               LRO_YUMBLIST, excluded_metadata_types);
        if (!ret)
            goto out;
    }

    ret = lr_handle_network_wait(priv->repo_handle, error, dnf_context_get_network_timeout_seconds(priv->context), NULL);
    if(!ret)
      goto out;
  
    lr_result_clear(priv->repo_result);
    ret = lr_handle_perform(priv->repo_handle,
                            priv->repo_result,
                            &error_local);
    if (!ret) {
        if (updatedata.last_mirror_failure_message) {
            g_autofree gchar *orig_message = error_local->message;
            error_local->message = g_strconcat(orig_message, "; Last error: ", updatedata.last_mirror_failure_message, NULL);
        }

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
        ret = dnf_move_recursive(priv->packages, priv->packages_tmp, &error_local);
        if (!ret) {
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_CANNOT_FETCH_SOURCE,
                        "cannot move %s to %s: %s",
                        priv->packages, priv->packages_tmp, error_local->message);
            goto out;
        }
    }

    /* delete old /var/cache/PackageKit/metadata/$REPO/ */
    ret = dnf_repo_clean(repo, error);
    if (!ret)
        goto out;

    /* rename .tmp actual name */
    ret = dnf_move_recursive(priv->location_tmp, priv->location, &error_local);
    if (!ret) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_CANNOT_FETCH_SOURCE,
                    "cannot move %s to %s: %s",
                    priv->location_tmp, priv->location, error_local->message);
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
    if (!ret) {
        /* remove the .tmp dir on failure */
        g_autoptr(GError) error_remove = NULL;
        if (!dnf_remove_recursive(priv->location_tmp, &error_remove))
            g_debug("Failed to remove %s: %s", priv->location_tmp, error_remove->message);
    }
    g_free(updatedata.last_mirror_failure_message);
    g_free(updatedata.last_mirror_url);
    dnf_state_release_locks(state);
    if (!lr_handle_setopt(priv->repo_handle, NULL, LRO_PROGRESSCB, NULL))
            g_debug("Failed to reset LRO_PROGRESSCB to NULL");
    if (!lr_handle_setopt(priv->repo_handle, NULL, LRO_HMFCB, NULL))
            g_debug("Failed to reset LRO_HMFCB to NULL");
    if (!lr_handle_setopt(priv->repo_handle, NULL, LRO_PROGRESSDATA, 0xdeadbeef))
            g_debug("Failed to set LRO_PROGRESSDATA to 0xdeadbeef");
    return ret;
} CATCH_TO_GERROR(FALSE)

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
                  GError **error) try
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    g_key_file_set_string(priv->keyfile, priv->repo->getId().c_str(), parameter, value);
    return TRUE;
} CATCH_TO_GERROR(FALSE)

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
dnf_repo_commit(DnfRepo *repo, GError **error) try
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
} CATCH_TO_GERROR(FALSE)

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
    auto data = static_cast<PackageDownloadData *>(user_data);
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
    auto data = static_cast<PackageDownloadData *>(user_data);

    g_slice_free(PackageDownloadData, data);

    return LR_CB_OK;
}

static int
mirrorlist_failure_cb(void *user_data,
                      const char *message,
                      const char *url)
{
    auto data = static_cast<PackageDownloadData *>(user_data);
    auto global_data = data->global_download_data;

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
                          GError **error) try
{
    g_autoptr(GPtrArray) packages = g_ptr_array_new();
    g_autofree gchar *basename = NULL;

    g_ptr_array_add(packages, pkg);

    if (!dnf_repo_download_packages(repo, packages, directory, state, error))
        return NULL;

    /* build return value */
    basename = g_path_get_basename(dnf_package_get_location(pkg));
    return g_build_filename(directory, basename, NULL);
} CATCH_TO_GERROR(NULL)

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
                           GError **error) try
{
    DnfRepoPrivate *priv = GET_PRIVATE(repo);
    gboolean ret = FALSE;
    guint i;
    GSList *package_targets = NULL;
    GlobalDownloadData global_data = { 0, };
    g_autoptr(GError) error_local = NULL;
    g_autofree gchar *directory_slash = NULL;

    /* ensure we reset the values from the keyfile */
    if (!dnf_repo_set_keyfile_data(repo, TRUE, error))
        goto out;

    /* if nothing specified then use cachedir */
    if (directory == NULL) {
        directory_slash = g_build_filename(priv->packages, "/", NULL);
        if (!g_file_test(directory_slash, G_FILE_TEST_EXISTS)) {
            if (g_mkdir_with_parents(directory_slash, 0755) != 0) {
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

    ret = lr_handle_network_wait(priv->repo_handle, error, dnf_context_get_network_timeout_seconds(priv->context), NULL);
    if(!ret)
      goto out;
  
    global_data.download_size = dnf_package_array_get_download_size(packages);
    for (i = 0; i < packages->len; i++) {
        auto pkg = static_cast<DnfPackage *>(packages->pdata[i]);
        PackageDownloadData *data;
        LrPackageTarget *target;
        const unsigned char *checksum;
        int checksum_type;
        g_autofree char *checksum_str = NULL;

        g_debug("downloading %s to %s",
                dnf_package_get_location(pkg),
                directory_slash);

        data = g_slice_new0(PackageDownloadData);
        data->pkg = pkg;
        data->state = state;
        data->global_download_data = &global_data;

        checksum = dnf_package_get_chksum(pkg, &checksum_type);
        checksum_str = hy_chksum_str(checksum, checksum_type);

        std::string encodedUrl = dnf_package_get_location(pkg);
        if (encodedUrl.find("://") == std::string::npos) {
            encodedUrl = libdnf::urlEncode(encodedUrl, "/");
        }

        target = lr_packagetarget_new_v2(priv->repo_handle,
                                         encodedUrl.c_str(),
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

    ret = TRUE;
out:
    if (!lr_handle_setopt(priv->repo_handle, NULL, LRO_PROGRESSCB, NULL))
            g_debug("Failed to reset LRO_PROGRESSCB to NULL");
    if (!lr_handle_setopt(priv->repo_handle, NULL, LRO_PROGRESSDATA, 0xdeadbeef))
            g_debug("Failed to set LRO_PROGRESSDATA to 0xdeadbeef");
    g_free(global_data.last_mirror_failure_message);
    g_free(global_data.last_mirror_url);
    g_slist_free_full(package_targets, (GDestroyNotify)lr_packagetarget_free);
    return ret;
} CATCH_TO_GERROR(FALSE)

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
    auto repo = DNF_REPO(g_object_new(DNF_TYPE_REPO, NULL));
    auto priv = GET_PRIVATE(repo);
    priv->context = context;
    g_object_add_weak_pointer(G_OBJECT(priv->context),(void **) &priv->context);
    return repo;
}

HyRepo dnf_repo_get_hy_repo(DnfRepo *repo)
{
    auto priv = GET_PRIVATE(repo);
    return priv->repo;
}

/**
 * dnf_repo_add_metadata_type_to_download:
 * @repo: a #DnfRepo instance.
 * @metadataType: a #gchar, e.g. %"filelist"
 *
 * Ask for additional repository metadata type to download.
 * Given metadata are appended to the default metadata set when repository is downloaded.
 *
 * Since: 0.24.0
 **/
void
dnf_repo_add_metadata_type_to_download(DnfRepo * repo, const gchar * metadataType)
{
    auto priv = GET_PRIVATE(repo);
    libdnf::repoGetImpl(priv->repo)->additionalMetadata.insert(metadataType);
}

/**
 * dnf_repo_get_metadata_content:
 * @repo: a #DnfRepo instance.
 * @metadataType: a #gchar, e.g. %"filelist"
 * @content: a #gpointer, a pointer to allocated memory with content of metadata file
 * @length: a #gsize
 * @error: a #GError or %NULL.
 *
 * Return content of the particular downloaded repository metadata.
 * Content of compressed metadata file is returned uncompressed.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.24.0
 **/
gboolean
dnf_repo_get_metadata_content(DnfRepo * repo, const gchar * metadataType, gpointer * content,
                              gsize * length, GError ** error) try
{
    auto path = dnf_repo_get_filename_md(repo, metadataType);
    if (!path) {
        g_set_error(error, DNF_ERROR, DNF_ERROR_FILE_NOT_FOUND,
                    "Cannot found metadata type \"%s\" for repo \"%s\"",
                    metadataType, dnf_repo_get_id(repo));
        return FALSE;
    }

    try {
        auto mdfile = libdnf::File::newFile(path);
        mdfile->open("r");
        const auto &fcontent = mdfile->getContent();
        mdfile->close();
        auto data = g_malloc(fcontent.length());
        memcpy(data, fcontent.data(), fcontent.length());
        *content = data;
        *length = fcontent.length();
        return TRUE;
    } catch (std::runtime_error & ex) {
        g_set_error(error, DNF_ERROR, DNF_ERROR_FAILED,
                    "Cannot load metadata type \"%s\" for repo \"%s\": %s",
                    metadataType, dnf_repo_get_id(repo), ex.what());
        return FALSE;
    }
} CATCH_TO_GERROR(FALSE)
