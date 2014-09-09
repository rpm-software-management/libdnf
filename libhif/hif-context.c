/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014 Richard Hughes <richard@hughsie.com>
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
 * SECTION:hif-context
 * @short_description: High level interface to libhif.
 * @include: libhif.h
 * @stability: Stable
 *
 * This object is a high level interface that does not allow the the user
 * to use objects from librepo, rpm or hawkey directly.
 */

#include "config.h"

#include <rpm/rpmlib.h>

#include <hawkey/query.h>
#include <hawkey/packagelist.h>
#include <librepo/librepo.h>

#include "hif-cleanup.h"
#include "hif-context.h"
#include "hif-context-private.h"
#include "hif-db.h"
#include "hif-goal.h"
#include "hif-keyring.h"
#include "hif-package.h"
#include "hif-transaction.h"
#include "hif-repos.h"
#include "hif-sack.h"
#include "hif-state.h"
#include "hif-utils.h"

typedef struct _HifContextPrivate	HifContextPrivate;
struct _HifContextPrivate
{
	gchar			*repo_dir;
	gchar			*base_arch;
	gchar			*release_ver;
	gchar			*cache_dir;
	gchar			*solv_dir;
	gchar			*lock_dir;
	gchar			*os_info;
	gchar			*arch_info;
	gchar			*install_root;
	gchar			*rpm_verbosity;
	gchar			**native_arches;
	gboolean		 cache_age;
	gboolean		 check_disk_space;
	gboolean		 check_transaction;
	gboolean		 only_trusted;
	gboolean		 keep_cache;
	HifLock			*lock;
	HifTransaction		*transaction;
	GFileMonitor		*monitor_rpmdb;

	/* used to implement a transaction */
	GPtrArray		*sources;
	HifRepos		*repos;
	HifState		*state;		/* used for setup() and run() */
	HyGoal			 goal;
	HySack			 sack;
};

enum {
	SIGNAL_INVALIDATE,
	SIGNAL_LAST
};

static guint signals [SIGNAL_LAST] = { 0 };

G_DEFINE_TYPE (HifContext, hif_context, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HIF_TYPE_CONTEXT, HifContextPrivate))

/**
 * hif_context_finalize:
 **/
static void
hif_context_finalize (GObject *object)
{
	HifContext *context = HIF_CONTEXT (object);
	HifContextPrivate *priv = GET_PRIVATE (context);

	g_free (priv->repo_dir);
	g_free (priv->base_arch);
	g_free (priv->release_ver);
	g_free (priv->cache_dir);
	g_free (priv->solv_dir);
	g_free (priv->lock_dir);
	g_free (priv->rpm_verbosity);
	g_free (priv->install_root);
	g_free (priv->os_info);
	g_free (priv->arch_info);
	g_strfreev (priv->native_arches);
	g_object_unref (priv->lock);
	g_object_unref (priv->state);

	if (priv->transaction != NULL)
		g_object_unref (priv->transaction);
	if (priv->repos != NULL)
		g_object_unref (priv->repos);
	if (priv->sources != NULL)
		g_ptr_array_unref (priv->sources);
	if (priv->goal != NULL)
		hy_goal_free (priv->goal);
	if (priv->sack != NULL)
		hy_sack_free (priv->sack);
	if (priv->monitor_rpmdb != NULL)
		g_object_unref (priv->monitor_rpmdb);

	G_OBJECT_CLASS (hif_context_parent_class)->finalize (object);
}

/**
 * hif_context_init:
 **/
static void
hif_context_init (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	priv->install_root = g_strdup ("/");
	priv->check_disk_space = TRUE;
	priv->check_transaction = TRUE;
	priv->state = hif_state_new ();
	priv->lock = hif_lock_new ();
	priv->cache_age = 60 * 60 * 24 * 7; /* 1 week */
}

/**
 * hif_context_class_init:
 **/
static void
hif_context_class_init (HifContextClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = hif_context_finalize;

	signals [SIGNAL_INVALIDATE] =
		g_signal_new ("invalidate",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (HifContextClass, invalidate),
			      NULL, NULL, g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);

	g_type_class_add_private (klass, sizeof (HifContextPrivate));
}

/**
 * hif_context_get_repo_dir:
 * @context: a #HifContext instance.
 *
 * Gets the context ID.
 *
 * Returns: the context ID, e.g. "fedora-updates"
 *
 * Since: 0.1.0
 **/
