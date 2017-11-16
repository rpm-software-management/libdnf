/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2014-2015 Richard Hughes <richard@hughsie.com>
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
 * SECTION:dnf-transaction
 * @short_description: High level interface to librpm.
 * @include: libdnf.h
 * @stability: Stable
 *
 * This object represents an RPM transaction.
 */


#include <rpm/rpmlib.h>
#include <rpm/rpmts.h>
#include <rpm/rpmlog.h>

#include "dnf-context.hpp"
#include "dnf-db.h"
#include "dnf-goal.h"
#include "dnf-keyring.h"
#include "dnf-package.h"
#include "dnf-rpmts.h"
#include "dnf-transaction.h"
#include "dnf-utils.h"
#include "hy-query.h"
#include "dnf-sack.h"
#include "hy-util-private.hpp"

#if WITH_SWDB

#include "dnf-swdb.h"
#include "dnf-swdb-obj.h"
#include "dnf-swdb-trans.h"

#endif

#include "utils/bgettext/bgettext-lib.h"

typedef enum {
    DNF_TRANSACTION_STEP_STARTED,
    DNF_TRANSACTION_STEP_PREPARING,
    DNF_TRANSACTION_STEP_WRITING,
    DNF_TRANSACTION_STEP_IGNORE
} DnfTransactionStep;

typedef struct
{
    DnfDb              *db;
    rpmKeyring          keyring;
    rpmts               ts;
    DnfContext         *context;    /* weak reference */
    GPtrArray          *repos;
    guint               uid;

    /* previously in the helper */
    DnfState            *state;
    DnfState            *child;
    FD_t                 fd;
    DnfTransactionStep   step;
    GTimer              *timer;
    guint                last_progress;
    GPtrArray           *remove;
    GPtrArray           *remove_helper;
    GPtrArray           *install;
    GPtrArray           *pkgs_to_download;
    GHashTable          *erased_by_package_hash;
    guint64             flags;
} DnfTransactionPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(DnfTransaction, dnf_transaction, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (static_cast<DnfTransactionPrivate *>(dnf_transaction_get_instance_private (o)))

typedef struct
{
    void *swdb;
    gint tid;
} SwdbHandle;

/**
 * dnf_transaction_finalize:
 **/
static void
dnf_transaction_finalize(GObject *object)
{
    DnfTransaction *transaction = DNF_TRANSACTION(object);
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);

    g_ptr_array_unref(priv->pkgs_to_download);
    g_timer_destroy(priv->timer);
    rpmKeyringFree(priv->keyring);
    rpmtsFree(priv->ts);

    if (priv->db != NULL)
        g_object_unref(priv->db);
    if (priv->repos != NULL)
        g_ptr_array_unref(priv->repos);
    if (priv->install != NULL)
        g_ptr_array_unref(priv->install);
    if (priv->remove != NULL)
        g_ptr_array_unref(priv->remove);
    if (priv->remove_helper != NULL)
        g_ptr_array_unref(priv->remove_helper);
    if (priv->erased_by_package_hash != NULL)
        g_hash_table_unref(priv->erased_by_package_hash);
    if (priv->context != NULL)
        g_object_remove_weak_pointer(G_OBJECT(priv->context),
                                     (void **) &priv->context);

    G_OBJECT_CLASS(dnf_transaction_parent_class)->finalize(object);
}

/**
 * dnf_transaction_init:
 **/
static void
dnf_transaction_init(DnfTransaction *transaction)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    priv->timer = g_timer_new();
    priv->pkgs_to_download = g_ptr_array_new_with_free_func((GDestroyNotify) g_object_unref);
}

/**
 * dnf_transaction_class_init:
 **/
static void
dnf_transaction_class_init(DnfTransactionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_transaction_finalize;
}

/**
 * dnf_transaction_get_flags:
 * @transaction: a #DnfTransaction instance.
 *
 * Gets the transaction flags.
 *
 * Returns: the transaction flags used for this transaction
 *
 * Since: 0.1.0
 **/
guint64
dnf_transaction_get_flags(DnfTransaction *transaction)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    return priv->flags;
}

/**
 * dnf_transaction_get_remote_pkgs:
 * @transaction: a #DnfTransaction instance.
 *
 * Gets the packages that will be downloaded in dnf_transaction_download().
 *
 * The dnf_transaction_depsolve() function must have been called before this
 * function will return sensible results.
 *
 * Returns:(transfer none): the list of packages
 *
 * Since: 0.1.0
 **/
GPtrArray *
dnf_transaction_get_remote_pkgs(DnfTransaction *transaction)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    return priv->pkgs_to_download;
}

/**
 * dnf_transaction_get_db:
 * @transaction: a #DnfTransaction instance.
 *
 * Gets the database instance used for this transaction.
 *
 * Returns:(transfer none): the #DnfDb
 *
 * Since: 0.1.0
 **/
DnfDb *
dnf_transaction_get_db(DnfTransaction *transaction)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    return priv->db;
}

/**
 * dnf_transaction_set_repos:
 * @transaction: a #DnfTransaction instance.
 * @repos: the repos to use with the transaction
 *
 * Sets the list of repos.
 *
 * Since: 0.1.0
 **/
void
dnf_transaction_set_repos(DnfTransaction *transaction, GPtrArray *repos)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    if (priv->repos != NULL)
        g_ptr_array_unref(priv->repos);
    priv->repos = g_ptr_array_ref(repos);
}

/**
 * dnf_transaction_set_uid:
 * @transaction: a #DnfTransaction instance.
 * @uid: the uid
 *
 * Sets the user ID for the person who started this transaction.
 *
 * Since: 0.1.0
 **/
void
dnf_transaction_set_uid(DnfTransaction *transaction, guint uid)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    priv->uid = uid;
}

/**
 * dnf_transaction_set_flags:
 * @transaction: a #DnfTransaction instance.
 * @flags: the flags, e.g. %DNF_TRANSACTION_FLAG_ONLY_TRUSTED
 *
 * Sets the flags used for this transaction.
 *
 * Since: 0.1.0
 **/
void
dnf_transaction_set_flags(DnfTransaction *transaction, guint64 flags)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    priv->flags = flags;
}

/**
 * dnf_transaction_ensure_repo:
 * @transaction: a #DnfTransaction instance.
 * @pkg: A #DnfPackage
 * @error: A #GError or %NULL
 *
 * Ensures the #DnfRepo is set on the #DnfPackage *if not already set.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 */
