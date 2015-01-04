/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013-2014 Richard Hughes <richard@hughsie.com>
 *
 * Most of this code was taken from Zif, libzif/zif-repos.c
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/**
 * SECTION:hif-source
 * @short_description: Object representing a remote source.
 * @include: libhif-private.h
 * @stability: Unstable
 *
 * Sources are remote repositories of packages.
 *
 * See also: #HifSource
 */

#include "config.h"

#include <strings.h>
#include <fcntl.h>
#include <errno.h>
#include <glib/gstdio.h>
#include <hawkey/util.h>
#include <librepo/librepo.h>
#include <rpm/rpmts.h>

#include "hif-cleanup.h"
#include "hif-keyring.h"
#include "hif-source.h"
#include "hif-package.h"
#include "hif-utils.h"

typedef struct _HifSourcePrivate	HifSourcePrivate;
struct _HifSourcePrivate
{
	HifSourceEnabled enabled;
	gboolean	 gpgcheck_md;
	gboolean	 gpgcheck_pkgs;
	gchar		*gpgkey;
	gchar          **exclude_packages;
	guint		 cost;
	gchar		*filename;	/* /etc/yum.repos.d/updates.repo */
	gchar		*id;
	gchar		*location;	/* /var/cache/PackageKit/metadata/fedora */
	gchar		*location_tmp;	/* /var/cache/PackageKit/metadata/fedora.tmp */
	gchar		*packages;	/* /var/cache/PackageKit/metadata/fedora/packages */
	gchar		*packages_tmp;	/* /var/cache/PackageKit/metadata/fedora.tmp/packages */
	gchar		*keyring;	/* /var/cache/PackageKit/metadata/fedora/gpgdir */
	gchar		*keyring_tmp;	/* /var/cache/PackageKit/metadata/fedora.tmp/gpgdir */
	gchar		*pubkey;	/* /var/cache/PackageKit/metadata/fedora/repomd.pub */
	gchar		*pubkey_tmp;	/* /var/cache/PackageKit/metadata/fedora.tmp/repomd.pub */
	gint64		 timestamp_generated;	/* µs */
	gint64		 timestamp_modified;	/* µs */
	GKeyFile	*keyfile;
	GHashTable	*filenames_md;	/* key:filename */
	HifContext	*context;
	HifSourceKind	 kind;
	HyRepo		 repo;
	LrHandle	*repo_handle;
	LrResult	*repo_result;
	LrUrlVars	*urlvars;
};

#define HIF_CONFIG_GROUP_NAME			"PluginHawkey"

G_DEFINE_TYPE (HifSource, hif_source, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HIF_TYPE_SOURCE, HifSourcePrivate))

/**
 * hif_source_finalize:
 **/
static void
hif_source_finalize (GObject *object)
{
	HifSource *source = HIF_SOURCE (object);
	HifSourcePrivate *priv = GET_PRIVATE (source);

	g_free (priv->id);
	g_free (priv->filename);
	g_free (priv->gpgkey);
	g_strfreev (priv->exclude_packages);
	g_free (priv->location_tmp);
	g_free (priv->location);
	g_free (priv->packages);
	g_free (priv->packages_tmp);
	g_free (priv->keyring);
	g_free (priv->keyring_tmp);
	g_free (priv->pubkey);
	g_free (priv->pubkey_tmp);
	g_hash_table_unref (priv->filenames_md);
	g_object_unref (priv->context);
	if (priv->repo_result != NULL)
		lr_result_free (priv->repo_result);
	if (priv->repo_handle != NULL)
		lr_handle_free (priv->repo_handle);
	if (priv->repo != NULL)
		hy_repo_free (priv->repo);
	if (priv->keyfile != NULL)
		g_key_file_unref (priv->keyfile);

	G_OBJECT_CLASS (hif_source_parent_class)->finalize (object);
}

/**
 * hif_source_init:
 **/
static void
hif_source_init (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	priv->cost = 1000;
	priv->repo_handle = lr_handle_init ();
	priv->repo_result = lr_result_init ();
	priv->filenames_md = g_hash_table_new_full (g_str_hash, g_str_equal,
						    g_free, g_free);
}

/**
 * hif_source_class_init:
 **/
static void
hif_source_class_init (HifSourceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = hif_source_finalize;
	g_type_class_add_private (klass, sizeof (HifSourcePrivate));
}

/**
 * hif_source_get_id:
 * @source: a #HifSource instance.
 *
 * Gets the source ID.
 *
 * Returns: the source ID, e.g. "fedora-updates"
 *
 * Since: 0.1.0
 **/
const gchar *
hif_source_get_id (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	return priv->id;
}

/**
 * hif_source_get_location:
 * @source: a #HifSource instance.
 *
 * Gets the source location.
 *
 * Returns: the source location, e.g. "/var/cache/PackageKit/metadata/fedora"
 *
 * Since: 0.1.0
 **/
const gchar *
hif_source_get_location (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	return priv->location;
}

/**
 * hif_source_get_filename:
 * @source: a #HifSource instance.
 *
 * Gets the source filename.
 *
 * Returns: the source filename, e.g. "/etc/yum.repos.d/updates.repo"
 *
 * Since: 0.1.0
 **/
const gchar *
hif_source_get_filename (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	return priv->filename;
}

/**
 * hif_source_get_packages:
 * @source: a #HifSource instance.
 *
 * Gets the source packages location.
 *
 * Returns: the source packages location, e.g. "/var/cache/PackageKit/metadata/fedora/packages"
 *
 * Since: 0.1.0
 **/
const gchar *
hif_source_get_packages (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	return priv->packages;
}

/**
 * hif_source_substitute:
 */
static gchar *
hif_source_substitute (HifSource *source, const gchar *url)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	char *tmp;
	gchar *substituted;

	/* do a little dance so we can use g_free() rather than lr_free() */
	tmp = lr_url_substitute (url, priv->urlvars);
	substituted = g_strdup (tmp);
	lr_free (tmp);

	return substituted;
}

/**
 * hif_source_get_description:
 * @source: a #HifSource instance.
 *
 * Gets the source description.
 *
 * Returns: the source description, e.g. "Fedora 20 Updates"
 *
 * Since: 0.1.0
 **/