const gchar *
hif_context_get_repo_dir (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->repo_dir;
}

/**
 * hif_context_get_base_arch:
 * @context: a #HifContext instance.
 *
 * Gets the context ID.
 *
 * Returns: the base architecture, e.g. "x86_64"
 *
 * Since: 0.1.0
 **/
const gchar *
hif_context_get_base_arch (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->base_arch;
}

/**
 * hif_context_get_os_info:
 * @context: a #HifContext instance.
 *
 * Gets the OS info.
 *
 * Returns: the OS info, e.g. "Linux"
 *
 * Since: 0.1.0
 **/
const gchar *
hif_context_get_os_info (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->os_info;
}

/**
 * hif_context_get_arch_info:
 * @context: a #HifContext instance.
 *
 * Gets the architecture info.
 *
 * Returns: the architecture info, e.g. "x86_64"
 *
 * Since: 0.1.0
 **/
const gchar *
hif_context_get_arch_info (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->arch_info;
}

/**
 * hif_context_get_release_ver:
 * @context: a #HifContext instance.
 *
 * Gets the release version.
 *
 * Returns: the version, e.g. "20"
 *
 * Since: 0.1.0
 **/
const gchar *
hif_context_get_release_ver (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->release_ver;
}

/**
 * hif_context_get_cache_dir:
 * @context: a #HifContext instance.
 *
 * Gets the cache dir to use for metadata files.
 *
 * Returns: fully specified path
 *
 * Since: 0.1.0
 **/
const gchar *
hif_context_get_cache_dir (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->cache_dir;
}

/**
 * hif_context_get_solv_dir:
 * @context: a #HifContext instance.
 *
 * Gets the solve cache directory.
 *
 * Returns: fully specified path
 *
 * Since: 0.1.0
 **/
const gchar *
hif_context_get_solv_dir (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->solv_dir;
}

/**
 * hif_context_get_lock_dir:
 * @context: a #HifContext instance.
 *
 * Gets the lock directory.
 *
 * Returns: fully specified path
 *
 * Since: 0.1.4
 **/
const gchar *
hif_context_get_lock_dir (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->lock_dir;
}

/**
 * hif_context_get_rpm_verbosity:
 * @context: a #HifContext instance.
 *
 * Gets the RPM verbosity string.
 *
 * Returns: the verbosity string, e.g. "info"
 *
 * Since: 0.1.0
 **/
const gchar *
hif_context_get_rpm_verbosity (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->rpm_verbosity;
}

/**
 * hif_context_get_install_root:
 * @context: a #HifContext instance.
 *
 * Gets the install root, by default "/".
 *
 * Returns: the install root, e.g. "/tmp/snapshot"
 *
 * Since: 0.1.0
 **/
const gchar *
hif_context_get_install_root (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->install_root;
}

/**
 * hif_context_get_native_arches:
 * @context: a #HifContext instance.
 *
 * Gets the native architectures, by default "noarch" and "i386".
 *
 * Returns: (transfer none): the native architectures
 *
 * Since: 0.1.0
 **/
const gchar **
hif_context_get_native_arches (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return (const gchar **) priv->native_arches;
}

/**
 * hif_context_get_sources:
 * @context: a #HifContext instance.
 *
 * Gets the sources used by the transaction.
 *
 * Returns: (transfer none): the source list
 *
 * Since: 0.1.0
 **/
GPtrArray *
hif_context_get_sources (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->sources;
}

/**
 * hif_context_get_transaction:
 * @context: a #HifContext instance.
 *
 * Gets the transaction used by the transaction.
 *
 * Returns: (transfer none): the HifTransaction object
 *
 * Since: 0.1.0
 **/
HifTransaction *
hif_context_get_transaction (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->transaction;
}

/**
 * hif_context_get_sack: (skip)
 * @context: a #HifContext instance.
 *
 * Returns: (transfer none): the HySack object
 *
 * Since: 0.1.0
 **/
HySack
hif_context_get_sack (HifContext	*context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->sack;
}

/**
 * hif_context_get_goal: (skip)
 * @context: a #HifContext instance.
 *
 * Returns: (transfer none): the HyGoal object
 *
 * Since: 0.1.0
 **/
HyGoal
hif_context_get_goal (HifContext	*context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->goal;
}