gboolean
dnf_transaction_ensure_repo(DnfTransaction *transaction,
                              DnfPackage *pkg,
                              GError **error)
{
    DnfRepo *repo;
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    guint i;

    /* not set yet */
    if (priv->repos == NULL) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    _("Sources not set when trying to ensure package %s"),
                    dnf_package_get_name(pkg));
        return FALSE;
    }

    /* this is a local file */
    if (g_strcmp0(dnf_package_get_reponame(pkg),
                  HY_CMDLINE_REPO_NAME) == 0) {
        dnf_package_set_filename(pkg, dnf_package_get_location(pkg));
        return TRUE;
    }

    /* get repo */
    if (dnf_package_installed(pkg))
        return TRUE;
    for (i = 0; i < priv->repos->len; i++) {
        repo = g_ptr_array_index(priv->repos, i);
        if (g_strcmp0(dnf_package_get_reponame(pkg),
                      dnf_repo_get_id(repo)) == 0) {
            dnf_package_set_repo(pkg, repo);
            return TRUE;
        }
    }

    /* not found */
    g_set_error(error,
                DNF_ERROR,
                DNF_ERROR_INTERNAL_ERROR,
                _("Failed to ensure %1$s as repo %2$s not "
                  "found(%3$i repos loaded)"),
                dnf_package_get_name(pkg),
                dnf_package_get_reponame(pkg),
                priv->repos->len);
    return FALSE;
}

/**
 * dnf_transaction_ensure_repo_list:
 * @transaction: a #DnfTransaction instance.
 * @pkglist: A #GPtrArray *
 * @error: A #GError or %NULL
 *
 * Ensures the #DnfRepo is set on the #GPtrArray *if not already set.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 */
gboolean
dnf_transaction_ensure_repo_list(DnfTransaction *transaction,
                                   GPtrArray *pkglist,
                                   GError **error)
{
    guint i;
    DnfPackage *pkg;

    for (i = 0; i < pkglist->len; i++) {
        pkg = g_ptr_array_index (pkglist, i);
        if (!dnf_transaction_ensure_repo(transaction, pkg, error))
            return FALSE;
    }
    return TRUE;
}

gboolean
dnf_transaction_gpgcheck_package(DnfTransaction *transaction,
                                 DnfPackage     *pkg,
                                 GError        **error)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    GError *error_local = NULL;
    DnfRepo *repo;
    const gchar *fn;

    /* ensure the filename is set */
    if (!dnf_transaction_ensure_repo(transaction, pkg, error)) {
        g_prefix_error(error, _("Failed to check untrusted: "));
        return FALSE;
    }

    /* find the location of the local file */
    fn = dnf_package_get_filename(pkg);
    if (fn == NULL) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_FILE_NOT_FOUND,
                    _("Downloaded file for %s not found"),
                    dnf_package_get_name(pkg));
        return FALSE;
    }

    /* check file */
    if (!dnf_keyring_check_untrusted_file(priv->keyring, fn, &error_local)) {

        /* probably an i/o error */
        if (!g_error_matches(error_local,
                             DNF_ERROR,
                             DNF_ERROR_GPG_SIGNATURE_INVALID)) {
            g_propagate_error(error, error_local);
            return FALSE;
        }

        /* if the repo is signed this is ALWAYS an error */
        repo = dnf_package_get_repo(pkg);
        if (repo != NULL && dnf_repo_get_gpgcheck(repo)) {
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_FILE_INVALID,
                        _("package %1$s cannot be verified "
                          "and repo %2$s is GPG enabled: %3$s"),
                        dnf_package_get_nevra(pkg),
                        dnf_repo_get_id(repo),
                        error_local->message);
            g_error_free(error_local);
            return FALSE;
        }

        /* we can only install signed packages in this mode */
        if ((priv->flags & DNF_TRANSACTION_FLAG_ONLY_TRUSTED) > 0) {
            g_propagate_error(error, error_local);
            return FALSE;
        } else {
            g_clear_error(&error_local);
        }
    }

    return TRUE;
}

/**
 * dnf_transaction_check_untrusted:
 * @transaction: Transaction
 * @goal: Target goal
 * @error: Error
 *
 * Verify GPG signatures for all pending packages to be changed as part
 * of @goal.
 */
gboolean
dnf_transaction_check_untrusted(DnfTransaction *transaction,
                                HyGoal goal,
                                GError **error)
{
    guint i;
    g_autoptr(GPtrArray) install = NULL;

    /* find a list of all the packages we might have to download */
    install = dnf_goal_get_packages(goal,
                                    DNF_PACKAGE_INFO_INSTALL,
                                    DNF_PACKAGE_INFO_REINSTALL,
                                    DNF_PACKAGE_INFO_DOWNGRADE,
                                    DNF_PACKAGE_INFO_UPDATE,
                                    -1);
    if (install->len == 0)
        return TRUE;

    /* find any packages in untrusted repos */
    for (i = 0; i < install->len; i++) {
        DnfPackage *pkg = g_ptr_array_index(install, i);

        if (!dnf_transaction_gpgcheck_package (transaction, pkg, error))
            return FALSE;
    }
    return TRUE;
}

/**
 * dnf_find_pkg_from_header:
 **/
static DnfPackage *
dnf_find_pkg_from_header(GPtrArray *array, Header hdr)
{
    const gchar *arch;
    const gchar *name;
    const gchar *release;
    const gchar *version;
    guint epoch;
    guint i;
    DnfPackage *pkg;

    /* get details */
    name = headerGetString(hdr, RPMTAG_NAME);
    epoch = headerGetNumber(hdr, RPMTAG_EPOCH);
    version = headerGetString(hdr, RPMTAG_VERSION);
    release = headerGetString(hdr, RPMTAG_RELEASE);
    arch = headerGetString(hdr, RPMTAG_ARCH);

    /* find in array */
    for (i = 0; i < array->len; i++) {
        pkg = g_ptr_array_index(array, i);
        if (g_strcmp0(name, dnf_package_get_name(pkg)) != 0)
            continue;
        if (g_strcmp0(version, dnf_package_get_version(pkg)) != 0)
            continue;
        if (g_strcmp0(release, dnf_package_get_release(pkg)) != 0)
            continue;
        if (g_strcmp0(arch, dnf_package_get_arch(pkg)) != 0)
            continue;
        if (epoch != dnf_package_get_epoch(pkg))
            continue;
        return pkg;
    }
    return NULL;
}

/**
 * dnf_find_pkg_from_filename_suffix:
 **/
static DnfPackage *
dnf_find_pkg_from_filename_suffix(GPtrArray *array, const gchar *filename_suffix)
{
    const gchar *filename;
    guint i;
    DnfPackage *pkg;

    /* find in array */
    for (i = 0; i < array->len; i++) {
        pkg = g_ptr_array_index(array, i);
        filename = dnf_package_get_filename(pkg);
        if (filename == NULL)
            continue;
        if (g_str_has_suffix(filename, filename_suffix))
            return pkg;
    }
    return NULL;
}

/**
 * dnf_find_pkg_from_name:
 **/
