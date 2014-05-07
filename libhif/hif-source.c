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

#include <glib/gstdio.h>
#include <hawkey/util.h>
#include <librepo/librepo.h>

#include "hif-source.h"
#include "hif-package.h"
#include "hif-utils.h"

typedef struct _HifSourcePrivate	HifSourcePrivate;
struct _HifSourcePrivate
{
	gboolean	 enabled;
	gboolean	 gpgcheck;
	guint		 cost;
	gchar		*filename;	/* /etc/yum.repos.d/updates.repo */
	gchar		*id;
	gchar		*location;	/* /var/cache/PackageKit/metadata/fedora */
	gchar		*location_tmp;	/* /var/cache/PackageKit/metadata/fedora.tmp */
	gchar		*packages;	/* /var/cache/PackageKit/metadata/fedora/packages */
	gchar		*packages_tmp;	/* /var/cache/PackageKit/metadata/fedora.tmp/packages */
	gint64		 timestamp_generated;	/* µs */
	gint64		 timestamp_modified;	/* µs */
	GKeyFile	*keyfile;
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
	g_free (priv->location_tmp);
	g_free (priv->location);
	g_free (priv->packages);
	g_free (priv->packages_tmp);
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
	gchar *substituted = NULL;
	gchar *tmp;

	/* is DVD */
	if (priv->kind == HIF_SOURCE_KIND_MEDIA) {
		tmp = g_key_file_get_string (priv->keyfile, "general", "name", NULL);
		if (tmp == NULL)
			goto out;
	} else {
		tmp = g_key_file_get_string (priv->keyfile,
					     hif_source_get_id (source),
					     "name",
					     NULL);
		if (tmp == NULL)
			goto out;
	}

	/* have to substitute things like $releasever and $basearch */
	substituted = hif_source_substitute (source, tmp);
out:
	g_free (tmp);
	return substituted;
}

/**
 * hif_source_get_enabled:
 * @source: a #HifSource instance.
 *
 * Gets if the source is enabled.
 *
 * Returns: %TRUE if enabled
 *
 * Since: 0.1.0
 **/
gboolean
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
 * hif_source_get_gpgcheck:
 * @source: a #HifSource instance.
 *
 * Gets if the source is signed.
 *
 * Returns: %TRUE if packages should be signed
 *
 * Since: 0.1.0
 **/
gboolean
hif_source_get_gpgcheck (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	return priv->gpgcheck;
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
	priv->location = g_strdup (location);
	priv->packages = g_build_filename (location, "packages", NULL);
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
	priv->location_tmp = g_strdup (location_tmp);
	priv->packages_tmp = g_build_filename (location_tmp, "packages", NULL);
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
hif_source_set_enabled (HifSource *source, gboolean enabled)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	priv->enabled = enabled;
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
 * @gpgcheck: if the source should be signed
 *
 * Sets the source signed status.
 *
 * Since: 0.1.0
 **/
void
hif_source_set_gpgcheck (HifSource *source, gboolean gpgcheck)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	priv->gpgcheck = gpgcheck;
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
	gboolean ret = TRUE;
	gchar *basearch = NULL;
	gchar *release = NULL;

	basearch = g_key_file_get_string (priv->keyfile, "general", "arch", NULL);
	if (basearch == NULL)
		basearch = g_strdup (hif_context_get_base_arch (priv->context));
	if (basearch == NULL) {
		ret = FALSE;
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_INTERNAL_ERROR,
				     "basearch not set");
		goto out;
	}
	release = g_key_file_get_string (priv->keyfile, "general", "version", NULL);
	if (release == NULL)
		release = g_strdup (hif_context_get_release_ver (priv->context));
	if (basearch == NULL) {
		ret = FALSE;
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_INTERNAL_ERROR,
				     "releasever not set");
		goto out;
	}
	ret = lr_handle_setopt (priv->repo_handle, error, LRO_USERAGENT, "PackageKit-hawkey");
	if (!ret)
		goto out;
	ret = lr_handle_setopt (priv->repo_handle, error, LRO_REPOTYPE, LR_YUMREPO);
	if (!ret)
		goto out;
	priv->urlvars = lr_urlvars_set (priv->urlvars, "releasever", release);
	priv->urlvars = lr_urlvars_set (priv->urlvars, "basearch", basearch);
	ret = lr_handle_setopt (priv->repo_handle, error, LRO_VARSUB, priv->urlvars);
	if (!ret)
		goto out;