/**
 * hif_context_get_state:
 * @context: a #HifContext instance.
 *
 * Returns: (transfer none): the HifState object
 *
 * Since: 0.1.2
 **/
HifState*
hif_context_get_state (HifContext	*context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->state;
}

/**
 * hif_context_get_check_disk_space:
 * @context: a #HifContext instance.
 *
 * Gets the diskspace check value.
 *
 * Returns: %TRUE if diskspace should be checked before the transaction
 *
 * Since: 0.1.0
 **/
gboolean
hif_context_get_check_disk_space (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->check_disk_space;
}

/**
 * hif_context_get_check_transaction:
 * @context: a #HifContext instance.
 *
 * Gets the test transaction value. A test transaction shouldn't be required
 * with hawkey and it takes extra time to complete, but bad things happen
 * if hawkey ever gets this wrong.
 *
 * Returns: %TRUE if a test transaction should be done
 *
 * Since: 0.1.0
 **/
gboolean
hif_context_get_check_transaction (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->check_transaction;
}

/**
 * hif_context_get_keep_cache:
 * @context: a #HifContext instance.
 *
 * Gets if the downloaded packages are kept.
 *
 * Returns: %TRUE if the packages will not be deleted
 *
 * Since: 0.1.0
 **/
gboolean
hif_context_get_keep_cache (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->keep_cache;
}

/**
 * hif_context_get_only_trusted:
 * @context: a #HifContext instance.
 *
 * Gets if only trusted packages can be installed.
 *
 * Returns: %TRUE if only trusted packages are allowed
 *
 * Since: 0.1.0
 **/
gboolean
hif_context_get_only_trusted (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->only_trusted;
}

/**
 * hif_context_get_cache_age:
 * @context: a #HifContext instance.
 *
 * Gets the maximum cache age.
 *
 * Returns: cache age in seconds
 *
 * Since: 0.1.0
 **/
guint
hif_context_get_cache_age (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	return priv->cache_age;
}

/**
 * hif_context_get_installonly_pkgs:
 * @context: a #HifContext instance.
 *
 * Gets the packages that are allowed to be installed more than once.
 *
 * Returns: (transfer none): array of package names
 */
const gchar **
hif_context_get_installonly_pkgs (HifContext *context)
{
	static const gchar *installonly_pkgs[] = { "kernel",
						   "installonlypkg(kernel)",
						   "installonlypkg(kernel-module)",
						   "installonlypkg(vm)",
						    NULL };
	return installonly_pkgs;
}

/**
 * hif_context_get_installonly_limit:
 * @context: a #HifContext instance.
 *
 * Gets the maximum number of packages for installonly packages
 *
 * Returns: integer value
 */
guint
hif_context_get_installonly_limit (HifContext *context)
{
	return 3;
}

/**
 * hif_context_set_repo_dir:
 * @context: a #HifContext instance.
 * @repo_dir: the repodir, e.g. "/etc/yum.repos.d"
 *
 * Sets the repo directory.
 *
 * Since: 0.1.0
 **/
void
hif_context_set_repo_dir (HifContext *context, const gchar *repo_dir)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	g_free (priv->repo_dir);
	priv->repo_dir = g_strdup (repo_dir);
}

/**
 * hif_context_set_release_ver:
 * @context: a #HifContext instance.
 * @release_ver: the release version, e.g. "20"
 *
 * Sets the release version.
 *
 * Since: 0.1.0
 **/
void
hif_context_set_release_ver (HifContext *context, const gchar *release_ver)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	g_free (priv->release_ver);
	priv->release_ver = g_strdup (release_ver);
}

/**
 * hif_context_set_cache_dir:
 * @context: a #HifContext instance.
 * @cache_dir: the cachedir, e.g. "/var/cache/PackageKit"
 *
 * Sets the cache directory.
 *
 * Since: 0.1.0
 **/
void
hif_context_set_cache_dir (HifContext *context, const gchar *cache_dir)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	g_free (priv->cache_dir);
	priv->cache_dir = g_strdup (cache_dir);
}

/**
 * hif_context_set_solv_dir:
 * @context: a #HifContext instance.
 * @solv_dir: the solve cache, e.g. "/var/cache/PackageKit/hawkey"
 *
 * Sets the solve cache directory.
 *
 * Since: 0.1.0
 **/
