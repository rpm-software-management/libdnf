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
 * SECTION:hif-transaction
 * @short_description: High level interface to librpm.
 * @include: libhif.h
 * @stability: Stable
 *
 * This object represents an RPM transaction.
 */

#include "config.h"

#include <rpm/rpmlib.h>
#include <rpm/rpmts.h>
#include <rpm/rpmlog.h>

#include <hawkey/packagelist.h>
#include <hawkey/query.h>
#include <hawkey/sack.h>
#include <hawkey/util.h>

#include "hif-context-private.h"
#include "hif-db.h"
#include "hif-goal.h"
#include "hif-keyring.h"
#include "hif-package.h"
#include "hif-rpmts.h"
#include "hif-state.h"
#include "hif-transaction.h"
#include "hif-utils.h"

typedef enum {
	HIF_TRANSACTION_STEP_STARTED,
	HIF_TRANSACTION_STEP_PREPARING,
	HIF_TRANSACTION_STEP_WRITING,
	HIF_TRANSACTION_STEP_IGNORE
} HifTransactionStep;

typedef struct _HifTransactionPrivate	HifTransactionPrivate;
struct _HifTransactionPrivate
{
	HifDb			*db;
	rpmKeyring		 keyring;
	rpmts			 ts;
	HifContext		*context;
	GPtrArray		*sources;
	guint			 uid;

	/* previously in the helper */
	HifState		*state;
	HifState		*child;
	FD_t			 fd;
	HifTransactionStep	 step;
	GTimer			*timer;
	guint			 last_progress;
	GPtrArray		*remove;
	GPtrArray		*remove_helper;
	GPtrArray		*install;
	GPtrArray		*pkgs_to_download;
	guint64			 flags;
};

G_DEFINE_TYPE (HifTransaction, hif_transaction, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), HIF_TYPE_TRANSACTION, HifTransactionPrivate))

/**
 * hif_transaction_finalize:
 **/
static void
hif_transaction_finalize (GObject *object)
{
	HifTransaction *transaction = HIF_TRANSACTION (object);
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);

	g_ptr_array_unref (priv->pkgs_to_download);
	g_object_unref (priv->context);
	g_timer_destroy (priv->timer);
	rpmKeyringFree (priv->keyring);
	rpmtsFree (priv->ts);

	if (priv->db != NULL)
		g_object_unref (priv->db);
	if (priv->sources != NULL)
		g_ptr_array_unref (priv->sources);
	if (priv->install != NULL)
		g_ptr_array_unref (priv->install);
	if (priv->remove != NULL)
		g_ptr_array_unref (priv->remove);
	if (priv->remove_helper != NULL)
		g_ptr_array_unref (priv->remove_helper);

	G_OBJECT_CLASS (hif_transaction_parent_class)->finalize (object);
}

/**
 * hif_transaction_init:
 **/
static void
hif_transaction_init (HifTransaction *transaction)
{
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);
	priv->ts = rpmtsCreate ();
	priv->keyring = rpmtsGetKeyring (priv->ts, 1);
	priv->timer = g_timer_new ();
	priv->pkgs_to_download = g_ptr_array_new_with_free_func ((GDestroyNotify) hy_package_free);
}

/**
 * hif_transaction_class_init:
 **/
static void
hif_transaction_class_init (HifTransactionClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = hif_transaction_finalize;

	g_type_class_add_private (klass, sizeof (HifTransactionPrivate));
}

/**
 * hif_transaction_get_flags:
 * @transaction: a #HifTransaction instance.
 *
 * Gets the transaction flags.
 *
 * Returns: the transaction flags used for this transaction
 *
 * Since: 0.1.0
 **/
guint64
hif_transaction_get_flags (HifTransaction *transaction)
{
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);
	return priv->flags;
}

/**
 * hif_transaction_get_remote_pkgs:
 * @transaction: a #HifTransaction instance.
 *
 * Gets the packages that will be downloaded in hif_transaction_download().
 *
 * The hif_transaction_depsolve() function must have been called before this
 * function will return sensible results.
 *
 * Returns: (transfer none): the list of packages
 *
 * Since: 0.1.0
 **/
GPtrArray *
hif_transaction_get_remote_pkgs (HifTransaction *transaction)
{
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);
	return priv->pkgs_to_download;
}

/**
 * hif_transaction_get_db:
 * @transaction: a #HifTransaction instance.
 *
 * Gets the database instance used for this transaction.
 *
 * Returns: (transfer none): the #HifDb
 *
 * Since: 0.1.0
 **/
HifDb *
hif_transaction_get_db (HifTransaction *transaction)
{
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);
	return priv->db;
}

/**
 * hif_transaction_set_sources:
 * @transaction: a #HifTransaction instance.
 * @sources: the sources to use with the transaction
 *
 * Sets the list of sources.
 *
 * Since: 0.1.0
 **/
void
hif_transaction_set_sources (HifTransaction *transaction, GPtrArray *sources)
{
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);
	if (priv->sources != NULL)
		g_ptr_array_unref (priv->sources);
	priv->sources = g_ptr_array_ref (sources);
}

/**
 * hif_transaction_set_uid:
 * @transaction: a #HifTransaction instance.
 * @uid: the uid
 *
 * Sets the user ID for the person who started this transaction.
 *
 * Since: 0.1.0
 **/
