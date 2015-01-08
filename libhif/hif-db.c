/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008-2014 Richard Hughes <richard@hughsie.com>
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
 * SECTION:hif-db
 * @short_description: An extra 'database' to store details about packages
 * @include: libhif.h
 * @stability: Unstable
 *
 * #HifDb is a simple flat file 'database' for stroring details about
 * installed packages, such as the command line that installed them,
 * the uid of the user performing the action and the repository they
 * came from.
 *
 * A yumdb is not really a database at all, and is really slow to read
 * and especially slow to write data for packages. It is provided for
 * compatibility with existing users of yum, but long term this
 * functionality should either be folded into rpm itself, or just put
 * into an actual database format like sqlite.
 *
 * Using the filesystem as a database probably wasn't a great design
 * decision.
 */

#include "config.h"

#include "hif-cleanup.h"
#include "hif-db.h"
#include "hif-package.h"
#include "hif-utils.h"

typedef struct _HifDbPrivate	HifDbPrivate;
struct _HifDbPrivate
{
	HifContext		*context;	/* weak reference */
};

G_DEFINE_TYPE (HifDb, hif_db, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HIF_TYPE_DB, HifDbPrivate))

/**
 * hif_db_init:
 **/
static void
hif_db_init (HifDb *db)
{
}

/**
 * hif_db_class_init:
 **/
static void
hif_db_class_init (HifDbClass *klass)
{
	g_type_class_add_private (klass, sizeof (HifDbPrivate));
}


/**
 * hif_db_create_dir:
 **/
static gboolean
hif_db_create_dir (const gchar *dir, GError **error)
{
	_cleanup_object_unref_ GFile *file = NULL;

	/* already exists */
	if (g_file_test (dir, G_FILE_TEST_IS_DIR))
		return TRUE;

	/* need to create */
	g_debug ("creating %s", dir);
	file = g_file_new_for_path (dir);
	return g_file_make_directory_with_parents (file, NULL, error);
}

/**
 * hif_db_get_dir_for_package:
 **/
static gchar *
hif_db_get_dir_for_package (HifDb *db, HyPackage package)
{
	const gchar *pkgid;
	HifDbPrivate *priv = GET_PRIVATE (db);
	const gchar *instroot;

	pkgid = hif_package_get_pkgid (package);
	if (pkgid == NULL)
		return NULL;

	instroot = hif_context_get_install_root (priv->context);
	if (g_strcmp0 (instroot, "/") == 0)
		instroot = "";

	return g_strdup_printf ("%s/var/lib/yum/yumdb/%c/%s-%s-%s-%s-%s",
				instroot,
				hy_package_get_name (package)[0],
				pkgid,
				hy_package_get_name (package),
				hy_package_get_version (package),
				hy_package_get_release (package),
				hy_package_get_arch (package));
}

/**
 * hif_db_get_string:
 * @db: a #HifDb instance.
 * @package: A package to use as a reference
 * @key: A key name to retrieve, e.g. "releasever"
 * @error: A #GError, or %NULL
 *
 * Gets a string value from the yumdb 'database'.
 *
 * Returns: An allocated value, or %NULL
 *
 * Since: 0.1.0
 **/
gchar *
hif_db_get_string (HifDb *db, HyPackage package, const gchar *key, GError **error)
{
	gchar *value = NULL;
	_cleanup_free_ gchar *filename = NULL;
	_cleanup_free_ gchar *index_dir = NULL;

	g_return_val_if_fail (HIF_IS_DB (db), NULL);
	g_return_val_if_fail (package != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* get file contents */
	index_dir = hif_db_get_dir_for_package (db, package);
	if (index_dir == NULL) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_FAILED,
			     "cannot create index for %s",
			     hif_package_get_id (package));
		return NULL;
	}

	filename = g_build_filename (index_dir, key, NULL);

	/* check it exists */
	if (!g_file_test (filename, G_FILE_TEST_EXISTS)) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_FAILED,
			     "%s key not found",
			     filename);
		return NULL;
	}

	/* get value */
	if (!g_file_get_contents (filename, &value, NULL, error))
		return NULL;
	return value;
}

/**
 * hif_db_set_string:
 * @db: a #HifDb instance.
 * @package: A package to use as a reference
 * @key: Key name to save, e.g. "reason"
 * @value: Key data to save, e.g. "dep"
 * @error: A #GError, or %NULL
 *
 * Writes a data value to the yumdb 'database'.
 *
 * Returns: Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_db_set_string (HifDb *db,
		   HyPackage package,
		   const gchar *key,
		   const gchar *value,
		   GError **error)
{
	_cleanup_free_ gchar *index_dir = NULL;
	_cleanup_free_ gchar *index_file = NULL;

	g_return_val_if_fail (HIF_IS_DB (db), FALSE);
	g_return_val_if_fail (package != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (value != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	/* create the index directory */
	index_dir = hif_db_get_dir_for_package (db, package);
	if (index_dir == NULL) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_FAILED,
			     "cannot create index for %s",
			     hif_package_get_id (package));
		return FALSE;
	}
	if (!hif_db_create_dir (index_dir, error))
		return FALSE;

	/* write the value */
	index_file = g_build_filename (index_dir, key, NULL);
	g_debug ("writing %s to %s", value, index_file);
	return g_file_set_contents (index_file, value, -1, error);
}

