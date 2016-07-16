/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2008-2015 Richard Hughes <richard@hughsie.com>
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
 * SECTION:dnf-db
 * @short_description: An extra 'database' to store details about packages
 * @include: libdnf.h
 * @stability: Unstable
 *
 * #DnfDb is a simple flat file 'database' for stroring details about
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


#include "dnf-db.h"
#include "dnf-package.h"
#include "dnf-utils.h"

typedef struct
{
    DnfContext      *context;    /* weak reference */
    gboolean         enabled;
} DnfDbPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(DnfDb, dnf_db, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (dnf_db_get_instance_private (o))

/**
 * dnf_db_finalize:
 **/
static void
dnf_db_finalize(GObject *object)
{
    DnfDb *db = DNF_DB(object);
    DnfDbPrivate *priv = GET_PRIVATE(db);

    if (priv->context != NULL)
        g_object_remove_weak_pointer(G_OBJECT(priv->context),
                                     (void **) &priv->context);

    G_OBJECT_CLASS(dnf_db_parent_class)->finalize(object);
}

/**
 * dnf_db_init:
 **/
static void
dnf_db_init(DnfDb *db)
{
}

/**
 * dnf_db_class_init:
 **/
static void
dnf_db_class_init(DnfDbClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_db_finalize;
}


/**
 * dnf_db_create_dir:
 **/
static gboolean
dnf_db_create_dir(const gchar *dir, GError **error)
{
    g_autoptr(GFile) file = NULL;

    /* already exists */
    if (g_file_test(dir, G_FILE_TEST_IS_DIR))
        return TRUE;

    /* need to create */
    g_debug("creating %s", dir);
    file = g_file_new_for_path(dir);
    return g_file_make_directory_with_parents(file, NULL, error);
}

/**
 * dnf_db_get_dir_for_package:
 **/
static gchar *
dnf_db_get_dir_for_package(DnfDb *db, DnfPackage *package)
{
    const gchar *pkgid;
    DnfDbPrivate *priv = GET_PRIVATE(db);
    const gchar *instroot;
#ifdef BUILDOPT_USE_DNF_YUMDB
    static const gchar *yumdb_dir = "/var/lib/dnf/yumdb";
#else
    static const gchar *yumdb_dir = "/var/lib/yum/yumdb";
#endif

    pkgid = dnf_package_get_pkgid(package);
    if (pkgid == NULL)
        return NULL;

    instroot = dnf_context_get_install_root(priv->context);
    if (g_strcmp0(instroot, "/") == 0)
        instroot = "";

    return g_strdup_printf("%s%s/%c/%s-%s-%s-%s-%s",
                          instroot,
                          yumdb_dir,
                          dnf_package_get_name(package)[0],
                          pkgid,
                          dnf_package_get_name(package),
                          dnf_package_get_version(package),
                          dnf_package_get_release(package),
                          dnf_package_get_arch(package));
}

/**
 * dnf_db_get_string:
 * @db: a #DnfDb instance.
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
dnf_db_get_string(DnfDb *db, DnfPackage *package, const gchar *key, GError **error)
{
    gchar *value = NULL;
    g_autofree gchar *filename = NULL;
    g_autofree gchar *index_dir = NULL;

    g_return_val_if_fail(DNF_IS_DB(db), NULL);
    g_return_val_if_fail(package != NULL, NULL);
    g_return_val_if_fail(key != NULL, NULL);
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    /* get file contents */
    index_dir = dnf_db_get_dir_for_package(db, package);
    if (index_dir == NULL) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_FAILED,
                    "cannot read index for %s",
                    dnf_package_get_package_id(package));
        return NULL;
    }

    filename = g_build_filename(index_dir, key, NULL);

    /* check it exists */
    if (!g_file_test(filename, G_FILE_TEST_EXISTS)) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_FAILED,
                    "%s key not found",
                    filename);
        return NULL;
    }

    /* get value */
    if (!g_file_get_contents(filename, &value, NULL, error))
        return NULL;
    return value;
}

/**
 * dnf_db_set_string:
 * @db: a #DnfDb instance.
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
dnf_db_set_string(DnfDb *db,
           DnfPackage *package,
           const gchar *key,
           const gchar *value,
           GError **error)
{
    DnfDbPrivate *priv = GET_PRIVATE(db);
    g_autofree gchar *index_dir = NULL;
    g_autofree gchar *index_file = NULL;

    g_return_val_if_fail(DNF_IS_DB(db), FALSE);
    g_return_val_if_fail(package != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(value != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (!priv->enabled)
        return TRUE;

    /* create the index directory */
    index_dir = dnf_db_get_dir_for_package(db, package);
    if (index_dir == NULL) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_FAILED,
                    "cannot create index for %s",
                    dnf_package_get_package_id(package));
        return FALSE;
    }
    if (!dnf_db_create_dir(index_dir, error))
        return FALSE;

    /* write the value */
    index_file = g_build_filename(index_dir, key, NULL);
    g_debug("writing %s to %s", value, index_file);
    return g_file_set_contents(index_file, value, -1, error);
}

