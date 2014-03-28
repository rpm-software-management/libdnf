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

#include "config.h"

#include <gio/gunixmounts.h>

#include "hif-package.h"
#include "hif-repos.h"
#include "hif-utils.h"

typedef struct _HifReposPrivate	HifReposPrivate;
struct _HifReposPrivate
{
	GFileMonitor		*monitor_repos;
	HifContext		*context;
	GPtrArray		*sources;
	GVolumeMonitor		*volume_monitor;
	gboolean		 loaded;
};

enum {
	SIGNAL_CHANGED,
	SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (HifRepos, hif_repos, G_TYPE_OBJECT)

#define GET_PRIVATE(o) (hif_repos_get_instance_private (o))

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
	g_object_unref (priv->context);
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
	HifReposPrivate *priv = hif_repos_get_instance_private (repos);
	priv->loaded = FALSE;
	g_ptr_array_set_size (priv->sources, 0);
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
}

/**
 * hif_repos_add_sack_from_mount_point:
 */
static gboolean
hif_repos_add_sack_from_mount_point (GPtrArray *sources,
				     const gchar *root,
				     guint *idx,
				     GError **error)
{
	const gchar *id = ".treeinfo";
	gboolean exists;
	gboolean ret = TRUE;
	gchar *treeinfo_fn;

	/* check if any installed media is an install disk */
	treeinfo_fn = g_build_filename (root, id, NULL);
	exists = g_file_test (treeinfo_fn, G_FILE_TEST_EXISTS);
	g_debug ("checking %s for %s: %s", root, id, exists ? "yes" : "no");
	if (!exists)
		goto out;

	/* add the repodata/repomd.xml as a source */
	ret = hif_source_add_media (sources, root, *idx, error);
	if (!ret)
		goto out;
	(*idx)++;
out:
	g_free (treeinfo_fn);
	return ret;
}

/**
 * hif_repos_get_sources_removable:
 */
static gboolean
hif_repos_get_sources_removable (GPtrArray *sources, GError **error)
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
		ret = hif_repos_add_sack_from_mount_point (sources,
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
 * hif_repos_refresh:
 */
static gboolean
hif_repos_refresh (HifRepos *repos, GError **error)
{
	GDir *dir = NULL;
	HifReposPrivate *priv = hif_repos_get_instance_private (repos);
	const gchar *file;
	const gchar *repo_path;
	gboolean ret = TRUE;
	gchar *path_tmp;

	/* no longer loaded */
	hif_repos_invalidate (repos);

	/* open dir */
	repo_path = hif_context_get_repo_dir (priv->context);
	dir = g_dir_open (repo_path, 0, error);
	if (dir == NULL) {
		ret = FALSE;
		goto out;
	}

	/* find all the .repo files */
	while ((file = g_dir_read_name (dir)) != NULL) {
		if (!g_str_has_suffix (file, ".repo"))
			continue;
		path_tmp = g_build_filename (repo_path, file, NULL);
		ret = hif_source_parse (priv->context, priv->sources, path_tmp, error);
		g_free (path_tmp);
		if (!ret)
			goto out;
	}

	/* add any DVD sources */
	ret = hif_repos_get_sources_removable (priv->sources, error);
	if (!ret)
		goto out;

	/* all okay */
	priv->loaded = TRUE;

	/* sort these in order of cost */
	g_ptr_array_sort (priv->sources, hi_repos_source_cost_fn);
out:
	if (dir != NULL)
		g_dir_close (dir);
	return ret;
}

/**
 * hif_repos_has_removable:
 */
gboolean
hif_repos_has_removable (HifRepos *repos)
{
	HifReposPrivate *priv = hif_repos_get_instance_private (repos);
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
 */
GPtrArray *
hif_repos_get_sources (HifRepos *repos, GError **error)
{
	GPtrArray *sources = NULL;
	HifReposPrivate *priv = hif_repos_get_instance_private (repos);
	gboolean ret;

	g_return_val_if_fail (HIF_IS_REPOS (repos), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* nothing set yet */
	if (!priv->loaded) {
		ret = hif_repos_refresh (repos, error);
		if (!ret)
			goto out;
	}

	/* all okay */
	sources = g_ptr_array_ref (priv->sources);
out:
	return sources;
}

/**
 * hif_repos_get_source_by_id:
 */
HifSource *
hif_repos_get_source_by_id (HifRepos *repos, const gchar *id, GError **error)
{
	HifReposPrivate *priv = hif_repos_get_instance_private (repos);
	HifSource *src = NULL;
	HifSource *tmp;
	gboolean ret;
	guint i;

	g_return_val_if_fail (HIF_IS_REPOS (repos), NULL);
	g_return_val_if_fail (id != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* nothing set yet */
	if (!priv->loaded) {
		ret = hif_repos_refresh (repos, error);
		if (!ret)
			goto out;
	}

	for (i = 0; i < priv->sources->len; i++) {
		tmp = g_ptr_array_index (priv->sources, i);
		if (g_strcmp0 (hif_source_get_id (tmp), id) == 0) {
			src = tmp;
			goto out;
		}
	}

	/* we didn't find anything */
	g_set_error (error,
		     HIF_ERROR,
		     HIF_ERROR_SOURCE_NOT_FOUND,
		     "failed to find %s", id);
out:
	return src;
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
	GError *error = NULL;
	GFile *file_repos = NULL;
	HifReposPrivate *priv = hif_repos_get_instance_private (repos);

	/* setup a file monitor on the repos directory */
	file_repos = g_file_new_for_path (hif_context_get_repo_dir (priv->context));
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
		g_error_free (error);
	}
	g_object_unref (file_repos);
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
	priv = hif_repos_get_instance_private (repos);
	priv->context = g_object_ref (context);
	hif_repos_setup_watch (repos);
	return HIF_REPOS (repos);
}