static DnfPackage *
dnf_find_pkg_from_name(GPtrArray *array, const gchar *pkgname)
{
    guint i;
    DnfPackage *pkg;

    /* find in array */
    for (i = 0; i < array->len; i++) {
        pkg = g_ptr_array_index(array, i);
        if (g_strcmp0(dnf_package_get_name(pkg), pkgname) == 0)
            return pkg;
    }
    return NULL;
}

/**
 * dnf_transaction_ts_progress_cb:
 **/
static void *
dnf_transaction_ts_progress_cb(const void *arg,
                               const rpmCallbackType what,
                               const rpm_loff_t amount,
                               const rpm_loff_t total,
                               fnpyKey key, void *data)
{
    DnfTransaction *transaction = DNF_TRANSACTION(data);
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    const char *filename =(const char *) key;
    const gchar *name = NULL;
    gboolean ret;
    guint percentage;
    guint speed;
    Header hdr =(Header) arg;
    DnfPackage *pkg;
    DnfStateAction action;
    void *rc = NULL;
    g_autoptr(GError) error_local = NULL;

    if (hdr != NULL)
        name = headerGetString(hdr, RPMTAG_NAME);
    g_debug("phase: %u(%i/%i, %s/%s)",
            (uint)what,
           (gint32) amount,
           (gint32) total,
           (const gchar *) key,
            name);

    switch(what) {
    case RPMCALLBACK_INST_OPEN_FILE:

        /* valid? */
        if (filename == NULL || filename[0] == '\0')
            return NULL;

        /* open the file and return file descriptor */
        priv->fd = Fopen(filename, "r.ufdio");
        return(void *) priv->fd;
        break;

    case RPMCALLBACK_INST_CLOSE_FILE:

        /* just close the file */
        if (priv->fd != NULL) {
            Fclose(priv->fd);
            priv->fd = NULL;
        }
        break;

    case RPMCALLBACK_INST_START:

        /* find pkg */
        pkg = dnf_find_pkg_from_filename_suffix(priv->install,
                                                filename);
        if (pkg == NULL)
            g_assert_not_reached();

        /* map to correct action code */
        action = dnf_package_get_action(pkg);
        if (action == DNF_STATE_ACTION_UNKNOWN)
            action = DNF_STATE_ACTION_INSTALL;

        /* set the pkgid if not already set */
        if (dnf_package_get_pkgid(pkg) == NULL) {
            const gchar *pkgid;
            pkgid = headerGetString(hdr, RPMTAG_SHA1HEADER);
            if (pkgid != NULL) {
                g_debug("setting %s pkgid %s", name, pkgid);
                dnf_package_set_pkgid(pkg, pkgid);
            }
        }

        /* install start */
        priv->step = DNF_TRANSACTION_STEP_WRITING;
        priv->child = dnf_state_get_child(priv->state);
        dnf_state_action_start(priv->child,
                               action,
                               dnf_package_get_package_id(pkg));
        g_debug("install start: %s size=%i", filename,(gint32) total);
        break;

    case RPMCALLBACK_UNINST_START:

        /* find pkg */
        pkg = dnf_find_pkg_from_header(priv->remove, hdr);
        if (pkg == NULL && filename != NULL) {
            pkg = dnf_find_pkg_from_filename_suffix(priv->remove,
                                                    filename);
        }
        if (pkg == NULL && name != NULL)
            pkg = dnf_find_pkg_from_name(priv->remove, name);
        if (pkg == NULL && name != NULL)
            pkg = dnf_find_pkg_from_name(priv->remove_helper, name);
        if (pkg == NULL) {
            g_warning("cannot find %s in uninst-start", name);
            priv->step = DNF_TRANSACTION_STEP_WRITING;
            break;
        }

        /* map to correct action code */
        action = dnf_package_get_action(pkg);
        if (action == DNF_STATE_ACTION_UNKNOWN)
            action = DNF_STATE_ACTION_REMOVE;

        /* remove start */
        priv->step = DNF_TRANSACTION_STEP_WRITING;
        priv->child = dnf_state_get_child(priv->state);
        dnf_state_action_start(priv->child,
                               action,
                               dnf_package_get_package_id(pkg));
        g_debug("remove start: %s size=%i", filename,(gint32) total);
        break;

    case RPMCALLBACK_TRANS_PROGRESS:
    case RPMCALLBACK_INST_PROGRESS:

        /* we're preparing the transaction */
        if (priv->step == DNF_TRANSACTION_STEP_PREPARING ||
            priv->step == DNF_TRANSACTION_STEP_IGNORE) {
            g_debug("ignoring preparing %i / %i",
                    (gint32) amount,(gint32) total);
            break;
        }

        /* work out speed */
        speed = (amount - priv->last_progress) /
                    g_timer_elapsed(priv->timer, NULL);
        dnf_state_set_speed(priv->state, speed);
        priv->last_progress = amount;
        g_timer_reset(priv->timer);

        /* progress */
        percentage =(100.0f /(gfloat) total) *(gfloat) amount;
        if (priv->child != NULL)
            dnf_state_set_percentage(priv->child, percentage);

        /* update UI */
        pkg = dnf_find_pkg_from_header(priv->install, hdr);
        if (pkg == NULL) {
            pkg = dnf_find_pkg_from_filename_suffix(priv->install,
                                                    filename);
        }
        if (pkg == NULL) {
            g_debug("cannot find %s(%s)", filename, name);
            break;
        }

        dnf_state_set_package_progress(priv->state,
                                       dnf_package_get_package_id(pkg),
                                       DNF_STATE_ACTION_INSTALL,
                                       percentage);
        break;

    case RPMCALLBACK_UNINST_PROGRESS:

        /* we're preparing the transaction */
        if (priv->step == DNF_TRANSACTION_STEP_PREPARING ||
            priv->step == DNF_TRANSACTION_STEP_IGNORE) {
            g_debug("ignoring preparing %i / %i",
                    (gint32) amount,(gint32) total);
            break;
        }

        /* progress */
        percentage =(100.0f /(gfloat) total) *(gfloat) amount;
        if (priv->child != NULL)
            dnf_state_set_percentage(priv->child, percentage);

        /* update UI */
        pkg = dnf_find_pkg_from_header(priv->remove, hdr);
        if (pkg == NULL && filename != NULL) {
            pkg = dnf_find_pkg_from_filename_suffix(priv->remove,
                                                    filename);
        }
        if (pkg == NULL && name != NULL)
            pkg = dnf_find_pkg_from_name(priv->remove, name);
        if (pkg == NULL && name != NULL)
            pkg = dnf_find_pkg_from_name(priv->remove_helper, name);
        if (pkg == NULL) {
            g_warning("cannot find %s in uninst-progress", name);
            break;
        }

        /* map to correct action code */
        action = dnf_package_get_action(pkg);
        if (action == DNF_STATE_ACTION_UNKNOWN)
            action = DNF_STATE_ACTION_REMOVE;

        dnf_state_set_package_progress(priv->state,
                                       dnf_package_get_package_id(pkg),
                                       action,
                                       percentage);
        break;

    case RPMCALLBACK_TRANS_START:

        /* we setup the state */
        g_debug("preparing transaction with %i items",(gint32) total);
        if (priv->step == DNF_TRANSACTION_STEP_IGNORE)
            break;

        dnf_state_set_number_steps(priv->state, total);
        priv->step = DNF_TRANSACTION_STEP_PREPARING;
        break;

    case RPMCALLBACK_TRANS_STOP:

        /* don't do anything */
        break;

    case RPMCALLBACK_INST_STOP:
    case RPMCALLBACK_UNINST_STOP:

        /* phase complete */
        ret = dnf_state_done(priv->state, &error_local);
        if (!ret) {
            g_warning("state increment failed: %s",
                      error_local->message);
        }
        break;

    default:
        break;
    }
    return rc;
}