void
hif_transaction_set_uid (HifTransaction *transaction, guint uid)
{
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);
	priv->uid = uid;
}

/**
 * hif_transaction_set_flags:
 * @transaction: a #HifTransaction instance.
 * @flags: the flags, e.g. %HIF_TRANSACTION_FLAG_ONLY_TRUSTED
 *
 * Sets the flags used for this transaction.
 *
 * Since: 0.1.0
 **/
void
hif_transaction_set_flags (HifTransaction *transaction, guint64 flags)
{
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);
	priv->flags = flags;
}

/**
 * hif_transaction_ensure_source:
 */
static gboolean
hif_transaction_ensure_source (GPtrArray *sources, HyPackage pkg, GError **error)
{
	HifSource *src;
	char *location;
	gboolean ret = TRUE;
	guint i;

	/* this is a local file */
	if (g_strcmp0 (hy_package_get_reponame (pkg),
		       HY_CMDLINE_REPO_NAME) == 0) {
		location = hy_package_get_location (pkg);
		hif_package_set_filename (pkg, location);
		hy_free (location);
		goto out;
	}

	/* get repo */
	if (hy_package_installed (pkg))
		goto out;
	for (i = 0; i < sources->len; i++) {
		src = g_ptr_array_index (sources, i);
		if (g_strcmp0 (hy_package_get_reponame (pkg),
			       hif_source_get_id (src)) == 0) {
			hif_package_set_source (pkg, src);
			goto out;
		}
	}

	/* not found */
	ret = FALSE;
	g_set_error (error,
		     HIF_ERROR,
		     HIF_ERROR_INTERNAL_ERROR,
		     "Failed to ensure %s as source not found",
		     hy_package_get_name (pkg));
out:
	return ret;
}

#if 0
/**
 * hif_transaction_ensure_source_list:
 */
static gboolean
hif_transaction_ensure_source_list (GPtrArray *sources,
				    HyPackageList pkglist,
				    GError **error)
{
	gboolean ret = TRUE;
	guint i;
	HyPackage pkg;

	FOR_PACKAGELIST(pkg, pkglist, i) {
		ret = hif_transaction_ensure_source (sources, pkg, error);
		if (!ret)
			goto out;
	}
out:
	return ret;
}
#endif

/**
 * hif_transaction_check_untrusted:
 */
static gboolean
hif_transaction_check_untrusted (rpmKeyring keyring,
				 GPtrArray *sources,
				 HyGoal goal,
				 GError **error)
{
	const gchar *filename;
	gboolean ret = TRUE;
	GPtrArray *install;
	guint i;
	HyPackage pkg;

	/* find a list of all the packages we might have to download */
	install = hif_goal_get_packages (goal,
					 HIF_PACKAGE_INFO_INSTALL,
					 HIF_PACKAGE_INFO_REINSTALL,
					 HIF_PACKAGE_INFO_DOWNGRADE,
					 HIF_PACKAGE_INFO_UPDATE,
					 -1);
	if (install->len == 0)
		goto out;

	/* find any packages in untrusted repos */
	for (i = 0; i < install->len; i++) {
		pkg = g_ptr_array_index (install, i);

		/* ensure the filename is set */
		ret = hif_transaction_ensure_source (sources, pkg, error);
		if (!ret) {
			g_prefix_error (error, "Failed to check untrusted: ");
			goto out;
		}

		/* find the location of the local file */
		filename = hif_package_get_filename (pkg);
		if (filename == NULL) {
			ret = FALSE;
			g_set_error (error,
				     HIF_ERROR,
				     HIF_ERROR_FILE_NOT_FOUND,
				     "Downloaded file for %s not found",
				     hy_package_get_name (pkg));
			goto out;
		}

		/* check file */
		ret = hif_keyring_check_untrusted_file (keyring,
							filename,
							error);
		if (!ret)
			goto out;
	}
out:
	g_ptr_array_unref (install);
	return ret;
}

/**
 * hif_transaction_rpmcb_type_to_string:
 **/
static const gchar *
hif_transaction_rpmcb_type_to_string (const rpmCallbackType what)
{
	const gchar *type = NULL;
	switch (what) {
	case RPMCALLBACK_UNKNOWN:
		type = "unknown";
		break;
	case RPMCALLBACK_INST_PROGRESS:
		type = "install-progress";
		break;
	case RPMCALLBACK_INST_START:
		type = "install-start";
		break;
	case RPMCALLBACK_INST_OPEN_FILE:
		type = "install-open-file";
		break;
	case RPMCALLBACK_INST_CLOSE_FILE:
		type = "install-close-file";
		break;
	case RPMCALLBACK_TRANS_PROGRESS:
		type = "transaction-progress";
		break;
	case RPMCALLBACK_TRANS_START:
		type = "transaction-start";
		break;
	case RPMCALLBACK_TRANS_STOP:
		type = "transaction-stop";
		break;
	case RPMCALLBACK_UNINST_PROGRESS:
		type = "uninstall-progress";
		break;
	case RPMCALLBACK_UNINST_START:
		type = "uninstall-start";
		break;
	case RPMCALLBACK_UNINST_STOP:
		type = "uninstall-stop";
		break;
	case RPMCALLBACK_REPACKAGE_PROGRESS:
		type = "repackage-progress";
		break;
	case RPMCALLBACK_REPACKAGE_START:
		type = "repackage-start";
		break;
	case RPMCALLBACK_REPACKAGE_STOP:
		type = "repackage-stop";
		break;
	case RPMCALLBACK_UNPACK_ERROR:
		type = "unpack-error";
		break;
	case RPMCALLBACK_CPIO_ERROR:
		type = "cpio-error";
		break;
	case RPMCALLBACK_SCRIPT_ERROR:
		type = "script-error";
		break;
	case RPMCALLBACK_SCRIPT_START:
		type = "script-start";
		break;
	case RPMCALLBACK_SCRIPT_STOP:
		type = "script-stop";
		break;
	case RPMCALLBACK_INST_STOP:
		type = "install-stop";
		break;
	}
	return type;
}