void
hif_context_set_solv_dir (HifContext *context, const gchar *solv_dir)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	g_free (priv->solv_dir);
	priv->solv_dir = g_strdup (solv_dir);
}

/**
 * hif_context_set_lock_dir:
 * @context: a #HifContext instance.
 * @lock_dir: the solve cache, e.g. "/var/run"
 *
 * Sets the lock directory.
 *
 * Since: 0.1.4
 **/
void
hif_context_set_lock_dir (HifContext *context, const gchar *lock_dir)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	g_free (priv->lock_dir);
	priv->lock_dir = g_strdup (lock_dir);
}

/**
 * hif_context_set_rpm_verbosity:
 * @context: a #HifContext instance.
 * @rpm_verbosity: the verbosity string, e.g. "info"
 *
 * Sets the RPM verbosity string.
 *
 * Since: 0.1.0
 **/
void
hif_context_set_rpm_verbosity (HifContext *context, const gchar *rpm_verbosity)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	g_free (priv->rpm_verbosity);
	priv->rpm_verbosity = g_strdup (rpm_verbosity);
}

/**
 * hif_context_set_install_root:
 * @context: a #HifContext instance.
 * @install_root: the install root, e.g. "/"
 *
 * Sets the install root to use for installing and removal.
 *
 * Since: 0.1.0
 **/
void
hif_context_set_install_root (HifContext *context, const gchar *install_root)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	g_free (priv->install_root);
	priv->install_root = g_strdup (install_root);
}

/**
 * hif_context_set_check_disk_space:
 * @context: a #HifContext instance.
 * @check_disk_space: %TRUE to check for diskspace
 *
 * Enables or disables the diskspace check.
 *
 * Since: 0.1.0
 **/
void
hif_context_set_check_disk_space (HifContext *context, gboolean check_disk_space)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	priv->check_disk_space = check_disk_space;
}

/**
 * hif_context_set_check_transaction:
 * @context: a #HifContext instance.
 * @check_transaction: %TRUE to do a test transaction
 *
 * Enables or disables the test transaction.
 *
 * Since: 0.1.0
 **/
void
hif_context_set_check_transaction (HifContext *context, gboolean check_transaction)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	priv->check_transaction = check_transaction;
}

/**
 * hif_context_set_keep_cache:
 * @context: a #HifContext instance.
 * @keep_cache: %TRUE keep the packages after installing or updating
 *
 * Enables or disables the automatic package deleting functionality.
 *
 * Since: 0.1.0
 **/
void
hif_context_set_keep_cache (HifContext *context, gboolean keep_cache)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	priv->keep_cache = keep_cache;
}

/**
 * hif_context_set_only_trusted:
 * @context: a #HifContext instance.
 * @only_trusted: %TRUE keep the packages after installing or updating
 *
 * Enables or disables the requirement of trusted packages.
 *
 * Since: 0.1.0
 **/
void
hif_context_set_only_trusted (HifContext *context, gboolean only_trusted)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	priv->only_trusted = only_trusted;
}

/**
 * hif_context_set_cache_age:
 * @context: a #HifContext instance.
 * @cache_age: Maximum cache age in seconds
 *
 * Sets the maximum cache age.
 *
 * Since: 0.1.0
 **/
void
hif_context_set_cache_age (HifContext *context, guint cache_age)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	priv->cache_age = cache_age;
}

/**
 * hif_context_set_os_release:
 **/
static gboolean
hif_context_set_os_release (HifContext *context, GError **error)
{
	_cleanup_free_ gchar *contents = NULL;
	_cleanup_free_ gchar *version = NULL;
	_cleanup_string_free_ GString *str = NULL;
	_cleanup_keyfile_unref_ GKeyFile *key_file = NULL;

	/* make a valid GKeyFile from the .ini data by prepending a header */
	if (!g_file_get_contents ("/etc/os-release", &contents, NULL, NULL))
		return FALSE;
	str = g_string_new (contents);
	g_string_prepend (str, "[os-release]\n");
	key_file = g_key_file_new ();
	if (!g_key_file_load_from_data (key_file,
					 str->str, -1,
					 G_KEY_FILE_NONE,
					 error))
		return FALSE;

	/* get keys */
	version = g_key_file_get_string (key_file,
					 "os-release",
					 "VERSION_ID",
					 error);
	if (version == NULL)
		return FALSE;
	hif_context_set_release_ver (context, version);
	return TRUE;
}