gchar *
hif_source_get_description (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	_cleanup_free_ gchar *tmp;

	/* is DVD */
	if (priv->kind == HIF_SOURCE_KIND_MEDIA) {
		tmp = g_key_file_get_string (priv->keyfile, "general", "name", NULL);
		if (tmp == NULL)
			return NULL;
	} else {
		tmp = g_key_file_get_string (priv->keyfile,
					     hif_source_get_id (source),
					     "name",
					     NULL);
		if (tmp == NULL)
			return NULL;
	}

	/* have to substitute things like $releasever and $basearch */
	return hif_source_substitute (source, tmp);
}

/**
 * hif_source_get_enabled:
 * @source: a #HifSource instance.
 *
 * Gets if the source is enabled.
 *
 * Returns: %HIF_SOURCE_ENABLED_PACKAGES if enabled
 *
 * Since: 0.1.0
 **/
HifSourceEnabled
hif_source_get_enabled (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	return priv->enabled;
}

/**
 * hif_source_get_cost:
 * @source: a #HifSource instance.
 *
 * Gets the source cost.
 *
 * Returns: the source cost, where 1000 is default
 *
 * Since: 0.1.0
 **/
guint
hif_source_get_cost (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	return priv->cost;
}

/**
 * hif_source_get_kind:
 * @source: a #HifSource instance.
 *
 * Gets the source kind.
 *
 * Returns: the source kind, e.g. %HIF_SOURCE_KIND_MEDIA
 *
 * Since: 0.1.0
 **/
HifSourceKind
hif_source_get_kind (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	return priv->kind;
}

/**
 * hif_source_get_exclude_packages:
 * @source: a #HifSource instance.
 *
 * Returns: (transfer none) (array zero-terminated=1): Packages which should be excluded
 *
 * Since: 0.1.9
 **/
gchar **
hif_source_get_exclude_packages	(HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	return priv->exclude_packages;
}

/**
 * hif_source_get_gpgcheck:
 * @source: a #HifSource instance.
 *
 * Gets if the packages should be signed.
 *
 * Returns: %TRUE if packages should be signed
 *
 * Since: 0.1.0
 **/
gboolean
hif_source_get_gpgcheck (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	return priv->gpgcheck_pkgs;
}

/**
 * hif_source_get_gpgcheck_md:
 * @source: a #HifSource instance.
 *
 * Gets if the metadata should be signed.
 *
 * Returns: %TRUE if metadata should be signed
 *
 * Since: 0.1.7
 **/
gboolean
hif_source_get_gpgcheck_md (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	return priv->gpgcheck_md;
}

/**
 * hif_source_get_repo:
 * @source: a #HifSource instance.
 *
 * Gets the #HyRepo backing this source.
 *
 * Returns: the #HyRepo
 *
 * Since: 0.1.0
 **/
HyRepo
hif_source_get_repo (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	return priv->repo;
}

/**
 * hif_source_is_devel:
 * @source: a #HifSource instance.
 *
 * Returns: %TRUE if the source is a development repo
 *
 * Since: 0.1.0
 **/
gboolean
hif_source_is_devel (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	if (g_str_has_suffix (priv->id, "-debuginfo"))
		return TRUE;
	if (g_str_has_suffix (priv->id, "-debug"))
		return TRUE;
	if (g_str_has_suffix (priv->id, "-development"))
		return TRUE;
	return FALSE;
}

/**
 * hif_source_is_local:
 * @source: a #HifSource instance.
 *
 * Returns: %TRUE if the source is a repo on local or media filesystem
 *
 * Since: 0.1.6
 **/
gboolean
hif_source_is_local (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);

	/* media or local */
	if (priv->kind == HIF_SOURCE_KIND_MEDIA ||
	    priv->kind == HIF_SOURCE_KIND_LOCAL)
		return TRUE;

	return FALSE;
}

/**
 * hif_source_is_source:
 * @source: a #HifSource instance.
 *
 * Returns: %TRUE if the source is a src repo
 *
 * Since: 0.1.0
 **/
gboolean
hif_source_is_source (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	if (g_str_has_suffix (priv->id, "-source"))
		return TRUE;
	return FALSE;
}

/**
 * hif_source_is_supported:
 * @source: a #HifSource instance.
 *
 * Returns: %TRUE if the source is supported
 *
 * Since: 0.1.0
 **/
gboolean
hif_source_is_supported (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	guint i;
	const gchar *valid[] = { "fedora",
				 "fedora-debuginfo",
				 "fedora-source",
				 "rawhide",
				 "rawhide-debuginfo",
				 "rawhide-source",
				 "updates",
				 "updates-debuginfo",
				 "updates-source",
				 "updates-testing",
				 "updates-testing-debuginfo",
				 "updates-testing-source",
				 NULL };
	for (i = 0; valid[i] != NULL; i++) {
		if (g_strcmp0 (priv->id, valid[i]) == 0)
			return TRUE;
	}
	return FALSE;
}

/**
 * hif_source_set_id:
 * @source: a #HifSource instance.
 * @id: the ID, e.g. "fedora-updates"
 *
 * Sets the source ID.
 *
 * Since: 0.1.0
 **/
void
hif_source_set_id (HifSource *source, const gchar *id)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	g_free (priv->id);
	priv->id = g_strdup (id);
}

/**
 * hif_source_set_location:
 * @source: a #HifSource instance.
 * @location: the path
 *
 * Sets the source location and the default packages path.
 *
 * Since: 0.1.0
 **/
void
hif_source_set_location (HifSource *source, const gchar *location)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	g_free (priv->location);
	g_free (priv->packages);
	g_free (priv->keyring);
	g_free (priv->pubkey);
	priv->location = hif_source_substitute (source, location);
	priv->packages = g_build_filename (location, "packages", NULL);
	priv->keyring = g_build_filename (location, "gpgdir", NULL);
	priv->pubkey = g_build_filename (location, "repomd.pub", NULL);
}