/**
 * hif_find_pkg_from_header:
 **/
static HyPackage
hif_find_pkg_from_header (GPtrArray *array, Header hdr)
{
	const gchar *arch;
	const gchar *name;
	const gchar *release;
	const gchar *version;
	guint epoch;
	guint i;
	HyPackage pkg;

	/* get details */
	name = headerGetString (hdr, RPMTAG_NAME);
	epoch = headerGetNumber (hdr, RPMTAG_EPOCH);
	version = headerGetString (hdr, RPMTAG_VERSION);
	release = headerGetString (hdr, RPMTAG_RELEASE);
	arch = headerGetString (hdr, RPMTAG_ARCH);

	/* find in array */
	for (i = 0; i < array->len; i++) {
		pkg = g_ptr_array_index (array, i);
		if (g_strcmp0 (name, hy_package_get_name (pkg)) != 0)
			continue;
		if (g_strcmp0 (version, hy_package_get_version (pkg)) != 0)
			continue;
		if (g_strcmp0 (release, hy_package_get_release (pkg)) != 0)
			continue;
		if (g_strcmp0 (arch, hy_package_get_arch (pkg)) != 0)
			continue;
		if (epoch != hy_package_get_epoch (pkg))
			continue;
		return pkg;
	}
	return NULL;
}

/**
 * hif_find_pkg_from_filename_suffix:
 **/
static HyPackage
hif_find_pkg_from_filename_suffix (GPtrArray *array,
				   const gchar *filename_suffix)
{
	const gchar *filename;
	guint i;
	HyPackage pkg;

	/* find in array */
	for (i = 0; i < array->len; i++) {
		pkg = g_ptr_array_index (array, i);
		filename = hif_package_get_filename (pkg);
		if (filename == NULL)
			continue;
		if (g_str_has_suffix (filename, filename_suffix))
			return pkg;
	}
	return NULL;
}

/**
 * hif_find_pkg_from_name:
 **/
static HyPackage
hif_find_pkg_from_name (GPtrArray *array, const gchar *pkgname)
{
	guint i;
	HyPackage pkg;

	/* find in array */
	for (i = 0; i < array->len; i++) {
		pkg = g_ptr_array_index (array, i);
		if (g_strcmp0 (hy_package_get_name (pkg), pkgname) == 0)
			return pkg;
	}
	return NULL;
}

/**
 * hif_transaction_ts_progress_cb:
 **/
