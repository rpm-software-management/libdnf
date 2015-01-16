/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013-2014 Richard Hughes <richard@hughsie.com>
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

/**
 * SECTION:hif-repos
 * @short_description: An object for loading several #HifSource objects.
 * @include: libhif-private.h
 * @stability: Stable
 *
 * This object can create #HifSource objects from either DVD media or from .repo
 * files in the repodir.
 *
 * See also: #HifSource
 */

#include <strings.h>
#include "config.h"

#include <gio/gunixmounts.h>
#include <librepo/util.h>

#include "hif-cleanup.h"
#include "hif-package.h"
#include "hif-repos.h"
#include "hif-utils.h"

typedef struct _HifReposPrivate	HifReposPrivate;
struct _HifReposPrivate
{
	GFileMonitor		*monitor_repos;
	HifContext		*context;	/* weak reference */
	GPtrArray		*sources;
	GVolumeMonitor		*volume_monitor;
	gboolean		 loaded;
};

enum {
	SIGNAL_CHANGED,
	SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

G_DEFINE_TYPE (HifRepos, hif_repos, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HIF_TYPE_REPOS, HifReposPrivate))

/**
 * hif_repos_finalize:
 **/
static void
hif_repos_finalize (GObject *object)
{
	HifRepos *repos = HIF_REPOS (object);
	HifReposPrivate *priv = GET_PRIVATE (repos);

	if (priv->monitor_repos != NULL)
		g_object_unref (priv->monitor_repos);
	g_object_unref (priv->volume_monitor);
	g_ptr_array_unref (priv->sources);

	G_OBJECT_CLASS (hif_repos_parent_class)->finalize (object);
}

/**
 * hif_repos_invalidate:
 */
static void
hif_repos_invalidate (HifRepos *repos)
{
	HifReposPrivate *priv = GET_PRIVATE (repos);
	priv->loaded = FALSE;
}

/**
 * hif_repos_mount_changed_cb:
 */
static void
hif_repos_mount_changed_cb (GVolumeMonitor *vm, GMount *mount, HifRepos *repos)
{
	/* invalidate all repos if a CD is inserted / removed */
	g_debug ("emit changed (mounts changed)");
	g_signal_emit (repos, signals[SIGNAL_CHANGED], 0);
	hif_repos_invalidate (repos);
}

/**
 * hif_repos_init:
 **/
static void
hif_repos_init (HifRepos *repos)
{
	HifReposPrivate *priv = GET_PRIVATE (repos);
	priv->sources = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
	priv->volume_monitor = g_volume_monitor_get ();
	g_signal_connect (priv->volume_monitor, "mount-added",
			  G_CALLBACK (hif_repos_mount_changed_cb), repos);
	g_signal_connect (priv->volume_monitor, "mount-removed",
			  G_CALLBACK (hif_repos_mount_changed_cb), repos);
}

/**
 * hif_repos_class_init:
 **/
static void
hif_repos_class_init (HifReposClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	/**
	 * HifRepos::changed:
	 **/
	signals[SIGNAL_CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HifReposClass, changed),
			      NULL, NULL, g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	object_class->finalize = hif_repos_finalize;
	g_type_class_add_private (klass, sizeof (HifReposPrivate));
}

/**
 * hif_repos_add_media:
 **/
static gboolean
hif_repos_add_media (HifRepos *repos,
		     const gchar *mount_point,
		     guint idx,
		     GError **error)
{
	HifReposPrivate *priv = GET_PRIVATE (repos);
	HifSource *source;
	_cleanup_free_ gchar *packages = NULL;
	_cleanup_free_ gchar *treeinfo_fn;
	_cleanup_keyfile_unref_ GKeyFile *treeinfo;

	/* get common things */
	treeinfo_fn = g_build_filename (mount_point, ".treeinfo", NULL);
	treeinfo = g_key_file_new ();
	if (!g_key_file_load_from_file (treeinfo, treeinfo_fn, 0, error))
		return FALSE;

	/* create read-only location */
	source = hif_source_new (priv->context);
	hif_source_set_enabled (source, HIF_SOURCE_ENABLED_PACKAGES);
	hif_source_set_gpgcheck (source, TRUE);
	hif_source_set_kind (source, HIF_SOURCE_KIND_MEDIA);
	hif_source_set_cost (source, 100);
	hif_source_set_keyfile (source, treeinfo);
	if (idx == 0) {
		hif_source_set_id (source, "media");
	} else {
		_cleanup_free_ gchar *tmp;
		tmp = g_strdup_printf ("media-%i", idx);
		hif_source_set_id (source, tmp);
	}
	hif_source_set_location (source, mount_point);
	if (!hif_source_setup (source, error))
		return FALSE;

	g_debug ("added source %s", hif_source_get_id (source));
	g_ptr_array_add (priv->sources, source);
	return TRUE;
}