out:
	g_free (basearch);
	g_free (release);
	return ret;
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
	gboolean ret = TRUE;
	gchar *filename;
	GFile *file;
	GFileInfo *info;

	filename = g_build_filename (priv->location, "repodata", "repomd.xml", NULL);
	file = g_file_new_for_path (filename);
	info = g_file_query_info (file,
				  G_FILE_ATTRIBUTE_TIME_MODIFIED ","
				  G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC,
				  G_FILE_QUERY_INFO_NONE,
				  NULL,
				  error);
	if (info == NULL) {
		ret = FALSE;
		goto out;
	}
	priv->timestamp_modified = g_file_info_get_attribute_uint64 (info,
					G_FILE_ATTRIBUTE_TIME_MODIFIED) * G_USEC_PER_SEC;
	priv->timestamp_modified += g_file_info_get_attribute_uint32 (info,
					G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC);
out:
	g_free (filename);
	g_object_unref (file);
	if (info != NULL)
		g_object_unref (info);
	return ret;
}

/**
 * hif_source_check:
 * @source: a #HifSource instance.
 * @permissible_cache_age: The oldest cache age allowed in seconds
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
					 NULL};
	const gchar *tmp;
	gboolean ret = TRUE;
	GError *error_local = NULL;
	LrYumRepo *yum_repo;
	const gchar *urls[] = { "", NULL };
	gint64 age_of_data; /* in seconds */

	/* has the media repo vanished? */
	if (priv->kind == HIF_SOURCE_KIND_MEDIA &&
	    !g_file_test (priv->location, G_FILE_TEST_EXISTS)) {
		priv->enabled = FALSE;
		goto out;
	}

	/* Yum metadata */
	hif_state_action_start (state, HIF_STATE_STATUS_LOADING_CACHE, NULL);
	urls[0] = priv->location;
	ret = lr_handle_setopt (priv->repo_handle, error, LRO_URLS, urls);
	if (!ret)
		goto out;
	ret = lr_handle_setopt (priv->repo_handle, error, LRO_LOCAL, TRUE);
	if (!ret)
		goto out;
	ret = lr_handle_setopt (priv->repo_handle, error, LRO_CHECKSUM, TRUE);
	if (!ret)
		goto out;
	ret = lr_handle_setopt (priv->repo_handle, error, LRO_YUMDLIST, download_list);
	if (!ret)
		goto out;
	lr_result_clear (priv->repo_result);
	ret = lr_handle_perform (priv->repo_handle, priv->repo_result, &error_local);
	if (!ret) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "repodata %s was not complete: %s",
			     priv->id, error_local->message);
		g_error_free (error_local);
		goto out;
	}

	/* get the metadata file locations */
	ret = lr_result_getinfo (priv->repo_result, &error_local, LRR_YUM_REPO, &yum_repo);
	if (!ret) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "failed to get yum-repo: %s",
			     error_local->message);
		g_error_free (error_local);
		goto out;
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
		g_error_free (error_local);
		goto out;
	}

	/* check metadata age */
	if (permissible_cache_age != G_MAXUINT) {
		ret = hif_source_set_timestamp_modified (source, error);
		if (!ret)
			goto out;
		age_of_data = (g_get_real_time () - priv->timestamp_modified) / G_USEC_PER_SEC;
		if (age_of_data > permissible_cache_age) {
			ret = FALSE;
			g_set_error (error,
				     HIF_ERROR,
				     HIF_ERROR_INTERNAL_ERROR,
				     "cache too old: %"G_GINT64_FORMAT" > %i",
				     age_of_data, permissible_cache_age);
			goto out;
		}
	}

	/* create a HyRepo */
	priv->repo = hy_repo_create (priv->id);
	hy_repo_set_string (priv->repo, HY_REPO_MD_FN, yum_repo->repomd);
	tmp = lr_yum_repo_path (yum_repo, "primary");
	if (tmp != NULL)
		hy_repo_set_string (priv->repo, HY_REPO_PRIMARY_FN, tmp);
	tmp = lr_yum_repo_path (yum_repo, "filelists");
	if (tmp != NULL)
		hy_repo_set_string (priv->repo, HY_REPO_FILELISTS_FN, tmp);
	tmp = lr_yum_repo_path (yum_repo, "updateinfo");
	if (tmp != NULL)
		hy_repo_set_string (priv->repo, HY_REPO_UPDATEINFO_FN, tmp);
out:
	return ret;
}

/**
 * hif_source_remove_contents:
 *
 * Does not remove the directory itself, only the contents.
 **/