/**
 * hif_source_set_location_tmp:
 * @source: a #HifSource instance.
 * @location_tmp: the temp path
 *
 * Sets the source location for temporary files and the default packages path.
 *
 * Since: 0.1.0
 **/
void
hif_source_set_location_tmp (HifSource *source, const gchar *location_tmp)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	g_free (priv->location_tmp);
	g_free (priv->packages_tmp);
	g_free (priv->keyring_tmp);
	g_free (priv->pubkey_tmp);
	priv->location_tmp = hif_source_substitute (source, location_tmp);
	priv->packages_tmp = g_build_filename (location_tmp, "packages", NULL);
	priv->keyring_tmp = g_build_filename (location_tmp, "gpgdir", NULL);
	priv->pubkey_tmp = g_build_filename (location_tmp, "repomd.pub", NULL);
}

/**
 * hif_source_set_filename:
 * @source: a #HifSource instance.
 * @filename: the filename path
 *
 * Sets the source backing filename.
 *
 * Since: 0.1.0
 **/
void
hif_source_set_filename (HifSource *source, const gchar *filename)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);

	g_free (priv->filename);
	priv->filename = g_strdup (filename);
}

/**
 * hif_source_set_packages:
 * @source: a #HifSource instance.
 * @packages: the path to the package files
 *
 * Sets the source package location, typically ending in "Packages" or "packages".
 *
 * Since: 0.1.0
 **/
void
hif_source_set_packages (HifSource *source, const gchar *packages)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	g_free (priv->packages);
	priv->packages = g_strdup (packages);
}

/**
 * hif_source_set_packages_tmp:
 * @source: a #HifSource instance.
 * @packages_tmp: the path to the temporary package files
 *
 * Sets the source package location for temporary files.
 *
 * Since: 0.1.0
 **/
void
hif_source_set_packages_tmp (HifSource *source, const gchar *packages_tmp)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	g_free (priv->packages_tmp);
	priv->packages_tmp = g_strdup (packages_tmp);
}

/**
 * hif_source_set_enabled:
 * @source: a #HifSource instance.
 * @enabled: if the source is enabled
 *
 * Sets the source enabled state.
 *
 * Since: 0.1.0
 **/
void
hif_source_set_enabled (HifSource *source, HifSourceEnabled enabled)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);

	priv->enabled = enabled;

	/* packages implies metadata */
	if (priv->enabled & HIF_SOURCE_ENABLED_PACKAGES)
		priv->enabled |= HIF_SOURCE_ENABLED_METADATA;
}

/**
 * hif_source_set_cost:
 * @source: a #HifSource instance.
 * @cost: the source cost
 *
 * Sets the source cost, where 1000 is the default.
 *
 * Since: 0.1.0
 **/
void
hif_source_set_cost (HifSource *source, guint cost)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	priv->cost = cost;
}

/**
 * hif_source_set_kind:
 * @source: a #HifSource instance.
 * @kind: the #HifSourceKind
 *
 * Sets the source kind.
 *
 * Since: 0.1.0
 **/
void
hif_source_set_kind (HifSource *source, HifSourceKind kind)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	priv->kind = kind;
}

/**
 * hif_source_set_gpgcheck:
 * @source: a #HifSource instance.
 * @gpgcheck_pkgs: if the source packages should be signed
 *
 * Sets the source signed status.
 *
 * Since: 0.1.0
 **/
void
hif_source_set_gpgcheck (HifSource *source, gboolean gpgcheck_pkgs)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	priv->gpgcheck_pkgs = gpgcheck_pkgs;
}

/**
 * hif_source_set_gpgcheck_md:
 * @source: a #HifSource instance.
 * @gpgcheck_md: if the source metadata should be signed
 *
 * Sets the metadata signed status.
 *
 * Since: 0.1.7
 **/
void
hif_source_set_gpgcheck_md (HifSource *source, gboolean gpgcheck_md)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	priv->gpgcheck_md = gpgcheck_md;
}

/**
 * hif_source_set_keyfile:
 * @source: a #HifSource instance.
 * @keyfile: the #GKeyFile
 *
 * Sets the source keyfile used to create the source.
 *
 * Since: 0.1.0
 **/
void
hif_source_set_keyfile (HifSource *source, GKeyFile *keyfile)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	if (priv->keyfile != NULL)
		g_key_file_unref (priv->keyfile);
	priv->keyfile = g_key_file_ref (keyfile);
}

/**
 * hif_source_get_username_password_string:
 */
static gchar *
hif_source_get_username_password_string (const gchar *user, const gchar *pass)
{
	if (user == NULL && pass == NULL)
		return NULL;
	if (user != NULL && pass == NULL)
		return g_strdup (user);
	if (user == NULL && pass != NULL)
		return g_strdup_printf (":%s", pass);
	return g_strdup_printf ("%s:%s", user, pass);
}

/**
 * hif_source_set_keyfile_data:
 */