/**
 * hif_db_remove:
 * @db: a #HifDb instance.
 * @package: A package to use as a reference
 * @key: Key name to delete, e.g. "reason"
 * @error: A #GError, or %NULL
 *
 * Removes a data value from the yumdb 'database' for a given package.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_db_remove (HifDb *db,
	       HyPackage package,
	       const gchar *key,
	       GError **error)
{
	_cleanup_free_ gchar *index_dir = NULL;
	_cleanup_free_ gchar *index_file = NULL;
	_cleanup_object_unref_ GFile *file = NULL;

	g_return_val_if_fail (HIF_IS_DB (db), FALSE);
	g_return_val_if_fail (package != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	/* create the index directory */
	index_dir = hif_db_get_dir_for_package (db, package);
	if (index_dir == NULL) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_FAILED,
			     "cannot create index for %s",
			     hif_package_get_id (package));
		return FALSE;
	}

	/* delete the value */
	g_debug ("deleting %s from %s", key, index_dir);
	index_file = g_build_filename (index_dir, key, NULL);
	file = g_file_new_for_path (index_file);
	return g_file_delete (file, NULL, error);
}

/**
 * hif_db_remove_all:
 * @db: a #HifDb instance.
 * @package: A package to use as a reference
 * @error: A #GError, or %NULL
 *
 * Removes a all data value from the yumdb 'database' for a given package.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_db_remove_all (HifDb *db, HyPackage package, GError **error)
{
	const gchar *filename;
	_cleanup_dir_close_ GDir *dir = NULL;
	_cleanup_free_ gchar *index_dir = NULL;
	_cleanup_object_unref_ GFile *file_directory = NULL;

	g_return_val_if_fail (HIF_IS_DB (db), FALSE);
	g_return_val_if_fail (package != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	/* get the folder */
	index_dir = hif_db_get_dir_for_package (db, package);
	if (index_dir == NULL) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_FAILED,
			     "cannot create index for %s",
			     hif_package_get_id (package));
		return FALSE;
	}
	if (!g_file_test (index_dir, G_FILE_TEST_IS_DIR)) {
		g_debug ("Nothing to delete in %s", index_dir);
		return TRUE;
	}

	/* open */
	dir = g_dir_open (index_dir, 0, error);
	if (dir == NULL)
		return FALSE;

	/* delete each one */
	filename = g_dir_read_name (dir);
	while (filename != NULL) {
		_cleanup_free_ gchar *index_file;
		_cleanup_object_unref_ GFile *file_tmp;

		index_file = g_build_filename (index_dir, filename, NULL);
		file_tmp = g_file_new_for_path (index_file);

		/* delete, ignoring error */
		g_debug ("deleting %s from %s", filename, index_dir);
		if (!g_file_delete (file_tmp, NULL, NULL))
			g_debug ("failed to delete %s", filename);
		filename = g_dir_read_name (dir);
	}

	/* now delete the directory */
	file_directory = g_file_new_for_path (index_dir);
	return g_file_delete (file_directory, NULL, error);
}

/**
 * hif_db_ensure_origin_pkg:
 * @db: a #HifDb instance.
 * @pkg: A package to set
 *
 * Sets the repo origin on a package if not already set.
 *
 * Since: 0.1.0
 */
void
hif_db_ensure_origin_pkg (HifDb *db, HyPackage pkg)
{
	_cleanup_error_free_ GError *error = NULL;
	_cleanup_free_ gchar *tmp = NULL;

	/* already set */
	if (hif_package_get_origin (pkg) != NULL)
		return;
	if (!hy_package_installed (pkg))
		return;

	/* set from the database if available */
	tmp = hif_db_get_string (db, pkg, "from_repo", &error);
	if (tmp == NULL) {
		g_debug ("no origin for %s: %s",
			 hif_package_get_id (pkg),
			 error->message);
	} else {
		hif_package_set_origin (pkg, tmp);
	}
}

/**
 * hif_db_ensure_origin_pkglist:
 * @db: a #HifDb instance.
 * @pkglist: A package list to set
 *
 * Sets the repo origin on several package if not already set.
 *
 * Since: 0.1.0
 */
void
hif_db_ensure_origin_pkglist (HifDb *db, HyPackageList pkglist)
{
	HyPackage pkg;
	gint i;

	FOR_PACKAGELIST(pkg, pkglist, i)
		hif_db_ensure_origin_pkg (db, pkg);
}

/**
 * hif_db_new:
 * @context: a #HifContext instance.
 *
 * Creates a new #HifDb.
 *
 * Returns: (transfer full): a #HifDb
 *
 * Since: 0.1.0
 **/
HifDb *
hif_db_new (HifContext *context)
{
	HifDb *db;
	HifDbPrivate *priv;
	db = g_object_new (HIF_TYPE_DB, NULL);
	priv = GET_PRIVATE (db);
	priv->context = context;
	g_object_add_weak_pointer (G_OBJECT (priv->context), (void **) &priv->context);
	return HIF_DB (db);
}