static gboolean
hif_source_remove_contents (const gchar *directory)
{
	gboolean ret = FALSE;
	GDir *dir;
	GError *error = NULL;
	const gchar *filename;
	gchar *src;
	gint retval;

	/* try to open */
	dir = g_dir_open (directory, 0, &error);
	if (dir == NULL) {
		g_warning ("cannot open directory: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* find each */
	while ((filename = g_dir_read_name (dir))) {
		src = g_build_filename (directory, filename, NULL);
		ret = g_file_test (src, G_FILE_TEST_IS_DIR);
		if (ret) {
			g_debug ("directory %s found in %s, deleting", filename, directory);
			/* recurse, but should be only 1 level deep */
			hif_source_remove_contents (src);
			retval = g_remove (src);
			if (retval != 0)
				g_warning ("failed to delete %s", src);
		} else {
			g_debug ("file found in %s, deleting", directory);
			retval = g_unlink (src);
			if (retval != 0)
				g_warning ("failed to delete %s", src);
		}
		g_free (src);
	}
	g_dir_close (dir);
	ret = TRUE;
out:
	return ret;
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
	gboolean ret;

	if (!g_file_test (priv->location, G_FILE_TEST_EXISTS))
		return TRUE;

	ret = hif_source_remove_contents (priv->location);
	if (!ret) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "Failed to remove %s",
			     priv->location);
	}
	return ret;
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
	gchar *pwd = NULL;
	gchar *str = NULL;
	gchar *usr = NULL;
	gchar **baseurls;
	gboolean ret;

	/* baseurl is optional */
	baseurls = g_key_file_get_string_list (priv->keyfile, priv->id, "baseurl", NULL, NULL);
	ret = lr_handle_setopt (priv->repo_handle, error, LRO_URLS, baseurls);
	if (!ret)
		goto out;
	g_strfreev (baseurls);

	/* mirrorlist is optional */
	str = g_key_file_get_string (priv->keyfile, priv->id, "mirrorlist", NULL);
	ret = lr_handle_setopt (priv->repo_handle, error, LRO_MIRRORLIST, str);
	if (!ret)
		goto out;
	g_free (str);

	/* metalink is optional */
	str = g_key_file_get_string (priv->keyfile, priv->id, "metalink", NULL);
	ret = lr_handle_setopt (priv->repo_handle, error, LRO_METALINKURL, str);
	if (!ret)
		goto out;
	g_free (str);

	/* gpgcheck is optional */
	// FIXME: https://github.com/Tojaj/librepo/issues/16
	//ret = lr_handle_setopt (priv->repo_handle, error, LRO_GPGCHECK, priv->gpgcheck == 1 ? 1 : 0);
	//if (!ret)
	//	goto out;

	/* proxy is optional */
	str = g_key_file_get_string (priv->keyfile, priv->id, "proxy", NULL);
	ret = lr_handle_setopt (priv->repo_handle, error, LRO_PROXY, str);
	if (!ret)
		goto out;
	g_free (str);

	/* both parts of the proxy auth are optional */
	usr = g_key_file_get_string (priv->keyfile, priv->id, "proxy_username", NULL);
	pwd = g_key_file_get_string (priv->keyfile, priv->id, "proxy_password", NULL);
	str = hif_source_get_username_password_string (usr, pwd);
	ret = lr_handle_setopt (priv->repo_handle, error, LRO_PROXYUSERPWD, str);
	if (!ret)
		goto out;
	g_free (usr);
	g_free (pwd);
	g_free (str);

	/* both parts of the HTTP auth are optional */
	usr = g_key_file_get_string (priv->keyfile, priv->id, "username", NULL);
	pwd = g_key_file_get_string (priv->keyfile, priv->id, "password", NULL);
	str = hif_source_get_username_password_string (usr, pwd);
	ret = lr_handle_setopt (priv->repo_handle, error, LRO_USERPWD, str);
	if (!ret)
		goto out;
out:
	g_free (usr);
	g_free (pwd);
	g_free (str);
	return ret;
//gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-fedora-$basearch
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
	GError *error_local = NULL;
	HifSourcePrivate *priv = GET_PRIVATE (source);
	HifState *state_local;
	gboolean ret;
	gint rc;
	gint64 timestamp_new = 0;

	/* take lock */
	ret = hif_state_take_lock (state,
				   HIF_LOCK_TYPE_METADATA,
				   HIF_LOCK_MODE_PROCESS,
				   error);
	if (!ret)
		goto out;

	/* cannot change DVD contents */
	if (priv->kind == HIF_SOURCE_KIND_MEDIA) {
		ret = FALSE;
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_SOURCE_NOT_AVAILABLE,
				     "Cannot update read-only source");
		goto out;
	}

	/* set state */
	ret = hif_state_set_steps (state, error,
				   50, /* download */
				   50, /* check */
				   -1);
	if (!ret)
		goto out;

	/* remove the temporary space if it already exists */
	if (g_file_test (priv->location_tmp, G_FILE_TEST_EXISTS)) {
		ret = hif_source_remove_contents (priv->location_tmp);
		if (!ret) {
			g_set_error (error,
				     HIF_ERROR,
				     HIF_ERROR_INTERNAL_ERROR,
				     "Failed to remove %s",
				     priv->location_tmp);
			goto out;
		}
	}

	/* ensure exists */
	if (!g_file_test (priv->location_tmp, G_FILE_TEST_EXISTS)) {
		rc = g_mkdir (priv->location_tmp, 0755);
		if (rc != 0) {
			ret = FALSE;
			g_set_error (error,
				     HIF_ERROR,
				     HIF_ERROR_INTERNAL_ERROR,
				     "Failed to create %s", priv->location_tmp);
			goto out;
		}
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
	ret = hif_source_set_keyfile_data (source, error);
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
				HIF_STATE_STATUS_DOWNLOAD_METADATA, NULL);
	ret = lr_handle_perform (priv->repo_handle,
				 priv->repo_result,
				 &error_local);
	if (!ret) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_CANNOT_FETCH_SOURCE,
			     "cannot update repo: %s",
			     error_local->message);
		g_error_free (error_local);
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
		g_error_free (error_local);
		goto out;
	}
	if ((flags & HIF_SOURCE_UPDATE_FLAG_FORCE) == 0 ||
	    timestamp_new < priv->timestamp_generated) {
		g_debug ("fresh metadata was older than what we have, ignoring");
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
				LRO_DESTDIR, priv->location_tmp);
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
 * Sets data on the source, which involves saving a new .repo file.
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
	gboolean ret;
	gchar *data = NULL;

	/* cannot change DVD contents */
	if (priv->kind == HIF_SOURCE_KIND_MEDIA) {
		ret = FALSE;
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_CANNOT_WRITE_SOURCE_CONFIG,
			     "Cannot set repo parameter %s=%s on read-only media",
			     parameter, value);
		goto out;
	}

	/* save change to keyfile and dump updated file to disk */
	g_key_file_set_string (priv->keyfile,
			       priv->id,
			       parameter,
			       value);
	data = g_key_file_to_data (priv->keyfile, NULL, error);
	if (data == NULL) {
		ret = FALSE;
		goto out;
	}
	ret = g_file_set_contents (priv->filename, data, -1, error);
	if (!ret)
		goto out;