static gboolean
hif_source_set_keyfile_data (HifSource *source, GError **error)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	guint cost;
	_cleanup_free_ gchar *metalink = NULL;
	_cleanup_free_ gchar *mirrorlist = NULL;
	_cleanup_free_ gchar *proxy = NULL;
	_cleanup_free_ gchar *exclude_string = NULL;
	_cleanup_free_ gchar *pwd = NULL;
	_cleanup_free_ gchar *usr = NULL;
	_cleanup_free_ gchar *usr_pwd = NULL;
	_cleanup_free_ gchar *usr_pwd_proxy = NULL;
	_cleanup_strv_free_ gchar **baseurls;

	g_debug ("setting keyfile data for %s", priv->id);

	/* cost is optional */
	cost = g_key_file_get_integer (priv->keyfile, priv->id, "cost", NULL);
	if (cost != 0)
		hif_source_set_cost (source, cost);

	/* baseurl is optional */
	baseurls = g_key_file_get_string_list (priv->keyfile, priv->id, "baseurl", NULL, NULL);
	if (baseurls && !lr_handle_setopt (priv->repo_handle, error, LRO_URLS, baseurls))
		return FALSE;

	/* mirrorlist is optional */
	mirrorlist = g_key_file_get_string (priv->keyfile, priv->id, "mirrorlist", NULL);
	if (mirrorlist && !lr_handle_setopt (priv->repo_handle, error, LRO_MIRRORLIST, mirrorlist))
		return FALSE;

	/* metalink is optional */
	metalink = g_key_file_get_string (priv->keyfile, priv->id, "metalink", NULL);
	if (metalink && !lr_handle_setopt (priv->repo_handle, error, LRO_METALINKURL, metalink))
		return FALSE;

	/* file:// */
	if (baseurls != NULL && baseurls[0] != NULL &&
	    mirrorlist == NULL && metalink == NULL) {
		_cleanup_free_ gchar *url = NULL;
		url = lr_prepend_url_protocol (baseurls[0]);
		if (url != NULL && strncasecmp (url, "file://", 7) == 0) {
			if (g_strstr_len (url, -1, "$testdatadir") == NULL)
				priv->kind = HIF_SOURCE_KIND_LOCAL;
			hif_source_set_location (source, url + 7);
		}
	}

	/* set location if currently unset */
	if (priv->location == NULL) {
		_cleanup_free_ gchar *tmp = NULL;
		tmp = g_build_filename (hif_context_get_cache_dir (priv->context),
					priv->id, NULL);
		hif_source_set_location (source, tmp);
	}

	/* set temp location for remote repos */
	if (priv->kind == HIF_SOURCE_KIND_REMOTE) {
		_cleanup_string_free_ GString *tmp = NULL;
		tmp = g_string_new (priv->location);
		if (tmp->str[tmp->len - 1] == '/')
			g_string_truncate (tmp, tmp->len - 1);
		g_string_append (tmp, ".tmp");
		hif_source_set_location_tmp (source, tmp->str);
	}

	/* gpgkey is optional for gpgcheck=1, but required for repo_gpgcheck=1 */
	g_free (priv->gpgkey);
	priv->gpgkey = g_key_file_get_string (priv->keyfile, priv->id, "gpgkey", NULL);
	priv->gpgcheck_pkgs = g_key_file_get_boolean (priv->keyfile, priv->id,
							  "gpgcheck", NULL);
	priv->gpgcheck_md = g_key_file_get_boolean (priv->keyfile, priv->id,
							  "repo_gpgcheck", NULL);
	if (priv->gpgcheck_md && priv->gpgkey == NULL) {
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_FILE_INVALID,
				     "gpgkey not set, yet repo_gpgcheck=1");
		return FALSE;
	}
	if (!lr_handle_setopt (priv->repo_handle, error, LRO_GPGCHECK, priv->gpgcheck_md))
		return FALSE;

	exclude_string = g_key_file_get_string (priv->keyfile, priv->id, "exclude", NULL);
	if (exclude_string) {
		priv->exclude_packages = g_strsplit (exclude_string, " ", -1);
	}

	/* proxy is optional */
	proxy = g_key_file_get_string (priv->keyfile, priv->id, "proxy", NULL);
	if (proxy == NULL)
		proxy = g_strdup (hif_context_get_http_proxy (priv->context));
	if (!lr_handle_setopt (priv->repo_handle, error, LRO_PROXY, proxy))
		return FALSE;

	/* both parts of the proxy auth are optional */
	usr = g_key_file_get_string (priv->keyfile, priv->id, "proxy_username", NULL);
	pwd = g_key_file_get_string (priv->keyfile, priv->id, "proxy_password", NULL);
	usr_pwd_proxy = hif_source_get_username_password_string (usr, pwd);
	if (!lr_handle_setopt (priv->repo_handle, error, LRO_PROXYUSERPWD, usr_pwd_proxy))
		return FALSE;

	/* both parts of the HTTP auth are optional */
	usr = g_key_file_get_string (priv->keyfile, priv->id, "username", NULL);
	pwd = g_key_file_get_string (priv->keyfile, priv->id, "password", NULL);
	usr_pwd = hif_source_get_username_password_string (usr, pwd);
	if (!lr_handle_setopt (priv->repo_handle, error, LRO_USERPWD, usr_pwd))
		return FALSE;
	return TRUE;
}

/**
 * hif_source_setup:
 * @source: a #HifSource instance.
 * @error: a #GError or %NULL
 *
 * Sets up the source ready for use.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.1.0
 **/
gboolean
hif_source_setup (HifSource *source, GError **error)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	_cleanup_free_ gchar *basearch = NULL;
	_cleanup_free_ gchar *release = NULL;
	_cleanup_free_ gchar *testdatadir = NULL;

	basearch = g_key_file_get_string (priv->keyfile, "general", "arch", NULL);
	if (basearch == NULL)
		basearch = g_strdup (hif_context_get_base_arch (priv->context));
	if (basearch == NULL) {
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_INTERNAL_ERROR,
				     "basearch not set");
		return FALSE;
	}
	release = g_key_file_get_string (priv->keyfile, "general", "version", NULL);
	if (release == NULL)
		release = g_strdup (hif_context_get_release_ver (priv->context));
	if (release == NULL) {
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_INTERNAL_ERROR,
				     "releasever not set");
		return FALSE;
	}
	if (!lr_handle_setopt (priv->repo_handle, error, LRO_USERAGENT, "PackageKit-hawkey"))
		return FALSE;
	if (!lr_handle_setopt (priv->repo_handle, error, LRO_REPOTYPE, LR_YUMREPO))
		return FALSE;
	if (!lr_handle_setopt (priv->repo_handle, error, LRO_INTERRUPTIBLE, FALSE))
		return FALSE;
	priv->urlvars = lr_urlvars_set (priv->urlvars, "releasever", release);
	priv->urlvars = lr_urlvars_set (priv->urlvars, "basearch", basearch);
	testdatadir = hif_realpath (TESTDATADIR);
	priv->urlvars = lr_urlvars_set (priv->urlvars, "testdatadir", testdatadir);
	if (!lr_handle_setopt (priv->repo_handle, error, LRO_VARSUB, priv->urlvars))
		return FALSE;
	if (!lr_handle_setopt (priv->repo_handle, error, LRO_GNUPGHOMEDIR, priv->keyring))
		return FALSE;
	return hif_source_set_keyfile_data (source, error);
}