/**
 * hif_repos_add_sack_from_mount_point:
 */
static gboolean
hif_repos_add_sack_from_mount_point (HifRepos *repos,
				     const gchar *root,
				     guint *idx,
				     GError **error)
{
	const gchar *id = ".treeinfo";
	gboolean exists;
	_cleanup_free_ gchar *treeinfo_fn;

	/* check if any installed media is an install disk */
	treeinfo_fn = g_build_filename (root, id, NULL);
	exists = g_file_test (treeinfo_fn, G_FILE_TEST_EXISTS);
	g_debug ("checking %s for %s: %s", root, id, exists ? "yes" : "no");
	if (!exists)
		return TRUE;

	/* add the repodata/repomd.xml as a source */
	if (!hif_repos_add_media (repos, root, *idx, error))
		return FALSE;
	(*idx)++;
	return TRUE;
}

/**
 * hif_repos_get_sources_removable:
 */
static gboolean
hif_repos_get_sources_removable (HifRepos *repos, GError **error)
{
	GList *mounts;
	GList *l;
	gboolean ret = TRUE;
	guint idx = 0;

	/* coldplug the mounts */
	mounts = g_unix_mounts_get (NULL);
	for (l = mounts; l != NULL; l = l->next) {
		GUnixMountEntry *e = (GUnixMountEntry *) l->data;
		if (!g_unix_mount_is_readonly (e))
			continue;
		if (g_strcmp0 (g_unix_mount_get_fs_type (e), "iso9660") != 0)
			continue;
		ret = hif_repos_add_sack_from_mount_point (repos,
							   g_unix_mount_get_mount_path (e),
							   &idx,
							   error);
		if (!ret)
			goto out;
	}
out:
	g_list_foreach (mounts, (GFunc) g_unix_mount_free, NULL);
	g_list_free (mounts);
	return ret;
}

/**
 * hi_repos_source_cost_fn:
 */
static gint
hi_repos_source_cost_fn (gconstpointer a, gconstpointer b)
{
	HifSource *src_a = *((HifSource **) a);
	HifSource *src_b = *((HifSource **) b);
	if (hif_source_get_cost (src_a) < hif_source_get_cost (src_b))
		return -1;
	if (hif_source_get_cost (src_a) > hif_source_get_cost (src_b))
		return 1;
	return 0;
}

/**
 * hif_repos_load_multiline_key_file:
 **/
static GKeyFile *
hif_repos_load_multiline_key_file (const gchar *filename, GError **error)
{
	GKeyFile *file = NULL;
	gboolean ret;
	gsize len;
	guint i;
	_cleanup_free_ gchar *data = NULL;
	_cleanup_string_free_ GString *string = NULL;
	_cleanup_strv_free_ gchar **lines = NULL;

	/* load file */
	if (!g_file_get_contents (filename, &data, &len, error))
		return NULL;

	/* split into lines */
	string = g_string_new ("");
	lines = g_strsplit (data, "\n", -1);
	for (i = 0; lines[i] != NULL; i++) {

		/* convert tabs to spaces */
		g_strdelimit (lines[i], "\t", ' ');

		/* if a line starts with whitespace, then append it on
		 * the previous line */
		if (lines[i][0] == ' ' && string->len > 0) {

			/* remove old newline from previous line */
			g_string_set_size (string, string->len - 1);

			/* whitespace strip this new line */
			g_strchug (lines[i]);

			/* only add a ';' if we have anything after the '=' */
			if (string->str[string->len - 1] == '=') {
				g_string_append_printf (string, "%s\n", lines[i]);
			} else {
				g_string_append_printf (string, ";%s\n", lines[i]);
			}
		} else {
			g_string_append_printf (string, "%s\n", lines[i]);
		}
	}

	/* remove final newline */
	if (string->len > 0)
		g_string_set_size (string, string->len - 1);

	/* load modified lines */
	file = g_key_file_new ();
	ret = g_key_file_load_from_data (file,
					 string->str,
					 -1,
					 G_KEY_FILE_KEEP_COMMENTS,
					 error);
	if (!ret) {
		g_key_file_free (file);
		return NULL;
	}
	return file;
}