out:
	g_free (data);
	return ret;
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
	GFile *file_dest;
	GFile *file_source;
	gboolean ret;
	gchar *basename = NULL;
	gchar *dest = NULL;

	/* copy the file with progress */
	file_source = g_file_new_for_path (hif_package_get_filename (pkg));
	basename = g_path_get_basename (hy_package_get_location (pkg));
	dest = g_build_filename (directory, basename, NULL);
	file_dest = g_file_new_for_path (dest);
	ret = g_file_copy (file_source, file_dest, G_FILE_COPY_NONE,
			   hif_state_get_cancellable (state),
			   hif_source_copy_progress_cb, state, error);
	if (!ret)
		goto out;
out:
	g_object_unref (file_source);
	g_object_unref (file_dest);
	g_free (basename);
	g_free (dest);
	return ret;
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
	GError *error_local = NULL;
	HifSourcePrivate *priv = GET_PRIVATE (source);
	char *checksum_str = NULL;
	const unsigned char *checksum;
	gboolean ret;
	gchar *basename = NULL;
	gchar *directory_slash;
	gchar *loc = NULL;
	gint rc;
	int checksum_type;

	/* if nothing specified then use cachedir */
	if (directory == NULL) {
		directory_slash = g_build_filename (priv->packages, "/", NULL);
		if (!g_file_test (directory_slash, G_FILE_TEST_EXISTS)) {
			rc = g_mkdir (directory_slash, 0755);
			if (rc != 0) {
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
	if (priv->keyfile == NULL) {
		hif_package_set_source (pkg, source);
		ret = hif_source_copy_package (pkg, directory, state, error);
		if (!ret)
			goto out;
		goto done;
	}

	/* setup the repo remote */
	ret = hif_source_set_keyfile_data (source, error);
	if (!ret)
		goto out;
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
				HIF_STATE_STATUS_DOWNLOAD_PACKAGES,
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
			g_error_free (error_local);
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
	g_free (basename);
	g_free (directory_slash);
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