/**
 * hif_source_update_state_cb:
 */
static int
hif_source_update_state_cb (void *user_data,
			    gdouble total_to_download,
			    gdouble now_downloaded)
{
	gboolean ret;
	gdouble percentage;
	HifState *state = (HifState *) user_data;

	/* abort */
	if (!hif_state_check (state, NULL))
		return -1;

	/* the number of files has changed */
	if (total_to_download <= 0.01 && now_downloaded <= 0.01) {
		hif_state_reset (state);
		return 0;
	}

	/* nothing sensible */
	if (total_to_download < 0)
		return 0;

	/* set percentage */
	percentage = 100.0f * now_downloaded / total_to_download;
	ret = hif_state_set_percentage (state, percentage);
	if (ret) {
		g_debug ("update state %.0f/%.0f",
			 now_downloaded,
			 total_to_download);
	}

	return 0;
}

/**
 * hif_source_set_timestamp_modified:
 */
static gboolean
hif_source_set_timestamp_modified (HifSource *source, GError **error)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	_cleanup_free_ gchar *filename;
	_cleanup_object_unref_ GFile *file;
	_cleanup_object_unref_ GFileInfo *info;

	filename = g_build_filename (priv->location, "repodata", "repomd.xml", NULL);
	file = g_file_new_for_path (filename);
	info = g_file_query_info (file,
				  G_FILE_ATTRIBUTE_TIME_MODIFIED ","
				  G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC,
				  G_FILE_QUERY_INFO_NONE,
				  NULL,
				  error);
	if (info == NULL)
		return FALSE;
	priv->timestamp_modified = g_file_info_get_attribute_uint64 (info,
					G_FILE_ATTRIBUTE_TIME_MODIFIED) * G_USEC_PER_SEC;
	priv->timestamp_modified += g_file_info_get_attribute_uint32 (info,
					G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC);
	return TRUE;
}

/**
 * hif_source_check:
 * @source: a #HifSource instance.
 * @permissible_cache_age: The oldest cache age allowed in seconds (wall clock time); Pass %G_MAXUINT to ignore
 * @state: a #HifState instance.
 * @error: a #GError or %NULL.
 *
 * Checks the source.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_source_check (HifSource *source,
		  guint permissible_cache_age,
		  HifState *state,
		  GError **error)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	const gchar *download_list[] = { "primary",
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
	_cleanup_error_free_ GError *error_local = NULL;

	/* has the media repo vanished? */
	if (priv->kind == HIF_SOURCE_KIND_MEDIA &&
	    !g_file_test (priv->location, G_FILE_TEST_EXISTS)) {
		priv->enabled = HIF_SOURCE_ENABLED_NONE;
		return TRUE;
	}

	/* Yum metadata */
	hif_state_action_start (state, HIF_STATE_ACTION_LOADING_CACHE, NULL);
	urls[0] = priv->location;
	if (!lr_handle_setopt (priv->repo_handle, error, LRO_URLS, urls))
		return FALSE;
	if (!lr_handle_setopt (priv->repo_handle, error, LRO_LOCAL, TRUE))
		return FALSE;
	if (!lr_handle_setopt (priv->repo_handle, error, LRO_CHECKSUM, TRUE))
		return FALSE;
	if (!lr_handle_setopt (priv->repo_handle, error, LRO_YUMDLIST, download_list))
		return FALSE;
	lr_result_clear (priv->repo_result);
	if (!lr_handle_perform (priv->repo_handle, priv->repo_result, &error_local)) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_SOURCE_NOT_AVAILABLE,
			     "repodata %s was not complete: %s",
			     priv->id, error_local->message);
		return FALSE;
	}

	/* get the metadata file locations */
	if (!lr_result_getinfo (priv->repo_result, &error_local, LRR_YUM_REPO, &yum_repo)) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "failed to get yum-repo: %s",
			     error_local->message);
		return FALSE;
	}

	/* get timestamp */
	ret = lr_result_getinfo (priv->repo_result, &error_local,
				 LRR_YUM_TIMESTAMP, &priv->timestamp_generated);
	if (!ret) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "failed to get timestamp: %s",
			     error_local->message);
		return FALSE;
	}

	/* check metadata age */
	if (permissible_cache_age != G_MAXUINT) {
		if (!hif_source_set_timestamp_modified (source, error))
			return FALSE;
		age_of_data = (g_get_real_time () - priv->timestamp_modified) / G_USEC_PER_SEC;
		if (age_of_data > permissible_cache_age) {
			g_set_error (error,
				     HIF_ERROR,
				     HIF_ERROR_INTERNAL_ERROR,
				     "cache too old: %"G_GINT64_FORMAT" > %i",
				     age_of_data, permissible_cache_age);
			return FALSE;
		}
	}

	/* create a HyRepo */
	priv->repo = hy_repo_create (priv->id);
	hy_repo_set_string (priv->repo, HY_REPO_MD_FN, yum_repo->repomd);
	tmp = lr_yum_repo_path (yum_repo, "primary");
	if (tmp != NULL) {
		hy_repo_set_string (priv->repo, HY_REPO_PRIMARY_FN, tmp);
		g_hash_table_insert (priv->filenames_md,
				     g_strdup ("primary"),
				     g_strdup (tmp));
	}
	tmp = lr_yum_repo_path (yum_repo, "filelists");
	if (tmp != NULL) {
		hy_repo_set_string (priv->repo, HY_REPO_FILELISTS_FN, tmp);
		g_hash_table_insert (priv->filenames_md,
				     g_strdup ("filelists"),
				     g_strdup (tmp));
	}
	tmp = lr_yum_repo_path (yum_repo, "updateinfo");
	if (tmp != NULL) {
		hy_repo_set_string (priv->repo, HY_REPO_UPDATEINFO_FN, tmp);
		g_hash_table_insert (priv->filenames_md,
				     g_strdup ("updateinfo"),
				     g_strdup (tmp));
	}
	tmp = lr_yum_repo_path (yum_repo, "group");
	if (tmp != NULL) {
		g_hash_table_insert (priv->filenames_md,
				     g_strdup ("group"),
				     g_strdup (tmp));
	}
	tmp = lr_yum_repo_path (yum_repo, "appstream");
	if (tmp != NULL) {
		g_hash_table_insert (priv->filenames_md,
				     g_strdup ("appstream"),
				     g_strdup (tmp));
	}
	tmp = lr_yum_repo_path (yum_repo, "appstream-icons");
	if (tmp != NULL) {
		g_hash_table_insert (priv->filenames_md,
				     g_strdup ("appstream-icons"),
				     g_strdup (tmp));
	}

	/* ensure we reset the values from the keyfile */
	if (!hif_source_set_keyfile_data (source, error))
		return FALSE;

	return TRUE;
}