static void *
hif_transaction_ts_progress_cb (const void *arg,
				const rpmCallbackType what,
				const rpm_loff_t amount,
				const rpm_loff_t total,
				fnpyKey key, void *data)
{
	HifTransaction *transaction = HIF_TRANSACTION (data);
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);
	const char *filename = (const char *) key;
	const gchar *name = NULL;
	gboolean ret;
	GError *error_local = NULL;
	guint percentage;
	guint speed;
	Header hdr = (Header) arg;
	HyPackage pkg;
	HifPackageInfo action;
	void *rc = NULL;

	if (hdr != NULL)
		name = headerGetString (hdr, RPMTAG_NAME);
	g_debug ("phase: %s (%i/%i, %s/%s)",
		 hif_transaction_rpmcb_type_to_string (what),
		 (gint32) amount,
		 (gint32) total,
		 (const gchar *) key,
		 name);

	switch (what) {
	case RPMCALLBACK_INST_OPEN_FILE:

		/* valid? */
		if (filename == NULL || filename[0] == '\0')
			return NULL;

		/* open the file and return file descriptor */
		priv->fd = Fopen (filename, "r.ufdio");
		return (void *) priv->fd;
		break;

	case RPMCALLBACK_INST_CLOSE_FILE:

		/* just close the file */
		if (priv->fd != NULL) {
			Fclose (priv->fd);
			priv->fd = NULL;
		}
		break;

	case RPMCALLBACK_INST_START:

		/* find pkg */
		pkg = hif_find_pkg_from_filename_suffix (priv->install,
							 filename);
		if (pkg == NULL)
			g_assert_not_reached ();

		/* map to correct action code */
		action = hif_package_get_action (pkg);
		if (action == HIF_PACKAGE_INFO_UNKNOWN)
			action = HIF_PACKAGE_INFO_INSTALL;

		/* install start */
		priv->step = HIF_TRANSACTION_STEP_WRITING;
		priv->child = hif_state_get_child (priv->state);
		hif_state_action_start (priv->child,
					action,
					hif_package_get_id (pkg));
		g_debug ("install start: %s size=%i", filename, (gint32) total);
		break;

	case RPMCALLBACK_UNINST_START:

		/* invalid? */
		if (filename == NULL) {
			g_debug ("no filename set in uninst-start with total %i",
				 (gint32) total);
			priv->step = HIF_TRANSACTION_STEP_WRITING;
			break;
		}

		/* find pkg */
		pkg = hif_find_pkg_from_name (priv->remove, name);
		if (pkg != NULL)
			pkg = hif_find_pkg_from_name (priv->remove_helper, name);
		if (pkg == NULL) {
			pkg = hif_find_pkg_from_filename_suffix (priv->remove,
								 filename);
		}
		if (pkg == NULL) {
			g_debug ("cannot find %s", filename);
			break;
		}

		/* map to correct action code */
		action = hif_package_get_action (pkg);
		if (action == HIF_PACKAGE_INFO_UNKNOWN)
			action = HIF_PACKAGE_INFO_REMOVE;

		/* remove start */
		priv->step = HIF_TRANSACTION_STEP_WRITING;
		priv->child = hif_state_get_child (priv->state);
		hif_state_action_start (priv->child,
					action,
					hif_package_get_id (pkg));
		g_debug ("remove start: %s size=%i", filename, (gint32) total);
		break;

	case RPMCALLBACK_TRANS_PROGRESS:
	case RPMCALLBACK_INST_PROGRESS:

		/* we're preparing the transaction */
		if (priv->step == HIF_TRANSACTION_STEP_PREPARING ||
		    priv->step == HIF_TRANSACTION_STEP_IGNORE) {
			g_debug ("ignoring preparing %i / %i",
				 (gint32) amount, (gint32) total);
			break;
		}

		/* work out speed */
		speed = (amount - priv->last_progress) /
				g_timer_elapsed (priv->timer, NULL);
		hif_state_set_speed (priv->state, speed);
		priv->last_progress = amount;
		g_timer_reset (priv->timer);

		/* progress */
		percentage = (100.0f / (gfloat) total) * (gfloat) amount;
		if (priv->child != NULL)
			hif_state_set_percentage (priv->child, percentage);

		/* update UI */
		pkg = hif_find_pkg_from_header (priv->install, hdr);
		if (pkg == NULL) {
			pkg = hif_find_pkg_from_filename_suffix (priv->install,
								 filename);
		}
		if (pkg == NULL) {
			g_debug ("cannot find %s (%s)", filename, name);
			break;
		}

		hif_state_set_package_progress (priv->state,
						hif_package_get_id (pkg),
						HIF_STATE_ACTION_INSTALL,
						percentage);
		break;

	case RPMCALLBACK_UNINST_PROGRESS:

		/* we're preparing the transaction */
		if (priv->step == HIF_TRANSACTION_STEP_PREPARING ||
		    priv->step == HIF_TRANSACTION_STEP_IGNORE) {
			g_debug ("ignoring preparing %i / %i",
				 (gint32) amount, (gint32) total);
			break;
		}

		/* progress */
		percentage = (100.0f / (gfloat) total) * (gfloat) amount;
		if (priv->child != NULL)
			hif_state_set_percentage (priv->child, percentage);

		/* update UI */
		pkg = hif_find_pkg_from_header (priv->remove, hdr);
		if (pkg == NULL && filename != NULL) {
			pkg = hif_find_pkg_from_filename_suffix (priv->remove,
								 filename);
		}
		if (pkg == NULL && name != NULL)
			pkg = hif_find_pkg_from_name (priv->remove, name);
		if (pkg == NULL && name != NULL)
			pkg = hif_find_pkg_from_name (priv->remove_helper, name);
		if (pkg == NULL) {
			g_warning ("cannot find %s", name);
			break;
		}
		hif_state_set_package_progress (priv->state,
						hif_package_get_id (pkg),
						HIF_STATE_ACTION_REMOVE,
						percentage);
		break;

	case RPMCALLBACK_TRANS_START:

		/* we setup the state */
		g_debug ("preparing transaction with %i items", (gint32) total);
		if (priv->step == HIF_TRANSACTION_STEP_IGNORE)
			break;

		hif_state_set_number_steps (priv->state, total);
		priv->step = HIF_TRANSACTION_STEP_PREPARING;
		break;

	case RPMCALLBACK_TRANS_STOP:

		/* don't do anything */
		break;

	case RPMCALLBACK_INST_STOP:
	case RPMCALLBACK_UNINST_STOP:

		/* phase complete */
		ret = hif_state_done (priv->state, &error_local);
		if (!ret) {
			g_warning ("state increment failed: %s",
				   error_local->message);
			g_error_free (error_local);
		}
		break;

	case RPMCALLBACK_UNPACK_ERROR:
	case RPMCALLBACK_CPIO_ERROR:
	case RPMCALLBACK_SCRIPT_ERROR:
	case RPMCALLBACK_SCRIPT_START:
	case RPMCALLBACK_SCRIPT_STOP:
	case RPMCALLBACK_UNKNOWN:
	case RPMCALLBACK_REPACKAGE_PROGRESS:
	case RPMCALLBACK_REPACKAGE_START:
	case RPMCALLBACK_REPACKAGE_STOP:
		g_debug ("%s uninteresting",
			 hif_transaction_rpmcb_type_to_string (what));
		break;
	default:
		g_warning ("unknown transaction phase: %u (%s)",
			   what,
			   hif_transaction_rpmcb_type_to_string (what));
		break;
	}
	return rc;
}