/**
 * hif_context_rpmdb_changed_cb:
 **/
static void
hif_context_rpmdb_changed_cb (GFileMonitor *monitor_,
			      GFile *file, GFile *other_file,
			      GFileMonitorEvent event_type,
			      HifContext *context)
{
	hif_context_invalidate (context, "rpmdb changed");
}

/* A heuristic; check whether /usr exists in the install root */
static gboolean
have_existing_install (HifContext *context)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	_cleanup_free_ gchar *usr_path = g_build_filename (priv->install_root, "usr", NULL);
	return g_file_test (usr_path, G_FILE_TEST_IS_DIR);
}

/**
 * hif_context_get_real_path:
 *
 * Resolves paths like ../../Desktop/bar.rpm to /home/hughsie/Desktop/bar.rpm
 **/
static gchar *
hif_context_get_real_path (const gchar *path)
{
	gchar *real = NULL;
	char *temp;

	/* don't trust realpath one little bit */
	if (path == NULL)
		return NULL;

	/* glibc allocates us a buffer to try and fix some brain damage */
	temp = realpath (path, NULL);
	if (temp == NULL)
		return NULL;
	real = g_strdup (temp);
	free (temp);
	return real;
}

/**
 * hif_context_setup_sack:
 * @context: a #HifContext instance.
 * @state: A #HifState
 * @error: A #GError or %NULL
 *
 * Sets up the internal sack ready for use -- note, this is called automatically
 * when you use hif_context_install(), hif_context_remove() or
 * hif_context_update(), although you may want to call this manually to control
 * the HifState children with correct percentage completion.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.3
 **/
gboolean
hif_context_setup_sack (HifContext *context, HifState *state, GError **error)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	gboolean ret;
	gint rc;
	_cleanup_free_ gchar *solv_dir_real = NULL;

	/* create empty sack */
	solv_dir_real = hif_context_get_real_path (priv->solv_dir);
	priv->sack = hy_sack_create (solv_dir_real, NULL,
				     priv->install_root,
				     HY_MAKE_CACHE_DIR);
	if (priv->sack == NULL) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "failed to create sack cache in %s for %s",
			     priv->solv_dir, priv->install_root);
		return FALSE;
	}
	hy_sack_set_installonly (priv->sack, hif_context_get_installonly_pkgs (context));
	hy_sack_set_installonly_limit (priv->sack, hif_context_get_installonly_limit (context));

	/* add installed packages */
	if (have_existing_install (context)) {
		rc = hy_sack_load_system_repo (priv->sack, NULL, HY_BUILD_CACHE);
		if (!hif_rc_to_gerror (rc, error)) {
			g_prefix_error (error, "Failed to load system repo: ");
			return FALSE;
		}
	}

	/* creates repo for command line rpms */
	hy_sack_create_cmdline_repo (priv->sack);

	/* add remote */
	ret = hif_sack_add_sources (priv->sack,
				    priv->sources,
				    priv->cache_age,
				    HIF_SACK_ADD_FLAG_FILELISTS,
				    state,
				    error);
	if (!ret)
		return FALSE;

	/* create goal */
	priv->goal = hy_goal_create (priv->sack);
	return TRUE;
}

/**
 * hif_context_ensure_exists:
 **/
static gboolean
hif_context_ensure_exists (const gchar *directory, GError **error)
{
	if (g_file_test (directory, G_FILE_TEST_EXISTS))
		return TRUE;
	if (g_mkdir_with_parents (directory, 0700) != 0) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "Failed to create: %s", directory);
		return FALSE;
	}
	return TRUE;
}