/**
 * hif_source_get_filename_md:
 * @source: a #HifSource instance.
 * @md_kind: The file kind, e.g. "primary" or "updateinfo"
 *
 * Gets the filename used for a source data kind.
 *
 * Returns: the full path to the data file, %NULL otherwise
 *
 * Since: 0.1.7
 **/
const gchar *
hif_source_get_filename_md (HifSource *source, const gchar *md_kind)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	g_return_val_if_fail (md_kind != NULL, NULL);
	return g_hash_table_lookup (priv->filenames_md, md_kind);
}

/**
 * hif_source_clean:
 * @source: a #HifSource instance.
 * @error: a #GError or %NULL.
 *
 * Cleans the source by deleting all the location contents.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_source_clean (HifSource *source, GError **error)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);

	/* do not clean media or local repos */
	if (priv->kind == HIF_SOURCE_KIND_MEDIA ||
	    priv->kind == HIF_SOURCE_KIND_LOCAL)
		return TRUE;

	if (!g_file_test (priv->location, G_FILE_TEST_EXISTS))
		return TRUE;
	if (!hif_remove_recursive (priv->location, error))
		return FALSE;
	return TRUE;
}

/**
 * hif_source_add_public_key:
 *
 * This imports a repodata public key into the default librpm keyring
 **/
static gboolean
hif_source_add_public_key (HifSource *source, GError **error)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	gboolean ret;
	rpmKeyring keyring;
	rpmts ts;

	/* then import to rpmdb */
	ts = rpmtsCreate ();
	keyring = rpmtsGetKeyring (ts, 1);
	ret = hif_keyring_add_public_key (keyring, priv->pubkey_tmp, error);
	rpmKeyringFree (keyring);
	rpmtsFree (ts);
	return ret;
}

/**
 * hif_source_download_import_public_key:
 **/
static gboolean
hif_source_download_import_public_key (HifSource *source, GError **error)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	int fd;

	/* does this file already exist */
	if (g_file_test (priv->pubkey_tmp, G_FILE_TEST_EXISTS))
		return lr_gpg_import_key (priv->pubkey_tmp, priv->keyring_tmp, error);

	/* create the gpgdir location */
	if (g_mkdir_with_parents (priv->keyring_tmp, 0755) != 0) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "Failed to create %s",
			     priv->keyring_tmp);
		return FALSE;
	}

	/* set the keyring location */
	if (!lr_handle_setopt (priv->repo_handle, error,
			       LRO_GNUPGHOMEDIR, priv->keyring_tmp))
		return FALSE;

	/* download to known location */
	g_debug ("Downloading %s to %s", priv->gpgkey, priv->pubkey_tmp);
	fd = g_open (priv->pubkey_tmp, O_CLOEXEC | O_CREAT | O_RDWR, 0774);
	if (fd < 0) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "Failed to open %s: %i",
			     priv->pubkey_tmp, fd);
		return FALSE;
	}
	if (!lr_download_url (priv->repo_handle, priv->gpgkey, fd, error)) {
		g_close (fd, NULL);
		return FALSE;
	}
	if (!g_close (fd, error))
		return FALSE;

	/* import found key */
	return lr_gpg_import_key (priv->pubkey_tmp, priv->keyring_tmp, error);
}