/**
 * dnf_rpm_verbosity_string_to_value:
 **/
static gint
dnf_rpm_verbosity_string_to_value(const gchar *value)
{
    if (g_strcmp0(value, "critical") == 0)
        return RPMLOG_CRIT;
    if (g_strcmp0(value, "emergency") == 0)
        return RPMLOG_EMERG;
    if (g_strcmp0(value, "error") == 0)
        return RPMLOG_ERR;
    if (g_strcmp0(value, "warn") == 0)
        return RPMLOG_WARNING;
    if (g_strcmp0(value, "debug") == 0)
        return RPMLOG_DEBUG;
    if (g_strcmp0(value, "info") == 0)
        return RPMLOG_INFO;
    return RPMLOG_EMERG;
}

/**
 * dnf_transaction_delete_packages:
 **/
static gboolean
dnf_transaction_delete_packages(DnfTransaction *transaction,
                 DnfState *state,
                 GError **error)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    DnfState *state_local;
    DnfPackage *pkg;
    const gchar *filename;
    const gchar *cachedir;
    guint i;

    /* nothing to delete? */
    if (priv->install->len == 0)
        return TRUE;

    /* get the cachedir so we only delete packages in the actual
     * cache, not local-install packages */
    cachedir = dnf_context_get_cache_dir(priv->context);
    if (cachedir == NULL) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_FAILED_CONFIG_PARSING,
                            _("Failed to get value for CacheDir"));
        return FALSE;
    }

    /* delete each downloaded file */
    state_local = dnf_state_get_child(state);
    dnf_state_set_number_steps(state_local, priv->install->len);
    for (i = 0; i < priv->install->len; i++) {
        pkg = g_ptr_array_index(priv->install, i);

        /* don't delete files not in the repo */
        filename = dnf_package_get_filename(pkg);
        if (g_str_has_prefix(filename, cachedir)) {
            g_autoptr(GFile) file = NULL;
            file = g_file_new_for_path(filename);
            if (!g_file_delete(file, NULL, error))
                return FALSE;
        }

        /* done */
        if (!dnf_state_done(state_local, error))
            return FALSE;
    }
    return TRUE;
}

static gchar *
dnf_transaction_get_propagated_reason(DnfTransaction *transaction,
                                      HyGoal goal,
                                      DnfPackage *pkg)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);

    /* kernel is always marked as "user" */
    if (dnf_package_is_installonly(pkg))
        return g_strdup("user");

    /* for updates, propagate updated package's reason from existing yumdb entry */
    if (dnf_package_get_action(pkg) == DNF_STATE_ACTION_DOWNGRADE ||
        dnf_package_get_action(pkg) == DNF_STATE_ACTION_REINSTALL ||
        dnf_package_get_action(pkg) == DNF_STATE_ACTION_UPDATE) {
        DnfPackage *erased_package;

        erased_package = g_hash_table_lookup(priv->erased_by_package_hash, dnf_package_get_package_id(pkg));
        if (erased_package != NULL) {
            gchar *reason;

            reason = dnf_db_get_string(priv->db, erased_package, "reason", NULL);
            if (reason != NULL) {
                g_debug("propagating yumdb reason %s from %s to %s",
                        reason,
                        dnf_package_get_package_id(erased_package),
                        dnf_package_get_package_id(pkg));
                return reason;
            }
        }
        return g_strdup("dep");
    }

    /* for non-upgrades, packages explicitly passed to hawkey for
     * installation get "user" and dependencies it adds on its own get
     * "dep" */
    if (hy_goal_get_reason(goal, pkg) == HY_REASON_USER)
        return g_strdup("user");
    else
        return g_strdup("dep");
}

#if WITH_SWDB

static gint64
_dnf_swdb_get_time()
{
    g_autoptr (GDateTime) t_struct = g_date_time_new_now_utc ();
    return g_date_time_to_unix (t_struct);
}

/**
 * _get_swdb_pkg:
 * @swdb: Software database object
 * @pkg: Hawkey package object
 *
 * Transform Hawkey package object to swdb object
 * Returns: SWDB Package ID
 **/
static gint
_get_swdb_pkg(DnfSwdb *swdb, DnfPackage *pkg)
{
    gint pid = 0;
    const gchar *nevra = dnf_package_get_nevra(pkg);
    DnfSwdbPkg *spkg = dnf_swdb_package(swdb, nevra);

    if (spkg) {
        pid = spkg->pid;
        g_object_unref(spkg);
        return pid;
    }

    int type = 0;
    const gchar *name = dnf_package_get_name(pkg);
    guint64 epoch = dnf_package_get_epoch(pkg);

    const gchar *version = dnf_package_get_version(pkg);
    const gchar *release = dnf_package_get_release(pkg);
    const gchar *arch = dnf_package_get_arch(pkg);

    const guchar *bin_chksum = dnf_package_get_chksum(pkg, &type);
    g_autofree gchar *chksum = hy_chksum_str(bin_chksum, type);
    const gchar *chksum_type = hy_chksum_name(type);

    spkg = dnf_swdb_pkg_new(name,
                            epoch,
                            version,
                            release,
                            arch,
                            chksum,
                            chksum_type,
                            DNF_SWDB_ITEM_RPM);

    pid = dnf_swdb_add_package(swdb, spkg);

    g_object_unref(spkg);
    return pid;
}


/**
 * _log_swdb_transaction:
 *
 * Create SWDB transaction for package @pkg
 */