/**
 * dnf_db_remove:
 * @db: a #DnfDb instance.
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
dnf_db_remove(DnfDb *db,
           DnfPackage *package,
           const gchar *key,
           GError **error)
{
    DnfDbPrivate *priv = GET_PRIVATE(db);
    g_autofree gchar *index_dir = NULL;
    g_autofree gchar *index_file = NULL;
    g_autoptr(GFile) file = NULL;

    g_return_val_if_fail(DNF_IS_DB(db), FALSE);
    g_return_val_if_fail(package != NULL, FALSE);
    g_return_val_if_fail(key != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (!priv->enabled)
        return TRUE;

    /* create the index directory */
    index_dir = dnf_db_get_dir_for_package(db, package);
    if (index_dir == NULL) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_FAILED,
                    "cannot create index for %s",
                    dnf_package_get_package_id(package));
        return FALSE;
    }

    /* delete the value */
    g_debug("deleting %s from %s", key, index_dir);
    index_file = g_build_filename(index_dir, key, NULL);
    file = g_file_new_for_path(index_file);
    return g_file_delete(file, NULL, error);
}

/**
 * dnf_db_remove_all:
 * @db: a #DnfDb instance.
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
dnf_db_remove_all(DnfDb *db, DnfPackage *package, GError **error)
{
    DnfDbPrivate *priv = GET_PRIVATE(db);
    const gchar *filename;
    g_autoptr(GDir) dir = NULL;
    g_autofree gchar *index_dir = NULL;
    g_autoptr(GFile) file_directory = NULL;

    g_return_val_if_fail(DNF_IS_DB(db), FALSE);
    g_return_val_if_fail(package != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    if (!priv->enabled)
        return TRUE;

    /* get the folder */
    index_dir = dnf_db_get_dir_for_package(db, package);
    if (index_dir == NULL) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_FAILED,
                    "cannot create index for %s",
                    dnf_package_get_package_id(package));
        return FALSE;
    }
    if (!g_file_test(index_dir, G_FILE_TEST_IS_DIR)) {
        g_debug("Nothing to delete in %s", index_dir);
        return TRUE;
    }

    /* open */
    dir = g_dir_open(index_dir, 0, error);
    if (dir == NULL)
        return FALSE;

    /* delete each one */
    filename = g_dir_read_name(dir);
    while (filename != NULL) {
        g_autofree gchar *index_file = NULL;
        g_autoptr(GFile) file_tmp = NULL;

        index_file = g_build_filename(index_dir, filename, NULL);
        file_tmp = g_file_new_for_path(index_file);

        /* delete, ignoring error */
        g_debug("deleting %s from %s", filename, index_dir);
        if (!g_file_delete(file_tmp, NULL, NULL))
            g_debug("failed to delete %s", filename);
        filename = g_dir_read_name(dir);
    }

    /* now delete the directory */
    file_directory = g_file_new_for_path(index_dir);
    return g_file_delete(file_directory, NULL, error);
}

/**
 * dnf_db_ensure_origin_pkg:
 * @db: a #DnfDb instance.
 * @pkg: A package to set
 *
 * Sets the repo origin on a package if not already set.
 *
 * Since: 0.1.0
 */
void
dnf_db_ensure_origin_pkg(DnfDb *db, DnfPackage *pkg)
{
    g_autoptr(GError) error = NULL;
    g_autofree gchar *tmp = NULL;

    /* already set */
    if (dnf_package_get_origin(pkg) != NULL)
        return;
    if (!dnf_package_installed(pkg))
        return;

    /* set from the database if available */
    tmp = dnf_db_get_string(db, pkg, "from_repo", &error);
    if (tmp == NULL) {
        g_debug("no origin for %s: %s",
             dnf_package_get_package_id(pkg),
             error->message);
    } else {
        dnf_package_set_origin(pkg, tmp);
    }
}

/**
 * dnf_db_ensure_origin_pkglist:
 * @db: a #DnfDb instance.
 * @pkglist: A package list to set
 *
 * Sets the repo origin on several package if not already set.
 *
 * Since: 0.1.0
 */
void
dnf_db_ensure_origin_pkglist(DnfDb *db, GPtrArray *pkglist)
{
    DnfPackage *pkg;
    guint i;
    for (i = 0; i < pkglist->len; i++) {
        pkg = g_ptr_array_index (pkglist, i);
        dnf_db_ensure_origin_pkg(db, pkg);
    }
}

/**
 * dnf_db_new:
 * @context: a #DnfContext instance.
 *
 * Creates a new #DnfDb.
 *
 * Returns:(transfer full): a #DnfDb
 *
 * Since: 0.1.0
 **/
DnfDb *
dnf_db_new(DnfContext *context)
{
    DnfDb *db;
    DnfDbPrivate *priv;
    db = g_object_new(DNF_TYPE_DB, NULL);
    priv = GET_PRIVATE(db);
    priv->context = context;
    g_object_add_weak_pointer(G_OBJECT(priv->context),(void **) &priv->context);
    return DNF_DB(db);
}

/**
 * dnf_db_set_enabled:
 * @db: a #DnfDb instance.
 * @enabled: If %FALSE, disable writes
 *
 * If @enabled is %FALSE, makes every API call to change the database
 * a no-op.
 *
 * Since: 0.2.0
 **/
void
dnf_db_set_enabled(DnfDb *db, gboolean enabled)
{
    DnfDbPrivate *priv = GET_PRIVATE(db);
    priv->enabled = enabled;
}