/**
 * hif_context_setup:
 * @context: a #HifContext instance.
 * @cancellable: A #GCancellable or %NULL
 * @error: A #GError or %NULL
 *
 * Sets up the context ready for use.
 *
 * This function will not do significant amounts of i/o or download new
 * metadata. Use hif_context_setup_sack() if you want to populate the internal
 * sack as well.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_context_setup (HifContext *context,
		   GCancellable *cancellable,
		   GError **error)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	const gchar *value;
	_cleanup_free_ char *rpmdb_path = NULL;
	_cleanup_object_unref_ GFile *file_rpmdb = NULL;

	/* check essential things are set */
	if (priv->solv_dir == NULL) {
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_INTERNAL_ERROR,
				     "solv_dir not set");
		return FALSE;
	}

	/* ensure directories exist */
	if (priv->repo_dir != NULL) {
		if (!hif_context_ensure_exists (priv->repo_dir, error))
			return FALSE;
	}
	if (priv->cache_dir != NULL) {
		if (!hif_context_ensure_exists (priv->cache_dir, error))
			return FALSE;
	}
	if (priv->solv_dir != NULL) {
		if (!hif_context_ensure_exists (priv->solv_dir, error))
			return FALSE;
	}
	if (priv->lock_dir != NULL) {
		hif_lock_set_lock_dir (priv->lock, priv->lock_dir);
		if (!hif_context_ensure_exists (priv->lock_dir, error))
			return FALSE;
	}
	if (priv->install_root != NULL) {
		if (!hif_context_ensure_exists (priv->install_root, error))
			return FALSE;
	}

	/* connect if set */
	if (cancellable != NULL)
		hif_state_set_cancellable (priv->state, cancellable);

	if (rpmReadConfigFiles (NULL, NULL) != 0) {
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_INTERNAL_ERROR,
				     "failed to read rpm config files");
		return FALSE;
	}

	/* get info from RPM */
	rpmGetOsInfo (&value, NULL);
	priv->os_info = g_strdup (value);
	rpmGetArchInfo (&value, NULL);
	priv->arch_info = g_strdup (value);
	if (g_strcmp0 (value, "i486") == 0 ||
	    g_strcmp0 (value, "i586") == 0 ||
	    g_strcmp0 (value, "i686") == 0) {
		value = "i386";
	} else if (g_strcmp0 (value, "armv7l") == 0 ||
		   g_strcmp0 (value, "armv6l") == 0 ||
		   g_strcmp0 (value, "armv5tejl") == 0 ||
		   g_strcmp0 (value, "armv5tel") == 0) {
		value = "arm";
	} else if (g_strcmp0 (value, "armv7hnl") == 0 ||
		 g_strcmp0 (value, "armv7hl") == 0) {
		value = "armhfp";
	}
	priv->base_arch = g_strdup (value);

	/* setup native arches */
	priv->native_arches = g_new0 (gchar *, 3);
	priv->native_arches[0] = g_strdup (priv->arch_info);
	priv->native_arches[1] = g_strdup ("noarch");

	/* get info from OS release file */
	if (!hif_context_set_os_release (context, error))
		return FALSE;

	/* setup RPM */
	priv->repos = hif_repos_new (context);
	priv->sources = hif_repos_get_sources (priv->repos, error);
	if (priv->sources == NULL)
		return FALSE;
	priv->transaction = hif_transaction_new (context);
	hif_transaction_set_sources (priv->transaction, priv->sources);
	hif_transaction_set_flags (priv->transaction,
				   HIF_TRANSACTION_FLAG_ONLY_TRUSTED);

	/* setup a file monitor on the rpmdb */
	rpmdb_path = g_build_filename (priv->install_root, "var/lib/rpm/Packages", NULL);
	file_rpmdb = g_file_new_for_path (rpmdb_path);
	priv->monitor_rpmdb = g_file_monitor_file (file_rpmdb,
						   G_FILE_MONITOR_NONE,
						   NULL,
						   error);
	if (priv->monitor_rpmdb == NULL)
		return FALSE;
	g_signal_connect (priv->monitor_rpmdb, "changed",
			  G_CALLBACK (hif_context_rpmdb_changed_cb), context);

	return TRUE;
}