static void
_log_swdb_transaction(SwdbHandle *handle,
                      DnfPackage *pkg,
                      const gchar *user,
                      const gchar *reason,
                      const gchar *state)
{
    // save these attributes to swdb if present
    DnfSwdb *swdb = (DnfSwdb *) handle->swdb;

    if (swdb == NULL) {
        g_debug("SWDB is not present, skipping write");
        return;
    }

    // Insert package to SWDB
    gint pid = _get_swdb_pkg(swdb, pkg);

    if (pid == 0) {
        g_warning("Package transformation to SWDB failed");
        return;
    }

    g_autoptr (DnfSwdbPkgData) pkg_data = dnf_swdb_pkgdata_new(
        NULL, //from_repo_revision
        0, //from_repo_timestamp
        user,
        NULL, //changed by
        dnf_package_get_reponame(pkg));

    // insert transaction data
    // XXX obsoleting packages in PackageKit?
    dnf_swdb_trans_data_beg(swdb, handle->tid, pid, dnf_convert_reason_to_id (reason), state, 0);

    // insert package data
    dnf_swdb_update_package_data(swdb, pid, handle->tid, pkg_data);

    // mark transaction as successfull
    dnf_swdb_trans_data_pid_end(swdb, pid, handle->tid, state);
}

/**
 * _log_swdb_removal:
 *
 * Create SWDB transaction for @pkg removal
 */
static void
_log_swdb_removal(SwdbHandle *handle,
                  DnfTransaction *trans,
                  HyGoal goal,
                  DnfPackage *pkg)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(trans);

    //get user
    g_autofree gchar *user = g_strdup_printf("%i", priv->uid);

    //get removal reason
    const gchar *reason = dnf_transaction_get_propagated_reason(trans, goal, pkg);

    //log transaction
    //FIXME proper state resultion
    _log_swdb_transaction(handle, pkg, user, reason, "Erase");
}

#endif

/**
 * dnf_transaction_write_yumdb_install_item:
 *
 * We've used dnf_package_set_pkgid() when running the transaction so we can
 * avoid the lookup in the rpmdb.
 **/
static gboolean
dnf_transaction_write_yumdb_install_item(DnfTransaction *transaction,
                                         HyGoal goal,
                                         DnfPackage *pkg,
                                         SwdbHandle *handle,
                                         DnfState *state,
                                         GError **error)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    const gchar *tmp;
    const gchar *release;
    g_autofree gchar *euid = NULL;
    g_autofree gchar *reason = NULL;

    /* should be set by dnf_transaction_ts_progress_cb() */
    if (dnf_package_get_pkgid(pkg) == NULL) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    _("no yumdb entry for %s as no pkgid"),
                    dnf_package_get_package_id(pkg));
        return FALSE;
    }

    /* uncancellable */
    dnf_state_set_allow_cancel(state, FALSE);

    /* set the repo this came from */
    tmp = dnf_package_get_reponame(pkg);
    if (!dnf_db_set_string(priv->db, pkg, "from_repo", tmp, error))
        return FALSE;

    /* write euid */
    euid = g_strdup_printf("%i", priv->uid);
    if (!dnf_db_set_string(priv->db, pkg, "installed_by", euid, error))
        return FALSE;

    /* set the correct reason */
    reason = dnf_transaction_get_propagated_reason(transaction, goal, pkg);
    if (!dnf_db_set_string(priv->db, pkg, "reason", reason, error))
        return FALSE;

    /* set the correct release */
    release = dnf_context_get_release_ver(priv->context);
    if (!dnf_db_set_string(priv->db, pkg, "releasever", release, error))
        return FALSE;

#if WITH_SWDB
    //FIXME proper state resultion
    _log_swdb_transaction(handle, pkg, euid, reason, "Install");
#endif

    return TRUE;
}

/**
 * _dnf_state_get_step_multiple_pair:
 *
 * 3,3    = 50
 * 3,0    = 99(we can't use 100 as an index value)
 * 0,3    = 1 (we can't use 0 as an index value)
 **/
static guint
_dnf_state_get_step_multiple_pair(guint first, guint second)
{
    return 1 +(first * 98.0 /(first + second));
}

/**
 * dnf_transaction_write_yumdb:
 **/
static gboolean
dnf_transaction_write_yumdb(DnfTransaction *transaction,
                 HyGoal goal,
                 DnfState *state,
                 GError **error)
{
    DnfState *state_local;
    DnfState *state_loop;
    DnfPackage *pkg;
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    gboolean ret;
    guint i;
    guint steps_auto;

    steps_auto = _dnf_state_get_step_multiple_pair(priv->install->len,
                                                   priv->remove->len + priv->remove_helper->len);
    ret = dnf_state_set_steps(state,
                              error,
                              steps_auto,       /* install */
                              100 - steps_auto, /* remove */
                              -1);
    if (!ret)
        return FALSE;


    /* prepare SWDB handle */
    SwdbHandle handle;
    handle.swdb = NULL;
    handle.tid = 0;

#if WITH_SWDB

    //get release
    const gchar *release = dnf_context_get_release_ver(priv->context);

    //prepare swdb object
    g_autoptr (DnfSwdb) swdb = dnf_swdb_new(DNF_SWDB_DEFAULT_PATH, release);

    /*
     * Skip writes to swdb when database is not present in the system.
     * Once SWDB compatible DNF will be installed it will migrate yumdb
     * into swdb including writes generated by PackageKit
     */
    gboolean swdb_exists = dnf_swdb_exist (swdb);

    if (swdb_exists) {

        //initialize handle
        handle.swdb = (void *) swdb;

        //get user id
        g_autofree gchar *uid = g_strdup_printf("%i", priv->uid);

        //get time
        gint64 timestamp = _dnf_swdb_get_time();

        //initialize transaction
        handle.tid = dnf_swdb_trans_beg(swdb,
                                        timestamp,
                                        NULL, //beg rpmdb version
                                        NULL, //cmdline
                                        uid, //login ID
                                        release);
    }

#endif

    /* add all the new entries */
    state_local = dnf_state_get_child(state);
    if (priv->install->len > 0)
        dnf_state_set_number_steps(state_local,
                        priv->install->len);
    for (i = 0; i < priv->install->len; i++) {
        pkg = g_ptr_array_index(priv->install, i);
        state_loop = dnf_state_get_child(state_local);
        ret = dnf_transaction_write_yumdb_install_item(transaction,
                                                       goal,
                                                       pkg,
                                                       &handle,
                                                       state_loop,
                                                       error);
        if (!ret)
            return FALSE;
        if (!dnf_state_done(state_local, error))
            return FALSE;
    }

    /* this section done */
    if (!dnf_state_done(state, error))
        return FALSE;

    /* remove all the old entries */
    if ((priv->remove->len + priv->remove_helper->len) > 0)
        dnf_state_set_number_steps(state_local, priv->remove->len + priv->remove_helper->len);
    for (i = 0; i < priv->remove->len; i++) {
        pkg = g_ptr_array_index(priv->remove, i);
#if WITH_SWDB
        _log_swdb_removal(&handle, transaction, goal, pkg);
#endif
        if (!dnf_transaction_ensure_repo(transaction, pkg, error))
            return FALSE;
        if (!dnf_db_remove_all(priv->db, pkg, error))
            return FALSE;
        if (!dnf_state_done(state_local, error))
            return FALSE;
    }
    for (i = 0; i < priv->remove_helper->len; i++) {
        pkg = g_ptr_array_index(priv->remove_helper, i);
#if WITH_SWDB
        _log_swdb_removal(&handle, transaction, goal, pkg);
#endif
        if (!dnf_transaction_ensure_repo(transaction, pkg, error))
            return FALSE;
        if (!dnf_db_remove_all(priv->db, pkg, error))
            return FALSE;
        if (!dnf_state_done(state_local, error))
            return FALSE;
    }

#if WITH_SWDB
    if (swdb_exists) {
        //get end time
        gint64 timestamp = _dnf_swdb_get_time();

        //transaction performed with
        dnf_swdb_trans_with_libdnf(swdb, handle.tid);

        //finalize transaction
        dnf_swdb_trans_end(
            swdb,
            handle.tid,
            timestamp,
            NULL, //end rpmdb version
            0); //return code
    }
#endif

    /* this section done */
    return dnf_state_done(state, error);
}