/**
 * hif_source_update:
 * @source: a #HifSource instance.
 * @flags: #HifSourceUpdateFlags, e.g. %HIF_SOURCE_UPDATE_FLAG_FORCE
 * @state: a #HifState instance.
 * @error: a #%GError or %NULL.
 *
 * Updates the source.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_source_update (HifSource *source,
		   HifSourceUpdateFlags flags,
		   HifState *state,
		   GError **error)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	HifState *state_local;
	gboolean ret;
	gint rc;
	gint64 timestamp_new = 0;
	_cleanup_error_free_ GError *error_local = NULL;

	/* cannot change DVD contents */
	if (priv->kind == HIF_SOURCE_KIND_MEDIA) {
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_SOURCE_NOT_AVAILABLE,
				     "Cannot update read-only source");
		return FALSE;
	}

	/* Just verify existence for local */
	if (priv->kind == HIF_SOURCE_KIND_LOCAL) {
		struct stat stbuf;
		if (stat (priv->location, &stbuf) != 0) {
			int errsv = errno;
			g_set_error (error, HIF_ERROR,
				     HIF_ERROR_SOURCE_NOT_AVAILABLE,
				     "Failed to read directory '%s': %s",
				     priv->location,
				     g_strerror (errsv));
			return FALSE;
		}

		/* Otherwise, don't refresh local repos */
		return TRUE;
	}

	/* this needs to be set */
	if (priv->location_tmp == NULL) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "location_tmp not set for %s",
			     priv->id);
		return FALSE;
	}

	/* ensure we set the values from the keyfile */
	if (!hif_source_set_keyfile_data (source, error))
		return FALSE;

	/* take lock */
	ret = hif_state_take_lock (state,
				   HIF_LOCK_TYPE_METADATA,
				   HIF_LOCK_MODE_PROCESS,
				   error);
	if (!ret)
		goto out;

	/* set state */
	ret = hif_state_set_steps (state, error,
				   95, /* download */
				   5, /* check */
				   -1);
	if (!ret)
		goto out;

	/* remove the temporary space if it already exists */
	if (g_file_test (priv->location_tmp, G_FILE_TEST_EXISTS)) {
		ret = hif_remove_recursive (priv->location_tmp, error);
		if (!ret)
			goto out;
	}

	/* ensure exists */
	rc = g_mkdir_with_parents (priv->location_tmp, 0755);
	if (rc != 0) {
		ret = FALSE;
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "Failed to create %s", priv->location_tmp);
		goto out;
	}

	/* download and import public key */
	if (priv->gpgcheck_md &&
	    (g_str_has_prefix (priv->gpgkey, "https://") ||
	     g_str_has_prefix (priv->gpgkey, "file://"))) {
		g_debug ("importing public key %s", priv->gpgkey);
		ret = hif_source_download_import_public_key (source, error);
		if (!ret)
			goto out;
	}

	/* do we autoimport this into librpm */
	if (priv->gpgcheck_md &&
	    (flags & HIF_SOURCE_UPDATE_FLAG_IMPORT_PUBKEY) > 0) {
		ret = hif_source_add_public_key (source, error);
		if (!ret)
			goto out;
	}

	g_debug ("Attempting to update %s", priv->id);
	ret = lr_handle_setopt (priv->repo_handle, error,
				LRO_LOCAL, FALSE);
	if (!ret)
		goto out;
	ret = lr_handle_setopt (priv->repo_handle, error,
				LRO_DESTDIR, priv->location_tmp);
	if (!ret)
		goto out;

	/* Callback to display progress of downloading */
	state_local = hif_state_get_child (state);
	ret = lr_handle_setopt (priv->repo_handle, error,
				LRO_PROGRESSDATA, state_local);
	if (!ret)
		goto out;
	ret = lr_handle_setopt (priv->repo_handle, error,
				LRO_PROGRESSCB, hif_source_update_state_cb);
	if (!ret)
		goto out;
	lr_result_clear (priv->repo_result);
	hif_state_action_start (state_local,
				HIF_STATE_ACTION_DOWNLOAD_METADATA, NULL);
	ret = lr_handle_perform (priv->repo_handle,
				 priv->repo_result,
				 &error_local);
	if (!ret) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_CANNOT_FETCH_SOURCE,
			     "cannot update repo '%s': %s",
			     hif_source_get_id (source),
			     error_local->message);
		goto out;
	}

	/* check the newer metadata is newer */
	ret = lr_result_getinfo (priv->repo_result, &error_local,
				 LRR_YUM_TIMESTAMP, &timestamp_new);
	if (!ret) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "failed to get timestamp: %s",
			     error_local->message);
		goto out;
	}
	if ((flags & HIF_SOURCE_UPDATE_FLAG_FORCE) == 0 &&
	    timestamp_new < priv->timestamp_generated) {
		g_debug ("fresh metadata was older than what we have, ignoring");
		if (!hif_state_finished (state, error))
			return FALSE;
		goto out;
	}

	/* only simulate */
	if (flags & HIF_SOURCE_UPDATE_FLAG_SIMULATE) {
		g_debug ("simulating, so not switching to new metadata");
		ret = hif_remove_recursive (priv->location_tmp, error);
		goto out;
	}

	/* move the packages directory from the old cache to the new cache */
	if (g_file_test (priv->packages, G_FILE_TEST_EXISTS)) {
		rc = g_rename (priv->packages, priv->packages_tmp);
		if (rc != 0) {
			ret = FALSE;
			g_set_error (error,
				     HIF_ERROR,
				     HIF_ERROR_CANNOT_FETCH_SOURCE,
				     "cannot move %s to %s",
				     priv->packages, priv->packages_tmp);
			goto out;
		}
	}

	/* delete old /var/cache/PackageKit/metadata/$REPO/ */
	ret = hif_source_clean (source, error);
	if (!ret)
		goto out;

	/* rename .tmp actual name */
	rc = g_rename (priv->location_tmp, priv->location);
	if (rc != 0) {
		ret = FALSE;
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_CANNOT_FETCH_SOURCE,
			     "cannot move %s to %s",
			     priv->location_tmp, priv->location);
		goto out;
	}
	ret = lr_handle_setopt (priv->repo_handle, error,
				LRO_DESTDIR, priv->location);
	if (!ret)
		goto out;
	ret = lr_handle_setopt (priv->repo_handle, error,
				LRO_GNUPGHOMEDIR, priv->keyring);
	if (!ret)
		goto out;

	/* done */
	ret = hif_state_done (state, error);
	if (!ret)
		goto out;

	/* now setup internal hawkey stuff */
	state_local = hif_state_get_child (state);
	ret = hif_source_check (source, G_MAXUINT, state_local, error);
	if (!ret)
		goto out;

	/* done */
	ret = hif_state_done (state, error);
	if (!ret)
		goto out;
out:
	hif_state_release_locks (state);
	lr_handle_setopt (priv->repo_handle, NULL, LRO_PROGRESSCB, NULL);
	lr_handle_setopt (priv->repo_handle, NULL, LRO_PROGRESSDATA, 0xdeadbeef);
	return ret;
}

/**
 * hif_source_set_data:
 * @source: a #HifSource instance.
 * @parameter: the UTF8 key.
 * @value: the UTF8 value.
 * @error: A #GError or %NULL
 *
 * Sets data on the source.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_source_set_data (HifSource *source,
		     const gchar *parameter,
		     const gchar *value,
		     GError **error)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	g_key_file_set_string (priv->keyfile, priv->id, parameter, value);
	return TRUE;
}

/**
 * hif_source_commit:
 * @source: a #HifSource instance.
 * @error: A #GError or %NULL
 *
 * Commits data on the source, which involves saving a new .repo file.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.4
 **/
gboolean
hif_source_commit (HifSource *source, GError **error)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	_cleanup_free_ gchar *data = NULL;

	/* cannot change DVD contents */
	if (priv->kind == HIF_SOURCE_KIND_MEDIA) {
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_CANNOT_WRITE_SOURCE_CONFIG,
				     "Cannot commit to read-only media");
		return FALSE;
	}

	/* dump updated file to disk */
	data = g_key_file_to_data (priv->keyfile, NULL, error);
	if (data == NULL)
		return FALSE;
	return g_file_set_contents (priv->filename, data, -1, error);
}