/**
 * hif_rpm_verbosity_string_to_value:
 **/
static gint
hif_rpm_verbosity_string_to_value (const gchar *value)
{
	if (g_strcmp0 (value, "critical") == 0)
		return RPMLOG_CRIT;
	if (g_strcmp0 (value, "emergency") == 0)
		return RPMLOG_EMERG;
	if (g_strcmp0 (value, "error") == 0)
		return RPMLOG_ERR;
	if (g_strcmp0 (value, "warn") == 0)
		return RPMLOG_WARNING;
	if (g_strcmp0 (value, "debug") == 0)
		return RPMLOG_DEBUG;
	if (g_strcmp0 (value, "info") == 0)
		return RPMLOG_INFO;
	return RPMLOG_EMERG;
}

/**
 * hif_transaction_delete_packages:
 **/
static gboolean
hif_transaction_delete_packages (HifTransaction *transaction,
				 HifState *state,
				 GError **error)
{
	GFile *file;
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);
	HifState *state_local;
	HyPackage pkg;
	const gchar *filename;
	const gchar *cachedir;
	guint i;
	guint ret = TRUE;

	/* nothing to delete? */
	if (priv->install->len == 0)
		goto out;

	/* get the cachedir so we only delete packages in the actual
	 * cache, not local-install packages */
	cachedir = hif_context_get_cache_dir (priv->context);
	if (cachedir == NULL) {
		ret = FALSE;
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_FAILED_CONFIG_PARSING,
				     "Failed to get value for CacheDir");
		goto out;
	}

	/* delete each downloaded file */
	state_local = hif_state_get_child (state);
	hif_state_set_number_steps (state_local, priv->install->len);
	for (i = 0; i < priv->install->len; i++) {
		pkg = g_ptr_array_index (priv->install, i);

		/* don't delete files not in the repo */
		filename = hif_package_get_filename (pkg);
		if (g_str_has_prefix (filename, cachedir)) {
			file = g_file_new_for_path (filename);
			ret = g_file_delete (file, NULL, error);
			g_object_unref (file);
			if (!ret)
				goto out;
		}

		/* done */
		ret = hif_state_done (state_local, error);
		if (!ret)
			goto out;
	}
out:
	return ret;
}

/**
 * hif_transaction_convert_to_system_repo:
 **/
static HyPackage
hif_transaction_convert_to_system_repo (HifTransaction *transaction,
					HyPackage pkg,
					HifState *state,
					GError **error)
{
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);
	HyPackageList pkglist = NULL;
	HyPackage pkg_installed = NULL;
	HyQuery query = NULL;
	HySack sack;
	const gchar *cachedir;
	gboolean ret;
	gint rc;

	/* load installed packages */
	cachedir = hif_context_get_cache_dir (priv->context);
	sack = hy_sack_create (cachedir, NULL, NULL, HY_MAKE_CACHE_DIR);
	if (sack == NULL) {
		ret = FALSE;
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "failed to create sack cache");
		goto out;
	}

	/* add installed packages */
	rc = hy_sack_load_system_repo (sack, NULL, HY_BUILD_CACHE);
	ret = hif_rc_to_gerror (rc, error);
	if (!ret) {
		g_prefix_error (error, "Failed to load system repo: ");
		goto out;
	}

	/* find exact package */
	query = hy_query_create (sack);
	hy_query_filter (query, HY_PKG_NAME, HY_EQ, hy_package_get_name (pkg));
	hy_query_filter (query, HY_PKG_EVR, HY_EQ, hy_package_get_evr (pkg));
	hy_query_filter (query, HY_PKG_ARCH, HY_EQ, hy_package_get_arch (pkg));
	hy_query_filter (query, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
	pkglist = hy_query_run (query);
	if (hy_packagelist_count (pkglist) != 1) {
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_PACKAGE_NOT_FOUND,
			     "Failed to find installed version of %s [%i]",
			     hy_package_get_name (pkg),
			     hy_packagelist_count (pkglist));
		goto out;
	}

	/* success */
	pkg_installed = hy_package_link (hy_packagelist_get (pkglist, 0));
out:
	if (query != NULL)
		hy_query_free (query);
	if (pkglist != NULL)
		hy_packagelist_free (pkglist);
	return pkg_installed;
}

/**
 * hif_transaction_write_yumdb_install_item:
 **/