static gboolean
dnf_transaction_check_free_space(DnfTransaction *transaction,
                                 GError **error)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    const gchar *cachedir;
    guint64 download_size;
    guint64 free_space;
    g_autoptr(GFile) file = NULL;
    g_autoptr(GFileInfo) filesystem_info = NULL;

    download_size = dnf_package_array_get_download_size(priv->pkgs_to_download);

    cachedir = dnf_context_get_cache_dir(priv->context);
    if (cachedir == NULL) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_FAILED_CONFIG_PARSING,
                            _("Failed to get value for CacheDir"));
        return FALSE;
    }

    file = g_file_new_for_path(cachedir);
    filesystem_info = g_file_query_filesystem_info(file, G_FILE_ATTRIBUTE_FILESYSTEM_FREE, NULL, error);
    if (filesystem_info == NULL) {
        g_prefix_error(error, _("Failed to get filesystem free size for %s: "), cachedir);
        return FALSE;
    }

    if (!g_file_info_has_attribute(filesystem_info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE)) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_FAILED,
                    _("Failed to get filesystem free size for %s"),
                    cachedir);
        return FALSE;
    }

    free_space = g_file_info_get_attribute_uint64(filesystem_info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
    if (free_space < download_size) {
        g_autofree gchar *formatted_download_size = NULL;
        g_autofree gchar *formatted_free_size = NULL;

        formatted_download_size = g_format_size(download_size);
        formatted_free_size = g_format_size(free_space);
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_NO_SPACE,
                    _("Not enough free space in %1$s: needed %2$s, available %3$s"),
                    cachedir,
                    formatted_download_size,
                    formatted_free_size);
        return FALSE;
    }

    return TRUE;
}

/**
 * dnf_transaction_download:
 * @transaction: a #DnfTransaction instance.
 * @state: A #DnfState
 * @error: A #GError or %NULL
 *
 * Downloads all the packages needed for a transaction.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_transaction_download(DnfTransaction *transaction,
                         DnfState *state,
                         GError **error)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);

    /* check that we have enough free space */
    if (!dnf_transaction_check_free_space(transaction, error))
        return FALSE;

    /* just download the list */
    return dnf_package_array_download(priv->pkgs_to_download,
                                      NULL,
                                      state,
                                      error);
}

/**
 * dnf_transaction_depsolve:
 * @transaction: a #DnfTransaction instance.
 * @goal: A #HyGoal
 * @state: A #DnfState
 * @error: A #GError or %NULL
 *
 * Depsolves the transaction.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_transaction_depsolve(DnfTransaction *transaction,
                         HyGoal goal,
                         DnfState *state,
                         GError **error)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    DnfPackage *pkg;
    gboolean valid;
    guint i;
    g_autoptr(GPtrArray) packages = NULL;

    /* depsolve */
    if (!dnf_goal_depsolve(goal, DNF_ALLOW_UNINSTALL, error))
        return FALSE;

    /* find a list of all the packages we have to download */
    g_ptr_array_set_size(priv->pkgs_to_download, 0);
    packages = dnf_goal_get_packages(goal,
                                     DNF_PACKAGE_INFO_INSTALL,
                                     DNF_PACKAGE_INFO_REINSTALL,
                                     DNF_PACKAGE_INFO_DOWNGRADE,
                                     DNF_PACKAGE_INFO_UPDATE,
                                     -1);
    g_debug("Goal has %u packages", packages->len);
    for (i = 0; i < packages->len; i++) {
        pkg = g_ptr_array_index(packages, i);

        /* get correct package repo */
        if (!dnf_transaction_ensure_repo(transaction, pkg, error))
            return FALSE;

        /* this is a local file */
        if (g_strcmp0(dnf_package_get_reponame(pkg),
                      HY_CMDLINE_REPO_NAME) == 0) {
            continue;
        }

        /* check package exists and checksum is okay */
        if (!dnf_package_check_filename(pkg, &valid, error))
            return FALSE;

        /* package needs to be downloaded */
        if (!valid) {
            g_ptr_array_add(priv->pkgs_to_download,
                            g_object_ref(pkg));
        }
    }
    return TRUE;
}

/**
 * dnf_transaction_reset:
 */
static void
dnf_transaction_reset(DnfTransaction *transaction)
{
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);

    /* reset */
    priv->child = NULL;
    g_ptr_array_set_size(priv->pkgs_to_download, 0);
    rpmtsEmpty(priv->ts);
    rpmtsSetNotifyCallback(priv->ts, NULL, NULL);

    /* clear */
    if (priv->install != NULL) {
        g_ptr_array_unref(priv->install);
        priv->install = NULL;
    }
    if (priv->remove != NULL) {
        g_ptr_array_unref(priv->remove);
        priv->remove = NULL;
    }
    if (priv->remove_helper != NULL) {
        g_ptr_array_unref(priv->remove_helper);
        priv->remove_helper = NULL;
    }
    if (priv->erased_by_package_hash != NULL) {
        g_hash_table_unref(priv->erased_by_package_hash);
        priv->erased_by_package_hash = NULL;
    }
}

/**
 * dnf_transaction_import_keys:
 * @transaction: a #DnfTransaction instance.
 * @error: A #GError or %NULL
 *
 * Imports all keys from /etc/pki/rpm-gpg as well as any
 * downloaded per-repo keys.  Note this is called automatically
 * by dnf_transaction_commit().
 **/
gboolean
dnf_transaction_import_keys(DnfTransaction *transaction,
                            GError **error)
{
    guint i;

    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);
    /* import all system wide GPG keys */
    if (!dnf_keyring_add_public_keys(priv->keyring, error))
        return FALSE;

    /* import downloaded repo GPG keys */
    for (i = 0; i < priv->repos->len; i++) {
        DnfRepo *repo = g_ptr_array_index(priv->repos, i);
        g_auto(GStrv) pubkeys = dnf_repo_get_public_keys(repo);

        /* does this file actually exist */
        for (char **iter = pubkeys; iter && *iter; iter++) {
            const char *pubkey = *iter;
            if (g_file_test(pubkey, G_FILE_TEST_EXISTS)) {
                /* import */
                if (!dnf_keyring_add_public_key(priv->keyring, pubkey, error))
                    return FALSE;
            }
        }
    }

    return TRUE;
}