/**
 * hif_source_checksum_hy_to_lr:
 **/
static LrChecksumType
hif_source_checksum_hy_to_lr (int checksum_hy)
{
	if (checksum_hy == HY_CHKSUM_MD5)
		return LR_CHECKSUM_MD5;
	if (checksum_hy == HY_CHKSUM_SHA1)
		return LR_CHECKSUM_SHA1;
	if (checksum_hy == HY_CHKSUM_SHA256)
		return LR_CHECKSUM_SHA256;
	return LR_CHECKSUM_UNKNOWN;
}

/**
 * hif_source_copy_progress_cb:
 **/
static void
hif_source_copy_progress_cb (goffset current, goffset total, gpointer user_data)
{
	HifState *state = HIF_STATE (user_data);
	hif_state_set_percentage (state, 100.0f * current / total);
}

/**
 * hif_source_copy_package:
 **/
static gboolean
hif_source_copy_package (HyPackage pkg,
			 const gchar *directory,
			 HifState *state,
			 GError **error)
{
	_cleanup_free_ gchar *basename = NULL;
	_cleanup_free_ gchar *dest = NULL;
	_cleanup_object_unref_ GFile *file_dest;
	_cleanup_object_unref_ GFile *file_source;

	/* copy the file with progress */
	file_source = g_file_new_for_path (hif_package_get_filename (pkg));
	basename = g_path_get_basename (hy_package_get_location (pkg));
	dest = g_build_filename (directory, basename, NULL);
	file_dest = g_file_new_for_path (dest);
	return g_file_copy (file_source, file_dest, G_FILE_COPY_NONE,
			    hif_state_get_cancellable (state),
			    hif_source_copy_progress_cb, state, error);
}

/**
 * hif_source_download_package:
 * @source: a #HifSource instance.
 * @pkg: a #HyPackage instance.
 * @directory: the destination directory.
 * @state: a #HifState.
 * @error: a #GError or %NULL..
 *
 * Downloads a package from a source.
 *
 * Returns: the complete filename of the downloaded file
 *
 * Since: 0.1.0
 **/
gchar *
hif_source_download_package (HifSource *source,
			     HyPackage pkg,
			     const gchar *directory,
			     HifState *state,
			     GError **error)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	char *checksum_str = NULL;
	const unsigned char *checksum;
	gboolean ret;
	gchar *loc = NULL;
	int checksum_type;
	_cleanup_error_free_ GError *error_local = NULL;
	_cleanup_free_ gchar *basename = NULL;
	_cleanup_free_ gchar *directory_slash;

	/* if nothing specified then use cachedir */
	if (directory == NULL) {
		directory_slash = g_build_filename (priv->packages, "/", NULL);
		if (!g_file_test (directory_slash, G_FILE_TEST_EXISTS)) {
			if (g_mkdir (directory_slash, 0755) != 0) {
				g_set_error (error,
					     HIF_ERROR,
					     HIF_ERROR_INTERNAL_ERROR,
					     "Failed to create %s",
					     directory_slash);
				goto out;
			}
		}
	} else {
		/* librepo uses the GNU basename() function to find out if the
		 * output directory is fully specified as a filename, but
		 * basename needs a trailing '/' to detect it's not a filename */
		directory_slash = g_build_filename (directory, "/", NULL);
	}

	/* is a local repo, i.e. we just need to copy */
	if (hif_source_is_local (source)) {
		hif_package_set_source (pkg, source);
		if (!hif_source_copy_package (pkg, directory, state, error))
			goto out;
		goto done;
	}

	ret = lr_handle_setopt (priv->repo_handle, error,
				LRO_PROGRESSDATA, state);
	if (!ret)
		goto out;
	//TODO: this doesn't actually report sane things
	ret = lr_handle_setopt (priv->repo_handle, error,
				LRO_PROGRESSCB, hif_source_update_state_cb);
	if (!ret)
		goto out;
	g_debug ("downloading %s to %s",
		 hy_package_get_location (pkg),
		 directory_slash);

	checksum = hy_package_get_chksum (pkg, &checksum_type);
	checksum_str = hy_chksum_str (checksum, checksum_type);
	hif_state_action_start (state,
				HIF_STATE_ACTION_DOWNLOAD_PACKAGES,
				hif_package_get_id (pkg));
	ret = lr_download_package (priv->repo_handle,
				  hy_package_get_location (pkg),
				  directory_slash,
				  hif_source_checksum_hy_to_lr (checksum_type),
				  checksum_str,
				  0, /* size unknown */
				  NULL, /* baseurl not required */
				  TRUE,
				  &error_local);
	if (!ret) {
		if (g_error_matches (error_local,
				     LR_PACKAGE_DOWNLOADER_ERROR,
				     LRE_ALREADYDOWNLOADED)) {
			/* ignore */
			g_clear_error (&error_local);
		} else {
			g_set_error (error,
				     HIF_ERROR,
				     HIF_ERROR_INTERNAL_ERROR,
				     "cannot download %s to %s: %s",
				     hy_package_get_location (pkg),
				     directory_slash,
				     error_local->message);
			goto out;
		}
	}
done:
	/* build return value */
	basename = g_path_get_basename (hy_package_get_location (pkg));
	loc = g_build_filename (directory_slash, basename, NULL);
out:
	lr_handle_setopt (priv->repo_handle, NULL, LRO_PROGRESSCB, NULL);
	lr_handle_setopt (priv->repo_handle, NULL, LRO_PROGRESSDATA, 0xdeadbeef);
	hy_free (checksum_str);
	return loc;
}

/**
 * hif_source_new:
 * @context: A #HifContext instance
 *
 * Creates a new #HifSource.
 *
 * Returns: (transfer full): a #HifSource
 *
 * Since: 0.1.0
 **/
HifSource *
hif_source_new (HifContext *context)
{
	HifSource *source;
	HifSourcePrivate *priv;
	source = g_object_new (HIF_TYPE_SOURCE, NULL);
	priv = GET_PRIVATE (source);
	priv->context = g_object_ref (context);
	return HIF_SOURCE (source);
}