/**
 * hif_context_run:
 * @context: a #HifContext instance.
 * @cancellable: A #GCancellable or %NULL
 * @error: A #GError or %NULL
 *
 * Runs the context installing or removing packages as required
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_context_run (HifContext *context, GCancellable *cancellable, GError **error)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	HifState *state_local;
	gboolean ret;

	/* connect if set */
	hif_state_reset (priv->state);
	if (cancellable != NULL)
		hif_state_set_cancellable (priv->state, cancellable);

	ret = hif_state_set_steps (priv->state, error,
				   5,	/* depsolve */
				   50,	/* download */
				   45,	/* commit */
				   -1);
	if (!ret)
		return FALSE;

	/* depsolve */
	state_local = hif_state_get_child (priv->state);
	ret = hif_transaction_depsolve (priv->transaction,
					priv->goal,
					state_local,
					error);
	if (!ret)
		return FALSE;

	/* this section done */
	if (!hif_state_done (priv->state, error))
		return FALSE;

	/* download */
	state_local = hif_state_get_child (priv->state);
	ret = hif_transaction_download (priv->transaction,
					state_local,
					error);
	if (!ret)
		return FALSE;

	/* this section done */
	if (!hif_state_done (priv->state, error))
		return FALSE;

	/* commit set up transaction */
	state_local = hif_state_get_child (priv->state);
	ret = hif_transaction_commit (priv->transaction,
				      priv->goal,
				      state_local,
				      error);
	if (!ret)
		return FALSE;

	/* this section done */
	return hif_state_done (priv->state, error);
}

/**
 * hif_context_install:
 * @context: a #HifContext instance.
 * @name: A package or group name, e.g. "firefox" or "@gnome-desktop"
 * @error: A #GError or %NULL
 *
 * Finds a remote package and marks it to be installed.
 *
 * If multiple packages are available then only the newest package is installed.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_context_install (HifContext *context, const gchar *name, GError **error)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	HyPackageList pkglist;
	HyPackage pkg;
	HyQuery query;
	gboolean ret = TRUE;
	guint i;

	/* create sack and add sources */
	if (priv->sack == NULL) {
		hif_state_reset (priv->state);
		ret = hif_context_setup_sack (context, priv->state, error);
		if (!ret)
			return FALSE;
	}

	/* find a newest remote package to install */
	query = hy_query_create (priv->sack);
	hy_query_filter_latest_per_arch (query, TRUE);
	hy_query_filter_in (query, HY_PKG_ARCH, HY_EQ,
			    (const gchar **) priv->native_arches);
	hy_query_filter (query, HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
	hy_query_filter (query, HY_PKG_ARCH, HY_NEQ, "src");
	hy_query_filter (query, HY_PKG_NAME, HY_EQ, name);
	pkglist = hy_query_run (query);

	if (hy_packagelist_count (pkglist) == 0) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_PACKAGE_NOT_FOUND,
			     "No package '%s' found", name);
		return FALSE;
	}

	/* add each package */
	FOR_PACKAGELIST(pkg, pkglist, i) {
		hif_package_set_user_action (pkg, TRUE);
		g_debug ("adding %s-%s to goal", hy_package_get_name (pkg), hy_package_get_evr (pkg));
		hy_goal_install (priv->goal, pkg);
	}
	hy_packagelist_free (pkglist);
	hy_query_free (query);
	return TRUE;
}

/**
 * hif_context_remove:
 * @context: a #HifContext instance.
 * @name: A package or group name, e.g. "firefox" or "@gnome-desktop"
 * @error: A #GError or %NULL
 *
 * Finds an installed package and marks it to be removed.
 *
 * If multiple packages are available then only the oldest package is removed.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_context_remove (HifContext *context, const gchar *name, GError **error)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	HyPackageList pkglist;
	HyPackage pkg;
	HyQuery query;
	gboolean ret = TRUE;
	guint i;

	/* create sack and add sources */
	if (priv->sack == NULL) {
		hif_state_reset (priv->state);
		ret = hif_context_setup_sack (context, priv->state, error);
		if (!ret)
			return FALSE;
	}

	/* find a newest remote package to install */
	query = hy_query_create (priv->sack);
	hy_query_filter_latest_per_arch (query, TRUE);
	hy_query_filter (query, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
	hy_query_filter (query, HY_PKG_ARCH, HY_NEQ, "src");
	hy_query_filter (query, HY_PKG_NAME, HY_EQ, name);
	pkglist = hy_query_run (query);

	/* add each package */
	FOR_PACKAGELIST(pkg, pkglist, i) {
		hif_package_set_user_action (pkg, TRUE);
		hy_goal_erase (priv->goal, pkg);
	}
	hy_packagelist_free (pkglist);
	hy_query_free (query);
	return TRUE;
}