/**
 * hif_repos_source_parse_id:
 **/
static gboolean
hif_repos_source_parse_id (HifRepos *repos,
			   const gchar *id,
			   const gchar *filename,
			   GKeyFile *keyfile,
			   GError **error)
{
	HifReposPrivate *priv = GET_PRIVATE (repos);
	HifSourceEnabled enabled = 0;
	_cleanup_object_unref_ HifSource *source;

	/* enabled isn't a required key */
	if (g_key_file_has_key (keyfile, id, "enabled", NULL)) {
		if (g_key_file_get_boolean (keyfile, id, "enabled", NULL))
			enabled |= HIF_SOURCE_ENABLED_PACKAGES;
	} else {
		enabled |= HIF_SOURCE_ENABLED_PACKAGES;
	}

	/* enabled_metadata isn't a required key */
	if (g_key_file_has_key (keyfile, id, "enabled_metadata", NULL)) {
		if (g_key_file_get_boolean (keyfile, id, "enabled_metadata", NULL))
			enabled |= HIF_SOURCE_ENABLED_METADATA;
	} else {
		_cleanup_free_ gchar *basename = NULL;
		basename = g_path_get_basename (filename);
		if (g_strcmp0 (basename, "redhat.repo") == 0)
			enabled |= HIF_SOURCE_ENABLED_METADATA;
	}

	source = hif_source_new (priv->context);
	hif_source_set_enabled (source, enabled);
	hif_source_set_kind (source, HIF_SOURCE_KIND_REMOTE);
	hif_source_set_keyfile (source, keyfile);
	hif_source_set_filename (source, filename);
	hif_source_set_id (source, id);

	/* set up the repo ready for use */
	if (!hif_source_setup (source, error))
		return FALSE;

	g_debug ("added source %s\t%s", filename, id);
	g_ptr_array_add (priv->sources, g_object_ref (source));
	return TRUE;
}

/**
 * hif_repos_source_parse:
 **/
static gboolean
hif_repos_source_parse (HifRepos *repos,
			const gchar *filename,
			GError **error)
{
	gboolean ret = TRUE;
	guint i;
	_cleanup_strv_free_ gchar **groups = NULL;
	_cleanup_keyfile_unref_ GKeyFile *keyfile;

	/* load non-standard keyfile */
	keyfile = hif_repos_load_multiline_key_file (filename, error);
	if (keyfile == NULL)
		return FALSE;

	/* save all the repos listed in the file */
	groups = g_key_file_get_groups (keyfile, NULL);
	for (i = 0; groups[i] != NULL; i++) {
		ret = hif_repos_source_parse_id (repos,
						 groups[i],
						 filename,
						 keyfile,
						 error);
		if (!ret)
			return FALSE;
	}
	return TRUE;
}

/**
 * hif_repos_refresh:
 */
static gboolean
hif_repos_refresh (HifRepos *repos, GError **error)
{
	HifReposPrivate *priv = GET_PRIVATE (repos);
	const gchar *file;
	const gchar *repo_path;
	_cleanup_dir_close_ GDir *dir = NULL;

	/* no longer loaded */
	hif_repos_invalidate (repos);
	g_ptr_array_set_size (priv->sources, 0);

	/* open dir */
	repo_path = hif_context_get_repo_dir (priv->context);
	dir = g_dir_open (repo_path, 0, error);
	if (dir == NULL)
		return FALSE;

	/* find all the .repo files */
	while ((file = g_dir_read_name (dir)) != NULL) {
		_cleanup_free_ gchar *path_tmp = NULL;
		if (!g_str_has_suffix (file, ".repo"))
			continue;
		path_tmp = g_build_filename (repo_path, file, NULL);
		if (!hif_repos_source_parse (repos, path_tmp, error))
			return FALSE;
	}

	/* add any DVD sources */
	if (!hif_repos_get_sources_removable (repos, error))
		return FALSE;

	/* all okay */
	priv->loaded = TRUE;

	/* sort these in order of cost */
	g_ptr_array_sort (priv->sources, hi_repos_source_cost_fn);
	return TRUE;
}