static gboolean
hif_transaction_write_yumdb_install_item (HifTransaction *transaction,
					  HyPackage pkg,
					  HifState *state,
					  GError **error)
{
	HifState *state_local;
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);
	HyPackage pkg_installed = NULL;
	const gchar *reason;
	gboolean ret;
	gchar *tmp;

	/* set steps */
	hif_state_set_number_steps (state, 5);

	/* need to find the HyPackage in the rpmdb, not the remote one that we
	 * just installed */
	state_local = hif_state_get_child (state);
	pkg_installed = hif_transaction_convert_to_system_repo (transaction,
								pkg,
								state_local,
								error);
	if (pkg_installed == NULL) {
		ret = FALSE;
		goto out;
	}

	/* this section done */
	ret = hif_state_done (state, error);
	if (!ret)
		goto out;

	/* set the repo this came from */
	ret = hif_db_set_string (priv->db,
				 pkg_installed,
				 "from_repo",
				 hy_package_get_reponame (pkg),
				 error);
	if (!ret)
		goto out;

	/* this section done */
	ret = hif_state_done (state, error);
	if (!ret)
		goto out;

	/* write euid */
	tmp = g_strdup_printf ("%i", priv->uid);
	ret = hif_db_set_string (priv->db,
				 pkg_installed,
				 "installed_by",
				 tmp,
				 error);
	g_free (tmp);
	if (!ret)
		goto out;

	/* this section done */
	ret = hif_state_done (state, error);
	if (!ret)
		goto out;

	/* set the correct reason */
	if (hif_package_get_user_action (pkg)) {
		reason = "user";
	} else {
		reason = "dep";
	}
	ret = hif_db_set_string (priv->db,
				 pkg_installed,
				 "reason",
				 reason,
				 error);
	if (!ret)
		goto out;

	/* this section done */
	ret = hif_state_done (state, error);
	if (!ret)
		goto out;

	/* set the correct release */
	ret = hif_db_set_string (priv->db,
				 pkg_installed,
				 "releasever",
				 hif_context_get_release_ver (priv->context),
				 error);
	if (!ret)
		goto out;

	/* this section done */
	ret = hif_state_done (state, error);
	if (!ret)
		goto out;
out:
	if (pkg_installed != NULL)
		hy_package_free (pkg_installed);
	return ret;
}

/**
 * _hif_state_get_step_multiple_pair:
 *
 * 3,3	= 50
 * 3,0	= 99 (we can't use 100 as an index value)
 * 0,3	= 1  (we can't use 0 as an index value)
 **/
static guint
_hif_state_get_step_multiple_pair (guint first, guint second)
{
	return 1 + (first * 98.0 / (first + second));
}

/**
 * hif_transaction_write_yumdb:
 **/
static gboolean
hif_transaction_write_yumdb (HifTransaction *transaction,
			     HifState *state,
			     GError **error)
{
	HifState *state_local;
	HifState *state_loop;
	HyPackage pkg;
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);
	gboolean ret;
	guint i;
	guint steps_auto;

	steps_auto = _hif_state_get_step_multiple_pair (priv->remove->len,
							priv->install->len);
	ret = hif_state_set_steps (state,
				   error,
				   steps_auto,		/* remove */
				   100 - steps_auto,	/* install */
				   -1);
	if (!ret)
		goto out;

	/* remove all the old entries */
	state_local = hif_state_get_child (state);
	if (priv->remove->len > 0)
		hif_state_set_number_steps (state_local,
					    priv->remove->len);
	for (i = 0; i < priv->remove->len; i++) {
		pkg = g_ptr_array_index (priv->remove, i);
		ret = hif_transaction_ensure_source (priv->sources, pkg, error);
		if (!ret)
			goto out;
		ret = hif_db_remove_all (priv->db,
					 pkg,
					 error);
		if (!ret)
			goto out;
		ret = hif_state_done (state_local, error);
		if (!ret)
			goto out;
	}

	/* this section done */
	ret = hif_state_done (state, error);
	if (!ret)
		goto out;

	/* add all the new entries */
	if (priv->install->len > 0)
		hif_state_set_number_steps (state_local,
					    priv->install->len);
	for (i = 0; i < priv->install->len; i++) {
		pkg = g_ptr_array_index (priv->install, i);
		state_loop = hif_state_get_child (state_local);
		ret = hif_transaction_write_yumdb_install_item (transaction,
								pkg,
								state_loop,
								error);
		if (!ret)
			goto out;
		ret = hif_state_done (state_local, error);
		if (!ret)
			goto out;
	}

	/* this section done */
	ret = hif_state_done (state, error);
	if (!ret)
		goto out;
out:
	return ret;
}

/**
 * hif_transaction_download:
 * @transaction: a #HifTransaction instance.
 * @goal: A #HyGoal
 * @state: A #HifState
 * @error: A #GError or %NULL
 *
 * Downloads all the packages needed for a transaction.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_transaction_download (HifTransaction *transaction,
			  HifState *state,
			  GError **error)
{
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);
	gboolean ret;

	/* just download the list */
	ret = hif_package_array_download (priv->pkgs_to_download,
					  NULL,
					  state,
					  error);
	if (!ret)
		goto out;
out:
	return ret;
}

/**
 * hif_transaction_depsolve:
 * @transaction: a #HifTransaction instance.
 * @state: A #HifState
 * @error: A #GError or %NULL
 *
 * Depsolves the transaction.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_transaction_depsolve (HifTransaction *transaction,
			  HyGoal goal,
			  HifState *state,
			  GError **error)
{
	GPtrArray *packages = NULL;
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);
	HyPackage pkg;
	gboolean ret;
	gboolean valid;
	guint i;

	/* depsolve */
	ret = hif_goal_depsolve (goal, error);
	if (!ret)
		goto out;

	/* find a list of all the packages we have to download */
	g_ptr_array_set_size (priv->pkgs_to_download, 0);
	packages = hif_goal_get_packages (goal,
					  HIF_PACKAGE_INFO_INSTALL,
					  HIF_PACKAGE_INFO_REINSTALL,
					  HIF_PACKAGE_INFO_DOWNGRADE,
					  HIF_PACKAGE_INFO_UPDATE,
					  -1);
	for (i = 0; i < packages->len; i++) {
		pkg = g_ptr_array_index (packages, i);

		/* get correct package source */
		ret = hif_transaction_ensure_source (priv->sources, pkg, error);
		if (!ret)
			goto out;

		/* this is a local file */
		if (g_strcmp0 (hy_package_get_reponame (pkg),
			       HY_CMDLINE_REPO_NAME) == 0) {
			continue;
		}

		/* check package exists and checksum is okay */
		ret = hif_package_check_filename (pkg, &valid, error);
		if (!ret)
			goto out;

		/* package needs to be downloaded */
		if (!valid) {
			g_ptr_array_add (priv->pkgs_to_download,
					 hy_package_link (pkg));
		}
	}