/**
 * hif_context_update:
 * @context: a #HifContext instance.
 * @name: A package or group name, e.g. "firefox" or "@gnome-desktop"
 * @error: A #GError or %NULL
 *
 * Finds an installed and remote package and marks it to be updated.
 *
 * If multiple packages are available then the newest package is updated to.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_context_update (HifContext *context, const gchar *name, GError **error)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	HyPackageList pkglist;
	HyPackage pkg;
	HyQuery query;
	gboolean ret = TRUE;
	guint i;

	/* create sack and add sources */
	if (priv->sack == NULL) {
		hif_state_reset (priv->state);
		ret = hif_context_setup_sack (context, priv->state, error);
		if (!ret)
			return FALSE;
	}

	/* find a newest remote package to install */
	query = hy_query_create (priv->sack);
	hy_query_filter_latest_per_arch (query, TRUE);
	hy_query_filter_in (query, HY_PKG_ARCH, HY_EQ,
			    (const gchar **) priv->native_arches);
	hy_query_filter (query, HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
	hy_query_filter (query, HY_PKG_NAME, HY_EQ, name);
	pkglist = hy_query_run (query);

	/* add each package */
	FOR_PACKAGELIST(pkg, pkglist, i) {
		hif_package_set_user_action (pkg, TRUE);
		if (hif_package_is_installonly (pkg))
			hy_goal_install (priv->goal, pkg);
		else
			hy_goal_upgrade_to (priv->goal, pkg);
	}
	hy_packagelist_free (pkglist);
	hy_query_free (query);
	return TRUE;
}

/**
 * hif_context_repo_set_data:
 **/
static gboolean
hif_context_repo_set_data (HifContext *context,
			   const gchar *repo_id,
			   gboolean enabled,
			   GError **error)
{
	HifContextPrivate *priv = GET_PRIVATE (context);
	HifSource *src = NULL;
	HifSource *src_tmp;
	guint i;

	/* find a source with a matching ID */
	for (i = 0; i < priv->sources->len; i++) {
		src_tmp = g_ptr_array_index (priv->sources, i);
		if (g_strcmp0 (hif_source_get_id (src_tmp), repo_id) == 0) {
			src = src_tmp;
			break;
		}
	}

	/* nothing found */
	if (src == NULL) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "repo %s not found", repo_id);
		return FALSE;
	}

	/* this is runtime only */
	hif_source_set_enabled (src, enabled);
	return TRUE;
}

/**
 * hif_context_repo_enable:
 * @context: a #HifContext instance.
 * @repo_id: A repo_id, e.g. "fedora-rawhide"
 * @error: A #GError or %NULL
 *
 * Enables a specific repo.
 *
 * This must be done before hif_context_setup() is called.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_context_repo_enable (HifContext *context,
			 const gchar *repo_id,
			 GError **error)
{
	return hif_context_repo_set_data (context, repo_id, TRUE, error);
}

/**
 * hif_context_repo_disable:
 * @context: a #HifContext instance.
 * @repo_id: A repo_id, e.g. "fedora-rawhide"
 * @error: A #GError or %NULL
 *
 * Disables a specific repo.
 *
 * This must be done before hif_context_setup() is called.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_context_repo_disable (HifContext *context,
			  const gchar *repo_id,
			  GError **error)
{
	return hif_context_repo_set_data (context, repo_id, FALSE, error);
}

/**
 * hif_context_commit:
 * @context: a #HifContext instance.
 * @state: A #HifState
 * @error: A #GError or %NULL
 *
 * Commits a context, which applies changes to the live system.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_context_commit (HifContext *context, HifState *state, GError **error)
{
	HifContextPrivate *priv = GET_PRIVATE (context);

	/* run the transaction */
	return hif_transaction_commit (priv->transaction,
				       priv->goal,
				       state,
				       error);
}

/**
 * hif_context_invalidate:
 * @context: a #HifContext instance.
 * @message: the reason for invalidating
 *
 * Emits a signal which signals that the context is invalid.
 *
 * Since: 0.1.0
 **/
void
hif_context_invalidate (HifContext *context, const gchar *message)
{
	g_signal_emit (context, signals [SIGNAL_INVALIDATE], 0, message);
}

/**
 * hif_context_new:
 *
 * Creates a new #HifContext.
 *
 * Returns: (transfer full): a #HifContext
 *
 * Since: 0.1.0
 **/
HifContext *
hif_context_new (void)
{
	HifContext *context;
	context = g_object_new (HIF_TYPE_CONTEXT, NULL);
	lr_global_init ();
	return HIF_CONTEXT (context);
}