/**
 * hif_repos_has_removable:
 */
gboolean
hif_repos_has_removable (HifRepos *repos)
{
	HifReposPrivate *priv = GET_PRIVATE (repos);
	HifSource *src;
	guint i;

	g_return_val_if_fail (HIF_IS_REPOS (repos), FALSE);

	/* are there any media repos */
	for (i = 0; i < priv->sources->len; i++) {
		src = g_ptr_array_index (priv->sources, i);
		if (hif_source_get_kind (src) == HIF_SOURCE_KIND_MEDIA)
			return TRUE;
	}
	return FALSE;
}

/**
 * hif_repos_get_sources:
 *
 * Returns: (transfer container) (element-type HifSource): Array of sources
 */
GPtrArray *
hif_repos_get_sources (HifRepos *repos, GError **error)
{
	HifReposPrivate *priv = GET_PRIVATE (repos);

	g_return_val_if_fail (HIF_IS_REPOS (repos), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* nothing set yet */
	if (!priv->loaded) {
		if (!hif_repos_refresh (repos, error))
			return NULL;
	}

	/* all okay */
	return g_ptr_array_ref (priv->sources);
}

/**
 * hif_repos_get_source_by_id:
 */
HifSource *
hif_repos_get_source_by_id (HifRepos *repos, const gchar *id, GError **error)
{
	HifReposPrivate *priv = GET_PRIVATE (repos);
	HifSource *tmp;
	guint i;

	g_return_val_if_fail (HIF_IS_REPOS (repos), NULL);
	g_return_val_if_fail (id != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* nothing set yet */
	if (!priv->loaded) {
		if (!hif_repos_refresh (repos, error))
			return NULL;
	}

	for (i = 0; i < priv->sources->len; i++) {
		tmp = g_ptr_array_index (priv->sources, i);
		if (g_strcmp0 (hif_source_get_id (tmp), id) == 0)
			return tmp;
	}

	/* we didn't find anything */
	g_set_error (error,
		     HIF_ERROR,
		     HIF_ERROR_SOURCE_NOT_FOUND,
		     "failed to find %s", id);
	return NULL;
}

/**
 * hif_repos_directory_changed_cb:
 **/
static void
hif_repos_directory_changed_cb (GFileMonitor *monitor_,
				GFile *file, GFile *other_file,
				GFileMonitorEvent event_type,
				HifRepos *repos)
{
	g_debug ("emit changed (ReposDir changed)");
	g_signal_emit (repos, signals[SIGNAL_CHANGED], 0);
	hif_repos_invalidate (repos);
}

/**
 * hif_repos_setup_watch:
 */
static void
hif_repos_setup_watch (HifRepos *repos)
{
	HifReposPrivate *priv = GET_PRIVATE (repos);
	const gchar *repo_dir;
	_cleanup_error_free_ GError *error = NULL;
	_cleanup_object_unref_ GFile *file_repos = NULL;

	/* setup a file monitor on the repos directory */
	repo_dir = hif_context_get_repo_dir (priv->context);
	if (repo_dir == NULL) {
		g_warning ("no repodir set");
		return;
	}
	file_repos = g_file_new_for_path (repo_dir);
	priv->monitor_repos = g_file_monitor_directory (file_repos,
							G_FILE_MONITOR_NONE,
							NULL,
							&error);
	if (priv->monitor_repos != NULL) {
		g_signal_connect (priv->monitor_repos, "changed",
				  G_CALLBACK (hif_repos_directory_changed_cb), repos);
	} else {
		g_warning ("failed to setup monitor: %s",
			   error->message);
	}
}

/**
 * hif_repos_new:
 * @context: A #HifContext instance
 *
 * Creates a new #HifRepos.
 *
 * Returns: (transfer full): a #HifRepos
 *
 * Since: 0.1.0
 **/
HifRepos *
hif_repos_new (HifContext *context)
{
	HifReposPrivate *priv;
	HifRepos *repos;
	repos = g_object_new (HIF_TYPE_REPOS, NULL);
	priv = GET_PRIVATE (repos);
	priv->context = context;
	g_object_add_weak_pointer (G_OBJECT (priv->context), (void **) &priv->context);
	hif_repos_setup_watch (repos);
	return HIF_REPOS (repos);
}