/**
 * dnf_transaction_commit:
 * @transaction: a #DnfTransaction instance.
 * @goal: A #HyGoal
 * @state: A #DnfState
 * @error: A #GError or %NULL
 *
 * Commits a transaction by installing and removing packages.
 *
 * NOTE: If this fails, you need to call dnf_transaction_depsolve() again.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_transaction_commit(DnfTransaction *transaction,
                       HyGoal goal,
                       DnfState *state,
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
    DnfState *state_local;
    GPtrArray *all_obsoleted;
    GPtrArray *pkglist;
    DnfPackage *pkg;
    DnfPackage *pkg_tmp;
    rpmprobFilterFlags problems_filter = 0;
    rpmtransFlags rpmts_flags = RPMTRANS_FLAG_NONE;
    DnfTransactionPrivate *priv = GET_PRIVATE(transaction);

    /* take lock */
    ret = dnf_state_take_lock(state,
                              DNF_LOCK_TYPE_RPMDB,
                              DNF_LOCK_MODE_PROCESS,
                              error);
    if (!ret)
        goto out;

    /* set state */
    if (priv->flags & DNF_TRANSACTION_FLAG_TEST) {
        ret = dnf_state_set_steps(state,
                                  error,
                                  2, /* install */
                                  2, /* remove */
                                  10, /* test-commit */
                                  86, /* commit */
                                  -1);
    } else {
        ret = dnf_state_set_steps(state,
                                  error,
                                  2, /* install */
                                  2, /* remove */
                                  10, /* test-commit */
                                  83, /* commit */
                                  1, /* write yumDB */
                                  2, /* delete files */
                                  -1);
    }
    if (!ret)
        goto out;

    ret = dnf_transaction_import_keys(transaction, error);
    if (!ret)
        goto out;

    /* find any packages without valid GPG signatures */
    ret = dnf_transaction_check_untrusted(transaction, goal, error);
    if (!ret)
        goto out;

    dnf_state_action_start(state, DNF_STATE_ACTION_REQUEST, NULL);

    /* get verbosity from the config file */
    tmp = dnf_context_get_rpm_verbosity(priv->context);
    verbosity = dnf_rpm_verbosity_string_to_value(tmp);
    rpmSetVerbosity(verbosity);

    /* setup the transaction */
    tmp = dnf_context_get_install_root(priv->context);
    rc = rpmtsSetRootDir(priv->ts, tmp);
    if (rc < 0) {
        ret = FALSE;
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_INTERNAL_ERROR,
                            _("failed to set root"));
        goto out;
    }
    rpmtsSetNotifyCallback(priv->ts,
                           dnf_transaction_ts_progress_cb,
                           transaction);

    /* add things to install */
    state_local = dnf_state_get_child(state);
    priv->install = dnf_goal_get_packages(goal,
                                          DNF_PACKAGE_INFO_INSTALL,
                                          DNF_PACKAGE_INFO_REINSTALL,
                                          DNF_PACKAGE_INFO_DOWNGRADE,
                                          DNF_PACKAGE_INFO_UPDATE,
                                          -1);
    if (priv->install->len > 0)
        dnf_state_set_number_steps(state_local,
                                   priv->install->len);
    for (i = 0; i < priv->install->len; i++) {

        pkg = g_ptr_array_index(priv->install, i);
        ret = dnf_transaction_ensure_repo(transaction, pkg, error);
        if (!ret)
            goto out;

        /* add the install */
        filename = dnf_package_get_filename(pkg);
        allow_untrusted =(priv->flags & DNF_TRANSACTION_FLAG_ONLY_TRUSTED) == 0;
        is_update = dnf_package_get_action(pkg) == DNF_STATE_ACTION_UPDATE ||
                dnf_package_get_action(pkg) == DNF_STATE_ACTION_DOWNGRADE;
        ret = dnf_rpmts_add_install_filename(priv->ts,
                                             filename,
                                             allow_untrusted,
                                             is_update,
                                             error);
        if (!ret)
            goto out;

        /* this section done */
        ret = dnf_state_done(state_local, error);
        if (!ret)
            goto out;
    }

    /* this section done */
    ret = dnf_state_done(state, error);
    if (!ret)
        goto out;

    /* add things to remove */
    priv->remove = dnf_goal_get_packages(goal,
                                         DNF_PACKAGE_INFO_OBSOLETE,
                                         DNF_PACKAGE_INFO_REMOVE,
                                         -1);
    for (i = 0; i < priv->remove->len; i++) {
        pkg = g_ptr_array_index(priv->remove, i);
        ret = dnf_rpmts_add_remove_pkg(priv->ts, pkg, error);
        if (!ret)
            goto out;

        /* pre-get the pkgid, as this isn't possible to get after
         * the sack is invalidated */
        if (dnf_package_get_pkgid(pkg) == NULL) {
            g_warning("failed to pre-get pkgid for %s",
                      dnf_package_get_package_id(pkg));
        }

        /* are the things being removed actually being upgraded */
        pkg_tmp = dnf_find_pkg_from_name(priv->install,
                                         dnf_package_get_name(pkg));
        if (pkg_tmp != NULL)
            dnf_package_set_action(pkg, DNF_STATE_ACTION_CLEANUP);
    }

    /* add anything that gets obsoleted to a helper array which is used to
     * map removed packages auto-added by rpm to actual DnfPackage's */
    priv->remove_helper = g_ptr_array_new_with_free_func((GDestroyNotify) g_object_unref);
    for (i = 0; i < priv->install->len; i++) {
        pkg = g_ptr_array_index(priv->install, i);
        is_update = dnf_package_get_action(pkg) == DNF_STATE_ACTION_UPDATE ||
                    dnf_package_get_action(pkg) == DNF_STATE_ACTION_DOWNGRADE;
        if (!is_update)
            continue;
        pkglist = hy_goal_list_obsoleted_by_package(goal, pkg);
        for (j = 0; j < pkglist->len; j++) {
            pkg_tmp = g_ptr_array_index (pkglist, j);
            g_ptr_array_add(priv->remove_helper,
                            g_object_ref(pkg_tmp));
            dnf_package_set_action(pkg_tmp, DNF_STATE_ACTION_CLEANUP);
        }
        g_ptr_array_unref(pkglist);
    }

    /* this section done */
    ret = dnf_state_done(state, error);
    if (!ret)
        goto out;

    /* map updated packages to their previous versions */
    priv->erased_by_package_hash = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                         g_free, (GDestroyNotify) g_object_unref);
    all_obsoleted = hy_goal_list_obsoleted(goal, NULL);
    for (i = 0; i < priv->install->len; i++) {
        pkg = g_ptr_array_index(priv->install, i);
        if (dnf_package_get_action(pkg) != DNF_STATE_ACTION_UPDATE &&
            dnf_package_get_action(pkg) != DNF_STATE_ACTION_DOWNGRADE &&
            dnf_package_get_action(pkg) != DNF_STATE_ACTION_REINSTALL)
            continue;

        pkglist = hy_goal_list_obsoleted_by_package(goal, pkg);
        for (j = 0; j < pkglist->len; j++) {
            pkg_tmp = g_ptr_array_index(pkglist, j);
            if (!hy_packagelist_has(all_obsoleted, pkg_tmp)) {
                g_hash_table_insert(priv->erased_by_package_hash,
                                    g_strdup(dnf_package_get_package_id(pkg)),
                                    g_object_ref(pkg_tmp));
            }
        }
        g_ptr_array_unref(pkglist);
    }
    g_ptr_array_unref(all_obsoleted);

    /* generate ordering for the transaction */
    rpmtsOrder(priv->ts);

    /* run the test transaction */
    if (dnf_context_get_check_transaction(priv->context)) {
        g_debug("running test transaction");
        dnf_state_action_start(state,
                               DNF_STATE_ACTION_TEST_COMMIT,
                               NULL);
        priv->state = dnf_state_get_child(state);
        priv->step = DNF_TRANSACTION_STEP_IGNORE;
        /* the output value of rpmtsCheck is not meaningful */
        rpmtsCheck(priv->ts);
        dnf_state_action_stop(state);
        ret = dnf_rpmts_look_for_problems(priv->ts, error);
        if (!ret)
            goto out;
    }

    /* this section done */
    ret = dnf_state_done(state, error);
    if (!ret)
        goto out;

    /* no signature checking, we've handled that already */
    vs_flags = rpmtsSetVSFlags(priv->ts,
                               _RPMVSF_NOSIGNATURES | _RPMVSF_NODIGESTS);
    rpmtsSetVSFlags(priv->ts, vs_flags);

    /* filter diskspace */
    if (!dnf_context_get_check_disk_space(priv->context))
        problems_filter |= RPMPROB_FILTER_DISKSPACE;
    if (priv->flags & DNF_TRANSACTION_FLAG_ALLOW_REINSTALL)
        problems_filter |= RPMPROB_FILTER_REPLACEPKG;
    if (priv->flags & DNF_TRANSACTION_FLAG_ALLOW_DOWNGRADE)
        problems_filter |= RPMPROB_FILTER_OLDPACKAGE;

    if (priv->flags & DNF_TRANSACTION_FLAG_NODOCS)
        rpmts_flags |= RPMTRANS_FLAG_NODOCS;

    if (priv->flags & DNF_TRANSACTION_FLAG_TEST) {
        /* run the transaction in test mode */
        rpmts_flags |= RPMTRANS_FLAG_TEST;

        priv->state = dnf_state_get_child(state);
        priv->step = DNF_TRANSACTION_STEP_IGNORE;
        rpmtsSetFlags(priv->ts, rpmts_flags);
        g_debug("Running transaction in test mode");
        dnf_state_set_allow_cancel(state, FALSE);
        rc = rpmtsRun(priv->ts, NULL, problems_filter);
        if (rc < 0) {
            ret = FALSE;
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_INTERNAL_ERROR,
                        _("Error %i running transaction test"), rc);
            goto out;
        }
        if (rc > 0) {
            ret = dnf_rpmts_look_for_problems(priv->ts, error);
            if (!ret)
                goto out;
        }

        /* transaction test done; return */
        ret = dnf_state_done(state, error);
        goto out;
    }

    /* run the transaction */
    priv->state = dnf_state_get_child(state);
    priv->step = DNF_TRANSACTION_STEP_STARTED;
    rpmtsSetFlags(priv->ts, rpmts_flags);
    g_debug("Running actual transaction");
    dnf_state_set_allow_cancel(state, FALSE);
    rc = rpmtsRun(priv->ts, NULL, problems_filter);
    if (rc < 0) {
        ret = FALSE;
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    _("Error %i running transaction"), rc);
        goto out;
    }
    if (rc > 0) {
        ret = dnf_rpmts_look_for_problems(priv->ts, error);
        if (!ret)
            goto out;
    }

    /* hmm, nothing was done... */
    if (priv->step != DNF_TRANSACTION_STEP_WRITING) {
        ret = FALSE;
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    _("Transaction did not go to writing phase, "
                      "but returned no error(%i)"),
                    priv->step);
        goto out;
    }

    /* this section done */
    ret = dnf_state_done(state, error);
    if (!ret)
        goto out;

    /* write to the yumDB */
    state_local = dnf_state_get_child(state);
    ret = dnf_transaction_write_yumdb(transaction,
                                      goal,
                                      state_local,
                                      error);
    if (!ret)
        goto out;

    /* this section done */
    ret = dnf_state_done(state, error);
    if (!ret)
        goto out;

    /* remove the files we downloaded */
    if (!dnf_context_get_keep_cache(priv->context)) {
        state_local = dnf_state_get_child(state);
        ret = dnf_transaction_delete_packages(transaction,
                                              state_local,
                                              error);
        if (!ret)
            goto out;
    }

    /* all sacks are invalid now */
    dnf_context_invalidate_full(priv->context, "transaction performed",
                                DNF_CONTEXT_INVALIDATE_FLAG_RPMDB |
                                DNF_CONTEXT_INVALIDATE_FLAG_ENROLLMENT);

    /* this section done */
    ret = dnf_state_done(state, error);
    if (!ret)
        goto out;
out:
    dnf_transaction_reset(transaction);
    dnf_state_release_locks(state);
    return ret;
}

/**
 * dnf_transaction_new:
 * @context: a #DnfContext instance.
 *
 * Creates a new #DnfTransaction.
 *
 * Returns:(transfer full): a #DnfTransaction
 *
 * Since: 0.1.0
 **/
DnfTransaction *
dnf_transaction_new(DnfContext *context)
{
    DnfTransaction *transaction;
    DnfTransactionPrivate *priv;
    transaction = g_object_new(DNF_TYPE_TRANSACTION, NULL);
    priv = GET_PRIVATE(transaction);
    priv->context = context;
    g_object_add_weak_pointer(G_OBJECT(priv->context),(void **) &priv->context);
    priv->ts = rpmtsCreate();
    rpmtsSetRootDir(priv->ts, dnf_context_get_install_root(context));
    priv->keyring = rpmtsGetKeyring(priv->ts, 1);
    priv->db = dnf_db_new(context);
    /* propagate db enablement */
    dnf_db_set_enabled(priv->db, dnf_context_get_yumdb_enabled(context));
    return DNF_TRANSACTION(transaction);
}