out:
	if (packages != NULL)
		g_ptr_array_unref (packages);
	return ret;
}

/**
 * hif_transaction_commit:
 * @transaction: a #HifTransaction instance.
 * @goal: A #HyGoal
 * @state: A #HifState
 * @error: A #GError or %NULL
 *
 * Commits a transaction.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_transaction_commit (HifTransaction *transaction,
			HyGoal goal,
			HifState *state,
			GError **error)
{
	const gchar *filename;
	const gchar *tmp;
	gboolean allow_untrusted;
	gboolean is_update;
	gboolean ret = FALSE;
	gint rc;
	gint verbosity;
	gint vs_flags;
	guint i;
	guint j;
	HifState *state_local;
	HyPackageList pkglist;
	HyPackage pkg;
	HyPackage pkg_tmp;
	rpmprobFilterFlags problems_filter = 0;
	HifTransactionPrivate *priv = GET_PRIVATE (transaction);

	/* take lock */
	ret = hif_state_take_lock (state,
				   HIF_LOCK_TYPE_RPMDB,
				   HIF_LOCK_MODE_PROCESS,
				   error);
	if (!ret)
		goto out;

	/* set state */
	ret = hif_state_set_steps (state,
				   error,
				   2, /* install */
				   2, /* remove */
				   10, /* test-commit */
				   83, /* commit */
				   1, /* write yumDB */
				   2, /* delete files */
				   -1);
	if (!ret)
		goto out;

	/* import all GPG keys */
	ret = hif_keyring_add_public_keys (priv->keyring, error);
	if (!ret)
		goto out;

	/* find any packages without valid GPG signatures */
	if ((priv->flags & HIF_TRANSACTION_FLAG_ONLY_TRUSTED) > 0) {
		ret = hif_transaction_check_untrusted (priv->keyring,
						       priv->sources,
						       goal,
						       error);
		if (!ret)
			goto out;
	}

	hif_state_action_start (state, HIF_STATE_ACTION_REQUEST, NULL);

	/* get verbosity from the config file */
	tmp = hif_context_get_rpm_verbosity (priv->context);
	verbosity = hif_rpm_verbosity_string_to_value (tmp);
	rpmSetVerbosity (verbosity);

	/* setup the transaction */
	tmp = hif_context_get_install_root (priv->context);
	rc = rpmtsSetRootDir (priv->ts, tmp);
	if (rc < 0) {
		ret = FALSE;
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_INTERNAL_ERROR,
				     "failed to set root");
		goto out;
	}
	rpmtsSetNotifyCallback (priv->ts,
				hif_transaction_ts_progress_cb,
				transaction);

	/* add things to install */
	state_local = hif_state_get_child (state);
	priv->install = hif_goal_get_packages (goal,
					       HIF_PACKAGE_INFO_INSTALL,
					       HIF_PACKAGE_INFO_REINSTALL,
					       HIF_PACKAGE_INFO_DOWNGRADE,
					       HIF_PACKAGE_INFO_UPDATE,
					       -1);
	if (priv->install->len > 0)
		hif_state_set_number_steps (state_local,
					    priv->install->len);
	for (i = 0; i < priv->install->len; i++) {

		pkg = g_ptr_array_index (priv->install, i);
		ret = hif_transaction_ensure_source (priv->sources,
						 pkg, error);
		if (!ret)
			goto out;

		/* add the install */
		filename = hif_package_get_filename (pkg);
		allow_untrusted = (priv->flags & HIF_TRANSACTION_FLAG_ONLY_TRUSTED) == 0;
		is_update = hif_package_get_action (pkg) == HIF_PACKAGE_INFO_UPDATE;
		ret = hif_rpmts_add_install_filename (priv->ts,
						      filename,
						      allow_untrusted,
						      is_update,
						      error);
		if (!ret)
			goto out;

		/* this section done */
		ret = hif_state_done (state_local, error);
		if (!ret)
			goto out;
	}

	/* this section done */
	ret = hif_state_done (state, error);
	if (!ret)
		goto out;

	/* add things to remove */
	priv->remove = hif_goal_get_packages (goal,
					      HIF_PACKAGE_INFO_REMOVE,
					      -1);
	for (i = 0; i < priv->remove->len; i++) {
		pkg = g_ptr_array_index (priv->remove, i);
		ret = hif_rpmts_add_remove_pkg (priv->ts, pkg, error);
		if (!ret)
			goto out;

		/* pre-get the pkgid, as this isn't possible to get after
		 * the sack is invalidated */
		if (hif_package_get_pkgid (pkg) == NULL) {
			ret = FALSE;
			g_set_error (error,
				     HIF_ERROR,
				     HIF_ERROR_INTERNAL_ERROR,
				     "failed to pre-get pkgid for %s",
				     hif_package_get_id (pkg));
			goto out;
		}

		/* are the things being removed actually being upgraded */
		pkg_tmp = hif_find_pkg_from_name (priv->install,
						  hy_package_get_name (pkg));
		if (pkg_tmp != NULL)
			hif_package_set_action (pkg, HIF_PACKAGE_INFO_CLEANUP);
	}

	/* add anything that gets obsoleted to a helper array which is used to
	 * map removed packages auto-added by rpm to actual HyPackage's */
	priv->remove_helper = g_ptr_array_new ();
	for (i = 0; i < priv->install->len; i++) {
		pkg = g_ptr_array_index (priv->install, i);
		is_update = hif_package_get_action (pkg) == HIF_PACKAGE_INFO_UPDATE;
		if (!is_update)
			continue;
		pkglist = hy_goal_list_obsoleted_by_package (goal, pkg);
		FOR_PACKAGELIST(pkg_tmp, pkglist, j) {
			g_ptr_array_add (priv->remove_helper, pkg);
			hif_package_set_action (pkg, HIF_PACKAGE_INFO_CLEANUP);
		}
		hy_packagelist_free (pkglist);
	}

	/* this section done */
	ret = hif_state_done (state, error);
	if (!ret)
		goto out;

	/* generate ordering for the transaction */
	rpmtsOrder (priv->ts);

	/* run the test transaction */
	if (hif_context_get_check_transaction (priv->context)) {
		g_debug ("running test transaction");
		hif_state_action_start (state,
					HIF_STATE_ACTION_TEST_COMMIT,
					NULL);
		priv->state = hif_state_get_child (state);
		priv->step = HIF_TRANSACTION_STEP_IGNORE;
		/* the output value of rpmtsCheck is not meaningful */
		rpmtsCheck (priv->ts);
		hif_state_action_stop (state);
		ret = hif_rpmts_look_for_problems (priv->ts, error);
		if (!ret)
			goto out;
	}

	/* this section done */
	ret = hif_state_done (state, error);
	if (!ret)
		goto out;

	/* no signature checking, we've handled that already */
	vs_flags = rpmtsSetVSFlags (priv->ts,
				    _RPMVSF_NOSIGNATURES | _RPMVSF_NODIGESTS);
	rpmtsSetVSFlags (priv->ts, vs_flags);

	/* filter diskspace */
	if (!hif_context_get_check_disk_space (priv->context))
		problems_filter += RPMPROB_FILTER_DISKSPACE;

	/* run the transaction */
	priv->state = hif_state_get_child (state);
	priv->step = HIF_TRANSACTION_STEP_STARTED;
	rpmtsSetFlags (priv->ts, RPMTRANS_FLAG_NONE);
	g_debug ("Running actual transaction");
	rc = rpmtsRun (priv->ts, NULL, problems_filter);
	if (rc < 0) {
		ret = FALSE;
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "Error %i running transaction", rc);
		goto out;
	}
	if (rc > 0) {
		ret = hif_rpmts_look_for_problems (priv->ts, error);
		if (!ret)
			goto out;
	}

	/* hmm, nothing was done... */
	if (priv->step != HIF_TRANSACTION_STEP_WRITING) {
		ret = FALSE;
		g_set_error (error,
			     HIF_ERROR,
			     HIF_ERROR_INTERNAL_ERROR,
			     "Transaction did not go to writing phase, "
			     "but returned no error (%i)",
			     priv->step);
		goto out;
	}

	/* this section done */
	ret = hif_state_done (state, error);
	if (!ret)
		goto out;

	/* all sacks are invalid now */
	hif_context_invalidate (priv->context, "transaction performed");

	/* write to the yumDB */
	state_local = hif_state_get_child (state);
	ret = hif_transaction_write_yumdb (transaction,
					   state_local,
					   error);
	if (!ret)
		goto out;

	/* this section done */
	ret = hif_state_done (state, error);
	if (!ret)
		goto out;

	/* remove the files we downloaded */
	if (!hif_context_get_keep_cache (priv->context)) {
		state_local = hif_state_get_child (state);
		ret = hif_transaction_delete_packages (transaction,
						       state_local,
						       error);
		if (!ret)
			goto out;
	}

	/* this section done */
	ret = hif_state_done (state, error);
	if (!ret)
		goto out;
out:
	hif_state_release_locks (state);
	return ret;
}

/**
 * hif_transaction_new:
 * @context: a #HifContext instance.
 *
 * Creates a new #HifTransaction.
 *
 * Returns: (transfer full): a #HifTransaction
 *
 * Since: 0.1.0
 **/
HifTransaction *
hif_transaction_new (HifContext *context)
{
	HifTransaction *transaction;
	HifTransactionPrivate *priv;
	transaction = g_object_new (HIF_TYPE_TRANSACTION, NULL);
	priv = GET_PRIVATE (transaction);
	priv->context = g_object_ref (context);
	priv->db = hif_db_new (context);
	return HIF_TRANSACTION (transaction);
}
