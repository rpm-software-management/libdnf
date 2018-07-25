/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2014-2015 Richard Hughes <richard@hughsie.com>
 * Copyright © 2016  Igor Gnatenko <ignatenko@redhat.com>
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
 * SECTION:dnf-context
 * @short_description: High level interface to libdnf.
 * @include: libdnf.h
 * @stability: Stable
 *
 * This object is a high level interface that does not allow the the user
 * to use objects from librepo, rpm or hawkey directly.
 */

#include <string>
#include <vector>
#include <gio/gio.h>
#include <rpm/rpmlib.h>
#include <rpm/rpmmacro.h>
#include <rpm/rpmts.h>
#include <rpm/rpmdb.h>
#include <librepo/librepo.h>
#ifdef RHSM_SUPPORT
#include <rhsm/rhsm.h>
#endif

#include "dnf-lock.h"
#include "dnf-package.h"
#include "dnf-repo-loader.h"
#include "dnf-sack.h"
#include "dnf-state.h"
#include "dnf-transaction.h"
#include "dnf-utils.h"
#include "dnf-sack.h"
#include "hy-query.h"
#include "hy-subject.h"
#include "hy-selector.h"
#include "dnf-repo.hpp"

#define MAX_NATIVE_ARCHES    12

#define RELEASEVER_PROV "system-release(releasever)"

/* data taken from https://github.com/rpm-software-management/dnf/blob/master/dnf/arch.py */
static const struct {
    const gchar    *base;
    const gchar    *native[MAX_NATIVE_ARCHES];
} arch_map[] =  {
    { "aarch64",    { "aarch64", NULL } },
    { "alpha",      { "alpha", "alphaev4", "alphaev45", "alphaev5",
                      "alphaev56", "alphaev6", "alphaev67",
                      "alphaev68", "alphaev7", "alphapca56", NULL } },
    { "arm",        { "armv5tejl", "armv5tel", "armv5tl", "armv6l", "armv7l", "armv8l", NULL } },
    { "armhfp",     { "armv7hl", "armv7hnl", "armv8hl", "armv8hnl", "armv8hcnl", NULL } },
    { "i386",       { "i386", "athlon", "geode", "i386",
                      "i486", "i586", "i686", NULL } },
    { "ia64",       { "ia64", NULL } },
    { "mips",       { "mips", NULL } },
    { "mipsel",     { "mipsel", NULL } },
    { "mips64",     { "mips64", NULL } },
    { "mips64el",   { "mips64el", NULL } },
    { "noarch",     { "noarch", NULL } },
    { "ppc",        { "ppc", NULL } },
    { "ppc64",      { "ppc64", "ppc64iseries", "ppc64p7",
                      "ppc64pseries", NULL } },
    { "ppc64le",    { "ppc64le", NULL } },
    { "s390",       { "s390", NULL } },
    { "s390x",      { "s390x", NULL } },
    { "sh3",        { "sh3", NULL } },
    { "sh4",        { "sh4", "sh4a", NULL } },
    { "sparc",      { "sparc", "sparc64", "sparc64v", "sparcv8",
                      "sparcv9", "sparcv9v", NULL } },
    { "x86_64",     { "x86_64", "amd64", "ia32e", NULL } },
    { NULL,         { NULL } }
};

typedef struct
{
    gchar            *repo_dir;
    gchar            *base_arch;
    gchar            *release_ver;
    gchar            *platform_module;
    gchar            *cache_dir;
    gchar            *solv_dir;
    gchar            *vendor_cache_dir;
    gchar            *vendor_solv_dir;
    gchar            *lock_dir;
    gchar            *os_info;
    gchar            *arch_info;
    gchar            *install_root;
    gchar            *source_root;
    gchar            *rpm_verbosity;
    gchar            **native_arches;
    gchar            *http_proxy;
    gchar            *user_agent;
    gchar            *arch;
    guint            cache_age;     /*seconds*/
    gboolean         check_disk_space;
    gboolean         check_transaction;
    gboolean         only_trusted;
    gboolean         enable_filelists;
    gboolean         keep_cache;
    gboolean         enrollment_valid;
    DnfLock         *lock;
    DnfTransaction  *transaction;
    GThread         *transaction_thread;
    GFileMonitor    *monitor_rpmdb;
    GHashTable      *override_macros;

    /* used to implement a transaction */
    DnfRepoLoader   *repo_loader;
    GPtrArray       *repos;
    DnfState        *state;        /* used for setup() and run() */
    HyGoal           goal;
    DnfSack         *sack;
} DnfContextPrivate;

enum {
    SIGNAL_INVALIDATE,
    SIGNAL_LAST
};

static guint signals [SIGNAL_LAST] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(DnfContext, dnf_context, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (static_cast<DnfContextPrivate *>(dnf_context_get_instance_private (o)))

/**
 * dnf_context_finalize:
 **/
static void
dnf_context_finalize(GObject *object)
{
    DnfContext *context = DNF_CONTEXT(object);
    DnfContextPrivate *priv = GET_PRIVATE(context);

    g_free(priv->repo_dir);
    g_free(priv->base_arch);
    g_free(priv->release_ver);
    g_free(priv->platform_module);
    g_free(priv->cache_dir);
    g_free(priv->solv_dir);
    g_free(priv->vendor_cache_dir);
    g_free(priv->vendor_solv_dir);
    g_free(priv->lock_dir);
    g_free(priv->rpm_verbosity);
    g_free(priv->install_root);
    g_free(priv->source_root);
    g_free(priv->os_info);
    g_free(priv->arch_info);
    g_free(priv->http_proxy);
    g_free(priv->user_agent);
    g_free(priv->arch);
    g_strfreev(priv->native_arches);
    g_object_unref(priv->lock);
    g_object_unref(priv->state);
    g_hash_table_unref(priv->override_macros);

    if (priv->transaction != NULL)
        g_object_unref(priv->transaction);
    if (priv->repo_loader != NULL)
        g_object_unref(priv->repo_loader);
    if (priv->repos != NULL)
        g_ptr_array_unref(priv->repos);
    if (priv->goal != NULL)
        hy_goal_free(priv->goal);
    if (priv->sack != NULL)
        g_object_unref(priv->sack);
    if (priv->monitor_rpmdb != NULL)
        g_object_unref(priv->monitor_rpmdb);

    G_OBJECT_CLASS(dnf_context_parent_class)->finalize(object);
}

/**
 * dnf_context_init:
 **/
static void
dnf_context_init(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    priv->install_root = g_strdup("/");
    priv->check_disk_space = TRUE;
    priv->check_transaction = TRUE;
    priv->enable_filelists = TRUE;
    priv->state = dnf_state_new();
    priv->lock = dnf_lock_new();
    priv->cache_age = 60 * 60 * 24 * 7; /* 1 week */
    priv->override_macros = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                  g_free, g_free);
    priv->user_agent = g_strdup("libdnf/" PACKAGE_VERSION);

    /* Initialize some state that used to happen in
     * dnf_context_setup(), because callers like rpm-ostree want
     * access to the basearch, but before installroot.
     */
    (void) dnf_context_globals_init(NULL);
}

/**
 * dnf_context_globals_init:
 * @error: a #GError or %NULL
 *
 * Sadly RPM has process global data.  You should invoke
 * this function early on in process startup.  If not,
 * it will be invoked for you.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.7.0
 */
gboolean
dnf_context_globals_init (GError **error)
{
    static gsize initialized = 0;
    gboolean ret = TRUE;

    if (g_once_init_enter (&initialized)) {

        /* librepo's globals */
        lr_global_init();

        /* librpm's globals */
        if (rpmReadConfigFiles(NULL, NULL) != 0) {
            g_set_error_literal(error,
                                DNF_ERROR,
                                DNF_ERROR_INTERNAL_ERROR,
                                "failed to read rpm config files");
            ret = FALSE;
        }

        g_once_init_leave (&initialized, 1);
    }
    return ret;
}

/**
 * dnf_context_class_init:
 **/
static void
dnf_context_class_init(DnfContextClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_context_finalize;

    signals [SIGNAL_INVALIDATE] =
        g_signal_new("invalidate",
                  G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET(DnfContextClass, invalidate),
                  NULL, NULL, g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);
}

/**
 * dnf_context_get_repo_dir:
 * @context: a #DnfContext instance.
 *
 * Gets the context ID.
 *
 * Returns: the context ID, e.g. "fedora-updates"
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_context_get_repo_dir(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->repo_dir;
}

/**
 * dnf_context_get_base_arch:
 * @context: a #DnfContext instance.
 *
 * Gets the context ID.
 *
 * Returns: the base architecture, e.g. "x86_64"
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_context_get_base_arch(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    int i, j;
    const char *value;

    if (priv->base_arch)
        return priv->base_arch;

    /* get info from RPM */
    rpmGetOsInfo(&value, NULL);
    priv->os_info = g_strdup(value);
    rpmGetArchInfo(&value, NULL);
    priv->arch_info = g_strdup(value);

    /* find the base architecture */
    for (i = 0; arch_map[i].base != NULL && priv->base_arch == NULL; i++) {
        for (j = 0; arch_map[i].native[j] != NULL; j++) {
            if (g_strcmp0(arch_map[i].native[j], value) == 0) {
                priv->base_arch = g_strdup(arch_map[i].base);
                break;
            }
        }
    }

    return priv->base_arch;
}

/**
 * dnf_context_get_os_info:
 * @context: a #DnfContext instance.
 *
 * Gets the OS info.
 *
 * Returns: the OS info, e.g. "Linux"
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_context_get_os_info(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->os_info;
}

/**
 * dnf_context_get_arch_info:
 * @context: a #DnfContext instance.
 *
 * Gets the architecture info.
 *
 * Returns: the architecture info, e.g. "x86_64"
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_context_get_arch_info(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->arch_info;
}

/**
 * dnf_context_get_release_ver:
 * @context: a #DnfContext instance.
 *
 * Gets the release version.
 *
 * Returns: the version, e.g. "20"
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_context_get_release_ver(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->release_ver;
}

/**
 * dnf_context_get_platform_module:
 * @context: a #DnfContext instance.
 *
 * Gets the platform module seted in context (not autodetected)
 *
 * Returns: the name:stream, e.g. "platform:f28"
 *
 * Since: 0.16.1
 **/
const gchar *
dnf_context_get_platform_module(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->platform_module;
}

/**
 * dnf_context_get_cache_dir:
 * @context: a #DnfContext instance.
 *
 * Gets the cache dir to use for metadata files.
 *
 * Returns: fully specified path
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_context_get_cache_dir(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->cache_dir;
}

/**
 * dnf_context_get_arch:
 * @context: a #DnfContext instance.
 *
 * Gets the arch that was previously setted by dnf_context_set_arch().
 *
 * Returns: arch
 * Since: 0.15.0
 **/
const gchar *
dnf_context_get_arch(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->arch;
}

/**
 * dnf_context_get_solv_dir:
 * @context: a #DnfContext instance.
 *
 * Gets the solve cache directory.
 *
 * Returns: fully specified path
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_context_get_solv_dir(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->solv_dir;
}

/**
 * dnf_context_get_lock_dir:
 * @context: a #DnfContext instance.
 *
 * Gets the lock directory.
 *
 * Returns: fully specified path
 *
 * Since: 0.1.4
 **/
const gchar *
dnf_context_get_lock_dir(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->lock_dir;
}

/**
 * dnf_context_get_rpm_verbosity:
 * @context: a #DnfContext instance.
 *
 * Gets the RPM verbosity string.
 *
 * Returns: the verbosity string, e.g. "info"
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_context_get_rpm_verbosity(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->rpm_verbosity;
}

/**
 * dnf_context_get_install_root:
 * @context: a #DnfContext instance.
 *
 * Gets the install root, by default "/".
 *
 * Returns: the install root, e.g. "/tmp/snapshot"
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_context_get_install_root(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->install_root;
}

/**
 * dnf_context_get_source_root:
 * @context: a #DnfContext instance.
 *
 * Gets the source root, by default "/".
 *
 * Returns: the source root, e.g. "/tmp/my_os"
 *
 * Since: 0.7.0
 **/
const gchar *
dnf_context_get_source_root(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->source_root != NULL ? priv->source_root : priv->install_root;
}

/**
 * dnf_context_get_native_arches:
 * @context: a #DnfContext instance.
 *
 * Gets the native architectures, by default "noarch" and "i386".
 *
 * Returns: (transfer none): the native architectures
 *
 * Since: 0.1.0
 **/
const gchar **
dnf_context_get_native_arches(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return(const gchar **) priv->native_arches;
}

/**
 * dnf_context_get_repo_loader:
 * @context: a #DnfContext instance.
 *
 * Gets the repo loader used by the transaction.
 *
 * Returns: (transfer none): the repo loader
 *
 * Since: 0.7.0
 **/
DnfRepoLoader *
dnf_context_get_repo_loader(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->repo_loader;
}

/**
 * dnf_context_get_repos:
 * @context: a #DnfContext instance.
 *
 * Gets the repos used by the transaction.
 *
 * Returns: (transfer none) (element-type DnfRepo): the repo list
 *
 * Since: 0.1.0
 **/
GPtrArray *
dnf_context_get_repos(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->repos;
}

/**
 * dnf_context_ensure_transaction:
 **/
static void
dnf_context_ensure_transaction(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);

    /* create if not yet created */
    if (priv->transaction == NULL) {
        priv->transaction = dnf_transaction_new(context);
        priv->transaction_thread = g_thread_self();
        dnf_transaction_set_repos(priv->transaction, priv->repos);
        return;
    }

    /* check the transaction is not being used from a different thread */
    if (priv->transaction_thread != g_thread_self())
        g_warning("transaction being re-used by a different thread!");
}

/**
 * dnf_context_get_transaction:
 * @context: a #DnfContext instance.
 *
 * Gets the transaction used by the transaction.
 *
 * IMPORTANT: This function cannot be used if #DnfContext is being re-used by
 * different threads, even if threads are run one at a time. If you're doing
 * this, just create a DnfTransaction for each thread rather than using the
 * context version.
 *
 * Returns: (transfer none): the DnfTransaction object
 *
 * Since: 0.1.0
 **/
DnfTransaction *
dnf_context_get_transaction(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    /* ensure transaction exists */
    dnf_context_ensure_transaction(context);
    return priv->transaction;
}

/**
 * dnf_context_get_sack:(skip)
 * @context: a #DnfContext instance.
 *
 * Returns: (transfer none): the DnfSack object
 *
 * Since: 0.1.0
 **/
DnfSack *
dnf_context_get_sack(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->sack;
}

/**
 * dnf_context_get_goal:(skip)
 * @context: a #DnfContext instance.
 *
 * Returns: (transfer none): the HyGoal object
 *
 * Since: 0.1.0
 **/
HyGoal
dnf_context_get_goal(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->goal;
}

/**
 * dnf_context_get_state:
 * @context: a #DnfContext instance.
 *
 * Returns: (transfer none): the DnfState object
 *
 * Since: 0.1.2
 **/
DnfState*
dnf_context_get_state(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->state;
}

/**
 * dnf_context_get_user_agent:
 * @context: a #DnfContext instance.
 *
 * Returns: (transfer none): the user agent
 **/
const gchar *
dnf_context_get_user_agent (DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->user_agent;
}

/**
 * dnf_context_get_check_disk_space:
 * @context: a #DnfContext instance.
 *
 * Gets the diskspace check value.
 *
 * Returns: %TRUE if diskspace should be checked before the transaction
 *
 * Since: 0.1.0
 **/
gboolean
dnf_context_get_check_disk_space(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->check_disk_space;
}

/**
 * dnf_context_get_check_transaction:
 * @context: a #DnfContext instance.
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
dnf_context_get_check_transaction(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->check_transaction;
}

/**
 * dnf_context_get_keep_cache:
 * @context: a #DnfContext instance.
 *
 * Gets if the downloaded packages are kept.
 *
 * Returns: %TRUE if the packages will not be deleted
 *
 * Since: 0.1.0
 **/
gboolean
dnf_context_get_keep_cache(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->keep_cache;
}

/**
 * dnf_context_get_only_trusted:
 * @context: a #DnfContext instance.
 *
 * Gets if only trusted packages can be installed.
 *
 * Returns: %TRUE if only trusted packages are allowed
 *
 * Since: 0.1.0
 **/
gboolean
dnf_context_get_only_trusted(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->only_trusted;
}

/**
 * dnf_context_get_enable_filelists:
 * @context: a #DnfContext instance.
 *
 * Returns: %TRUE if filelists are enabled
 *
 * Since: 0.13.0
 */
gboolean
dnf_context_get_enable_filelists (DnfContext     *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->enable_filelists;
}

/**
 * dnf_context_get_cache_age:
 * @context: a #DnfContext instance.
 *
 * Gets the maximum cache age.
 *
 * Returns: cache age in seconds
 *
 * Since: 0.1.0
 **/
guint
dnf_context_get_cache_age(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->cache_age;
}

/**
 * dnf_context_get_installonly_pkgs:
 * @context: a #DnfContext instance.
 *
 * Gets the packages that are allowed to be installed more than once.
 *
 * Returns: (transfer none): array of package names
 */
const gchar **
dnf_context_get_installonly_pkgs(DnfContext *context)
{
    static const gchar *installonly_pkgs[] = {
        "kernel",
        "kernel-PAE",
        "installonlypkg(kernel)",
        "installonlypkg(kernel-module)",
        "installonlypkg(vm)",
        "multiversion(kernel)",
        NULL };
    return installonly_pkgs;
}

/**
 * dnf_context_get_installonly_limit:
 * @context: a #DnfContext instance.
 *
 * Gets the maximum number of packages for installonly packages
 *
 * Returns: integer value
 */
guint
dnf_context_get_installonly_limit(DnfContext *context)
{
    return 3;
}

/**
 * dnf_context_set_repo_dir:
 * @context: a #DnfContext instance.
 * @repo_dir: the repodir, e.g. "/etc/yum.repos.d"
 *
 * Sets the repo directory.
 *
 * Since: 0.1.0
 **/
void
dnf_context_set_repo_dir(DnfContext *context, const gchar *repo_dir)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_free(priv->repo_dir);
    priv->repo_dir = g_strdup(repo_dir);
}

/**
 * dnf_context_set_release_ver:
 * @context: a #DnfContext instance.
 * @release_ver: the release version, e.g. "20"
 *
 * Sets the release version.
 *
 * Since: 0.1.0
 **/
void
dnf_context_set_release_ver(DnfContext *context, const gchar *release_ver)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_free(priv->release_ver);
    priv->release_ver = g_strdup(release_ver);
}

/**
 * dnf_context_set_platform_module:
 * @context: a #DnfContext instance.
 * @release_ver: the platform name:stream, e.g. "platform:28"
 *
 * Sets the platform module. If value is set it disable autodetection of platform module.
 *
 * Since: 0.16.1
 **/
void
dnf_context_set_platform_module(DnfContext *context, const gchar *platform_module)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_free(priv->platform_module);
    priv->platform_module = g_strdup(platform_module);
}

/**
 * dnf_context_set_cache_dir:
 * @context: a #DnfContext instance.
 * @cache_dir: the cachedir, e.g. "/var/cache/PackageKit"
 *
 * Sets the cache directory.
 *
 * Since: 0.1.0
 **/
void
dnf_context_set_cache_dir(DnfContext *context, const gchar *cache_dir)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_free(priv->cache_dir);
    priv->cache_dir = g_strdup(cache_dir);
}

/**
 * dnf_context_set_arch:
 * @context: a #DnfContext instance.
 * @base_arch: the base_arch, e.g. "x86_64"
 *
 * Sets the base arch of sack.
 *
 * Since: 0.15.0
 **/
void
dnf_context_set_arch(DnfContext *context, const gchar *arch)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_free(priv->cache_dir);
    priv->arch = g_strdup(arch);
}

/**
 * dnf_context_set_solv_dir:
 * @context: a #DnfContext instance.
 * @solv_dir: the solve cache, e.g. "/var/cache/PackageKit/hawkey"
 *
 * Sets the solve cache directory.
 *
 * Since: 0.1.0
 **/
void
dnf_context_set_solv_dir(DnfContext *context, const gchar *solv_dir)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_free(priv->solv_dir);
    priv->solv_dir = g_strdup(solv_dir);
}

/**
 * dnf_context_set_vendor_cache_dir:
 * @context: a #DnfContext instance.
 * @vendor_cache_dir: the cachedir, e.g. "/usr/share/PackageKit/metadata"
 *
 * Sets the vendor cache directory.
 *
 * Since: 0.1.6
 **/
void
dnf_context_set_vendor_cache_dir(DnfContext *context, const gchar *vendor_cache_dir)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_free(priv->vendor_cache_dir);
    priv->vendor_cache_dir = g_strdup(vendor_cache_dir);
}

/**
 * dnf_context_set_vendor_solv_dir:
 * @context: a #DnfContext instance.
 * @vendor_solv_dir: the solve cache, e.g. "/usr/share/PackageKit/hawkey"
 *
 * Sets the vendor solve cache directory.
 *
 * Since: 0.1.6
 **/
void
dnf_context_set_vendor_solv_dir(DnfContext *context, const gchar *vendor_solv_dir)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_free(priv->vendor_solv_dir);
    priv->vendor_solv_dir = g_strdup(vendor_solv_dir);
}

/**
 * dnf_context_set_lock_dir:
 * @context: a #DnfContext instance.
 * @lock_dir: the solve cache, e.g. "/var/run"
 *
 * Sets the lock directory.
 *
 * Since: 0.1.4
 **/
void
dnf_context_set_lock_dir(DnfContext *context, const gchar *lock_dir)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_free(priv->lock_dir);
    priv->lock_dir = g_strdup(lock_dir);
}

/**
 * dnf_context_set_rpm_verbosity:
 * @context: a #DnfContext instance.
 * @rpm_verbosity: the verbosity string, e.g. "info"
 *
 * Sets the RPM verbosity string.
 *
 * Since: 0.1.0
 **/
void
dnf_context_set_rpm_verbosity(DnfContext *context, const gchar *rpm_verbosity)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_free(priv->rpm_verbosity);
    priv->rpm_verbosity = g_strdup(rpm_verbosity);
}

/**
 * dnf_context_set_install_root:
 * @context: a #DnfContext instance.
 * @install_root: the install root, e.g. "/"
 *
 * Sets the install root to use for installing and removal.
 *
 * Since: 0.1.0
 **/
void
dnf_context_set_install_root(DnfContext *context, const gchar *install_root)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_free(priv->install_root);
    priv->install_root = g_strdup(install_root);
}

/**
 * dnf_context_set_source_root:
 * @context: a #DnfContext instance.
 * @source_root: the source root, e.g. "/"
 *
 * Sets the source root to use for retrieving system information.
 *
 * Since: 0.7.0
 **/
void
dnf_context_set_source_root(DnfContext *context, const gchar *source_root)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_free(priv->source_root);
    priv->source_root = g_strdup(source_root);
}

/**
 * dnf_context_set_check_disk_space:
 * @context: a #DnfContext instance.
 * @check_disk_space: %TRUE to check for diskspace
 *
 * Enables or disables the diskspace check.
 *
 * Since: 0.1.0
 **/
void
dnf_context_set_check_disk_space(DnfContext *context, gboolean check_disk_space)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    priv->check_disk_space = check_disk_space;
}

/**
 * dnf_context_set_check_transaction:
 * @context: a #DnfContext instance.
 * @check_transaction: %TRUE to do a test transaction
 *
 * Enables or disables the test transaction.
 *
 * Since: 0.1.0
 **/
void
dnf_context_set_check_transaction(DnfContext *context, gboolean check_transaction)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    priv->check_transaction = check_transaction;
}

/**
 * dnf_context_set_keep_cache:
 * @context: a #DnfContext instance.
 * @keep_cache: %TRUE keep the packages after installing or updating
 *
 * Enables or disables the automatic package deleting functionality.
 *
 * Since: 0.1.0
 **/
void
dnf_context_set_keep_cache(DnfContext *context, gboolean keep_cache)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    priv->keep_cache = keep_cache;
}

/**
 * dnf_context_set_enable_filelists:
 * @context: a #DnfContext instance.
 * @enable_filelists: %TRUE to download and parse filelist metadata
 *
 * Enables or disables download and parsing of filelists.
 *
 * Since: 0.13.0
 **/
void
dnf_context_set_enable_filelists (DnfContext     *context,
                                  gboolean        enable_filelists)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    priv->enable_filelists = enable_filelists;
}

/**
 * dnf_context_set_only_trusted:
 * @context: a #DnfContext instance.
 * @only_trusted: %TRUE keep the packages after installing or updating
 *
 * Enables or disables the requirement of trusted packages.
 *
 * Since: 0.1.0
 **/
void
dnf_context_set_only_trusted(DnfContext *context, gboolean only_trusted)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    priv->only_trusted = only_trusted;
}

/**
 * dnf_context_set_cache_age:
 * @context: a #DnfContext instance.
 * @cache_age: Maximum cache age in seconds
 *
 * Sets the maximum cache age.
 *
 * Since: 0.1.0
 **/
void
dnf_context_set_cache_age(DnfContext *context, guint cache_age)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    priv->cache_age = cache_age;
}

/**
 * dnf_context_set_os_release:
 **/
static gboolean
dnf_context_set_os_release(DnfContext *context, GError **error)
{
    const char *source_root = dnf_context_get_source_root (context);

    gboolean found_in_rpmdb = FALSE;
    rpmts ts = rpmtsCreate ();
    rpmtsSetRootDir (ts, source_root);
    rpmdbMatchIterator mi = rpmtsInitIterator (ts, RPMTAG_PROVIDENAME, RELEASEVER_PROV, 0);
    Header hdr;
    while ((hdr = rpmdbNextIterator (mi)) != NULL) {
        const char *v = headerGetString (hdr, RPMTAG_VERSION);
        rpmds ds = rpmdsNew (hdr, RPMTAG_PROVIDENAME, 0);
        while (rpmdsNext (ds) >= 0) {
            if (strcmp (rpmdsN (ds), RELEASEVER_PROV) == 0 && rpmdsFlags (ds) == RPMSENSE_EQUAL)
                v = rpmdsEVR (ds);
        }
        found_in_rpmdb = TRUE;
        dnf_context_set_release_ver (context, v);
        rpmdsFree (ds);
        break;
    }
    rpmdbFreeIterator (mi);
    rpmtsFree (ts);
    if (found_in_rpmdb)
        return TRUE;

    g_autofree gchar *contents = NULL;
    g_autofree gchar *maybe_quoted_version = NULL;
    g_autofree gchar *version = NULL;
    g_autofree gchar *os_release = NULL;
    g_autoptr(GString) str = NULL;
    g_autoptr(GKeyFile) key_file = NULL;

    os_release = g_build_filename(source_root, "etc/os-release", NULL);
    if (!dnf_get_file_contents_allow_noent(os_release, &contents, NULL, error))
        return FALSE;

    if (contents == NULL) {
        g_free(os_release);
        /* For rpm-ostree use on systems that haven't adapted to usr/lib/os-release yet,
         * e.g. CentOS 7 Core AH */
        os_release = g_build_filename(source_root, "usr/etc/os-release", NULL);
        if (!dnf_get_file_contents_allow_noent(os_release, &contents, NULL, error))
            return FALSE;
    }

    if (contents == NULL) {
        g_free (os_release);
        os_release = g_build_filename(source_root, "usr/lib/os-release", NULL);
        if (!dnf_get_file_contents_allow_noent(os_release, &contents, NULL, error))
            return FALSE;
    }

    if (contents == NULL) {
        g_set_error(error, DNF_ERROR, DNF_ERROR_FILE_NOT_FOUND,
                    "Could not find os-release in etc/, usr/etc, or usr/lib "
                    "under source root '%s'", source_root);
        return FALSE;
    }

    str = g_string_new(contents);

    /* make a valid GKeyFile from the .ini data by prepending a header */
    g_string_prepend(str, "[os-release]\n");

    key_file = g_key_file_new();
    if (!g_key_file_load_from_data(key_file,
                     str->str, -1,
                     G_KEY_FILE_NONE,
                     error))
        return FALSE;

    /* get keys */
    maybe_quoted_version = g_key_file_get_string(key_file,
                                                 "os-release",
                                                 "VERSION_ID",
                                                 error);
    if (maybe_quoted_version == NULL)
        return FALSE;
    version = g_shell_unquote(maybe_quoted_version, error);
    if (!version)
        return FALSE;
    dnf_context_set_release_ver(context, version);
    return TRUE;
}

/**
 * dnf_context_set_user_agent:
 * @context: Context
 * @user_agent: User-Agent string for HTTP requests
 **/
void
dnf_context_set_user_agent (DnfContext  *context,
                            const gchar *user_agent)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_free (priv->user_agent);
    priv->user_agent = g_strdup (user_agent);
}

/**
 * dnf_context_rpmdb_changed_cb:
 **/
static void
dnf_context_rpmdb_changed_cb(GFileMonitor *monitor_,
                             GFile *file, GFile *other_file,
                             GFileMonitorEvent event_type,
                             DnfContext *context)
{
    dnf_context_invalidate(context, "rpmdb changed");
}

/* A heuristic; check whether /usr exists in the install root */
static gboolean
have_existing_install(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_autofree gchar *usr_path = g_build_filename(priv->install_root, "usr", NULL);
    return g_file_test(usr_path, G_FILE_TEST_IS_DIR);
}

/**
 * dnf_context_setup_sack:(skip)
 * @context: a #DnfContext instance.
 * @state: A #DnfState
 * @error: A #GError or %NULL
 *
 * Sets up the internal sack ready for use -- note, this is called automatically
 * when you use dnf_context_install(), dnf_context_remove() or
 * dnf_context_update(), although you may want to call this manually to control
 * the DnfState children with correct percentage completion.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.3
 **/
gboolean
dnf_context_setup_sack(DnfContext *context, DnfState *state, GError **error)
{
    return dnf_context_setup_sack_with_flags(context, state,
                                             DNF_CONTEXT_SETUP_SACK_FLAG_NONE, error);
}

/**
 * dnf_context_setup_sack_with_flags:(skip)
 * @context: a #DnfContext instance.
 * @state: A #DnfState
 * @flags: A #DnfContextSetupSackFlags
 * @error: A #GError or %NULL
 *
 * Sets up the internal sack ready for use. This provides the same functionality as
 * dnf_context_setup_sack but allows finer control on its behaviour.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.13.0
 **/
gboolean
dnf_context_setup_sack_with_flags(DnfContext               *context,
                                  DnfState                 *state,
                                  DnfContextSetupSackFlags  flags,
                                  GError                  **error)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    gboolean ret;
    g_autofree gchar *solv_dir_real = nullptr;

    /* create empty sack */
    solv_dir_real = dnf_realpath(priv->solv_dir);
    priv->sack = dnf_sack_new();
    dnf_sack_set_cachedir(priv->sack, solv_dir_real);
    dnf_sack_set_rootdir(priv->sack, priv->install_root);
    if (priv->arch) {
        if(!dnf_sack_set_arch(priv->sack, priv->arch, error)) {
            return FALSE;
        }
    }
    if (!dnf_sack_setup(priv->sack, DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR, error))
        return FALSE;
    dnf_sack_set_installonly(priv->sack, dnf_context_get_installonly_pkgs(context));
    dnf_sack_set_installonly_limit(priv->sack, dnf_context_get_installonly_limit(context));

    /* add installed packages */
    const gboolean skip_rpmdb = ((flags & DNF_CONTEXT_SETUP_SACK_FLAG_SKIP_RPMDB) > 0);
    if (!skip_rpmdb && have_existing_install(context)) {
        if (!dnf_sack_load_system_repo(priv->sack,
                                       nullptr,
                                       DNF_SACK_LOAD_FLAG_BUILD_CACHE,
                                       error))
            return FALSE;
    }

    DnfSackAddFlags add_flags = DNF_SACK_ADD_FLAG_NONE;
    if ((flags & DNF_CONTEXT_SETUP_SACK_FLAG_LOAD_UPDATEINFO) > 0)
        add_flags = static_cast<DnfSackAddFlags>(add_flags | DNF_SACK_ADD_FLAG_UPDATEINFO);
    if (priv->enable_filelists && !((flags & DNF_CONTEXT_SETUP_SACK_FLAG_SKIP_FILELISTS) > 0))
        add_flags = static_cast<DnfSackAddFlags>(add_flags | DNF_SACK_ADD_FLAG_FILELISTS);

    /* add remote */
    ret = dnf_sack_add_repos(priv->sack,
                             priv->repos,
                             priv->cache_age,
                             add_flags,
                             state,
                             error);
    if (!ret)
        return FALSE;

    DnfSack *sack = priv->sack;
    if (sack != nullptr) {
        std::vector<const char *> hotfixRepos;
        // don't filter RPMs from repos with the 'module_hotfixes' flag set
        for (unsigned int i = 0; i < priv->repos->len; i++) {
            auto repo = static_cast<DnfRepo *>(g_ptr_array_index(priv->repos, i));
            if (dnf_repo_get_module_hotfixes(repo)) {
                hotfixRepos.push_back(dnf_repo_get_id(repo));
            }
        }
        hotfixRepos.push_back(nullptr);
        dnf_sack_filter_modules(sack, hotfixRepos.data(), priv->install_root, priv->platform_module);
    }

    /* create goal */
    if (priv->goal != nullptr)
        hy_goal_free(priv->goal);
    priv->goal = hy_goal_create(priv->sack);
    return TRUE;
}

/**
 * dnf_context_ensure_exists:
 **/
static gboolean
dnf_context_ensure_exists(const gchar *directory, GError **error)
{
    if (g_file_test(directory, G_FILE_TEST_EXISTS))
        return TRUE;
    if (g_mkdir_with_parents(directory, 0755) != 0) {
        g_set_error(error,
                 DNF_ERROR,
                 DNF_ERROR_INTERNAL_ERROR,
                 "Failed to create: %s", directory);
        return FALSE;
    }
    return TRUE;
}

/**
 * dnf_utils_copy_files:
 */
static gboolean
dnf_utils_copy_files(const gchar *src, const gchar *dest, GError **error)
{
    const gchar *tmp;
    gint rc;
    g_autoptr(GDir) dir = NULL;

    /* create destination directory */
    rc = g_mkdir_with_parents(dest, 0755);
    if (rc < 0) {
        g_set_error(error,
                 DNF_ERROR,
                 DNF_ERROR_FAILED,
                 "failed to create %s", dest);
        return FALSE;
    }

    /* copy files */
    dir = g_dir_open(src, 0, error);
    if (dir == NULL)
        return FALSE;
    while ((tmp = g_dir_read_name(dir)) != NULL) {
        g_autofree gchar *path_src = NULL;
        g_autofree gchar *path_dest = NULL;
        path_src = g_build_filename(src, tmp, NULL);
        path_dest = g_build_filename(dest, tmp, NULL);
        if (g_file_test(path_src, G_FILE_TEST_IS_DIR)) {
            if (!dnf_utils_copy_files(path_src, path_dest, error))
                return FALSE;
        } else {
            g_autoptr(GFile) file_src = NULL;
            g_autoptr(GFile) file_dest = NULL;
            file_src = g_file_new_for_path(path_src);
            file_dest = g_file_new_for_path(path_dest);
            if (!g_file_copy(file_src, file_dest,
                      G_FILE_COPY_NONE,
                      NULL, NULL, NULL,
                      error))
                return FALSE;
        }
    }
    return TRUE;
}

/**
 * dnf_context_copy_vendor_cache:
 *
 * The vendor cache is typically structured like this:
 * /usr/share/PackageKit/metadata/fedora/repodata/primary.xml.gz
 * /usr/share/PackageKit/metadata/updates/repodata/primary.xml.gz
 **/
static gboolean
dnf_context_copy_vendor_cache(DnfContext *context, GError **error)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    DnfRepo *repo;
    const gchar *id;
    guint i;

    /* not set, or does not exists */
    if (priv->vendor_cache_dir == NULL)
        return TRUE;
    if (!g_file_test(priv->vendor_cache_dir, G_FILE_TEST_EXISTS))
        return TRUE;

    /* test each enabled repo in turn */
    for (i = 0; i < priv->repos->len; i++) {
        g_autofree gchar *path = NULL;
        g_autofree gchar *path_vendor = NULL;
        repo = static_cast<DnfRepo *>(g_ptr_array_index(priv->repos, i));
        if (dnf_repo_get_enabled(repo) == DNF_REPO_ENABLED_NONE)
            continue;

        /* does the repo already exist */
        id = dnf_repo_get_id(repo);
        path = g_build_filename(priv->cache_dir, id, NULL);
        if (g_file_test(path, G_FILE_TEST_EXISTS))
            continue;

        /* copy the files */
        path_vendor = g_build_filename(priv->vendor_cache_dir, id, NULL);
        if (!g_file_test(path_vendor, G_FILE_TEST_EXISTS))
            continue;
        g_debug("copying files from %s to %s", path_vendor, path);
        if (!dnf_utils_copy_files(path_vendor, path, error))
            return FALSE;
    }

    return TRUE;
}

/**
 * dnf_context_copy_vendor_solv:
 *
 * The solv cache is typically structured like this:
 * /usr/share/PackageKit/hawkey/@System.solv
 * /usr/share/PackageKit/hawkey/fedora-filenames.solvx
 **/
static gboolean
dnf_context_copy_vendor_solv(DnfContext *context, GError **error)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_autofree gchar *system_db = NULL;

    /* not set, or does not exists */
    if (priv->vendor_solv_dir == NULL)
        return TRUE;
    if (!g_file_test(priv->vendor_solv_dir, G_FILE_TEST_EXISTS))
        return TRUE;

    /* already exists */
    system_db = g_build_filename(priv->solv_dir, "@System.solv", NULL);
    if (g_file_test(system_db, G_FILE_TEST_EXISTS))
        return TRUE;

    /* copy all the files */
    g_debug("copying files from %s to %s", priv->vendor_solv_dir, priv->solv_dir);
    if (!dnf_utils_copy_files(priv->vendor_solv_dir, priv->solv_dir, error))
        return FALSE;

    return TRUE;
}

/**
 * dnf_context_set_rpm_macro:
 * @context: a #DnfContext instance.
 * @key: Variable name
 * @value: Variable value
 *
 * Override an RPM macro definition.  This is useful for
 * "_install_langs" and other macros that control aspects of the
 * installation.
 *
 * Since: 0.1.9
 **/
void
dnf_context_set_rpm_macro(DnfContext    *context,
                          const gchar    *key,
                          const gchar  *value)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_hash_table_replace(priv->override_macros, g_strdup(key), g_strdup(value));
}


/**
 * dnf_context_get_http_proxy:
 * @context: a #DnfContext instance.
 *
 * Returns: the HTTP proxy; none is configured by default.
 *
 * Since: 0.1.9
 **/
const gchar *
dnf_context_get_http_proxy(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->http_proxy;
}

/**
 * dnf_context_set_http_proxy:
 * @context: a #DnfContext instance.
 * @proxyurl: Proxy URL
 *
 * Set the HTTP proxy used by default.  Per-repo configuration will
 * override.
 *
 * Since: 0.1.9
 **/
void
dnf_context_set_http_proxy(DnfContext *context, const gchar *proxyurl)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_free(priv->http_proxy);
    priv->http_proxy = g_strdup(proxyurl);
}

/**
 * dnf_context_setup_enrollments:
 * @context: a #DnfContext instance.
 * @error: A #GError or %NULL
 *
 * Resyncs the enrollment with the vendor system. This may change the contents
 * of files in repos.d according to subscription levels.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.2.1
 **/
gboolean
dnf_context_setup_enrollments(DnfContext *context, GError **error)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);

    /* no need to refresh */
    if (priv->enrollment_valid)
        return TRUE;

    /* Let's assume that alternative installation roots don't
     * require entitlement.  We only want to do system things if
     * we're actually affecting the system.  A more accurate test
     * here would be checking that we're using /etc/yum.repos.d or
     * so, but that can come later.
     */
    if (g_strcmp0(priv->install_root, "/") != 0)
        return TRUE;

    /* Also, all of the subman stuff only works as root, if we're not
     * root, assume we're running in the test suite, or we're part of
     * e.g. rpm-ostree which is trying to run totally as non-root.
     */
    if (getuid () != 0)
        return TRUE;

#ifdef RHSM_SUPPORT
    g_autoptr(RHSMContext) rhsm_ctx = rhsm_context_new ();
    g_autofree gchar *repofname = g_build_filename (priv->repo_dir,
                                                    "redhat.repo",
                                                    NULL);
    g_autoptr(GKeyFile) repofile = rhsm_utils_yum_repo_from_context (rhsm_ctx);

    if (!g_key_file_save_to_file (repofile, repofname, error))
        return FALSE;
#endif

    priv->enrollment_valid = TRUE;
    return TRUE;
}

/**
 * dnf_context_setup:
 * @context: a #DnfContext instance.
 * @cancellable: A #GCancellable or %NULL
 * @error: A #GError or %NULL
 *
 * Sets up the context ready for use.
 *
 * This function will not do significant amounts of i/o or download new
 * metadata. Use dnf_context_setup_sack() if you want to populate the internal
 * sack as well.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_context_setup(DnfContext *context,
           GCancellable *cancellable,
           GError **error)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    guint i;
    guint j;
    GHashTableIter hashiter;
    gpointer hashkey, hashval;
    g_autoptr(GString) buf = NULL;
    g_autofree char *rpmdb_path = NULL;
    g_autoptr(GFile) file_rpmdb = NULL;

    /* check essential things are set */
    if (priv->solv_dir == NULL) {
        g_set_error_literal(error,
                     DNF_ERROR,
                     DNF_ERROR_INTERNAL_ERROR,
                     "solv_dir not set");
        return FALSE;
    }

    /* ensure directories exist */
    if (priv->repo_dir != NULL) {
        if (!dnf_context_ensure_exists(priv->repo_dir, error))
            return FALSE;
    }
    if (priv->cache_dir != NULL) {
        if (!dnf_context_ensure_exists(priv->cache_dir, error))
            return FALSE;
    }
    if (priv->solv_dir != NULL) {
        if (!dnf_context_ensure_exists(priv->solv_dir, error))
            return FALSE;
    }
    if (priv->lock_dir != NULL) {
        dnf_lock_set_lock_dir(priv->lock, priv->lock_dir);
        if (!dnf_context_ensure_exists(priv->lock_dir, error))
            return FALSE;
    }
    if (priv->install_root != NULL) {
        if (!dnf_context_ensure_exists(priv->install_root, error))
            return FALSE;
    }

    /* connect if set */
    if (cancellable != NULL)
        dnf_state_set_cancellable(priv->state, cancellable);

    if (!dnf_context_globals_init (error))
        return FALSE;

    buf = g_string_new("");
    g_hash_table_iter_init(&hashiter, priv->override_macros);
    while (g_hash_table_iter_next(&hashiter, &hashkey, &hashval)) {
        g_string_assign(buf, "%define ");
        g_string_append(buf,(gchar*)hashkey);
        g_string_append_c(buf, ' ');
        g_string_append(buf,(gchar*)hashval);
        /* Calling expand with %define (ignoring the return
         * value) is apparently the way to change the global
         * macro context.
         */
        free(rpmExpand(buf->str, NULL));
    }

    (void) dnf_context_get_base_arch(context);

    /* find all the native archs */
    priv->native_arches = g_new0(gchar *, MAX_NATIVE_ARCHES);
    for (i = 0; arch_map[i].base != NULL; i++) {
        if (g_strcmp0(arch_map[i].base, priv->base_arch) == 0) {
            for (j = 0; arch_map[i].native[j] != NULL; j++)
                priv->native_arches[j] = g_strdup(arch_map[i].native[j]);
            priv->native_arches[j++] = g_strdup("noarch");
            break;
        }
    }

    /* get info from OS release file */
    if (priv->release_ver == NULL &&
        !dnf_context_set_os_release(context, error))
        return FALSE;

    /* setup a file monitor on the rpmdb, if we're operating on the native / */
    if (g_strcmp0(priv->install_root, "/") == 0) {
        rpmdb_path = g_build_filename(priv->install_root, "var/lib/rpm/Packages", NULL);
        file_rpmdb = g_file_new_for_path(rpmdb_path);
        priv->monitor_rpmdb = g_file_monitor_file(file_rpmdb,
                               G_FILE_MONITOR_NONE,
                               NULL,
                               error);
        if (priv->monitor_rpmdb == NULL)
            return FALSE;
        g_signal_connect(priv->monitor_rpmdb, "changed",
                         G_CALLBACK(dnf_context_rpmdb_changed_cb), context);
    }

    /* copy any vendor distributed cached metadata */
    if (!dnf_context_copy_vendor_cache(context, error))
        return FALSE;
    if (!dnf_context_copy_vendor_solv(context, error))
        return FALSE;

    /* initialize external frameworks where installed */
    if (!dnf_context_setup_enrollments(context, error))
        return FALSE;

    /* initialize repos */
    priv->repo_loader = dnf_repo_loader_new(context);
    priv->repos = dnf_repo_loader_get_repos(priv->repo_loader, error);
    if (priv->repos == NULL)
        return FALSE;

    return TRUE;
}

/**
 * dnf_context_run:
 * @context: a #DnfContext instance.
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
dnf_context_run(DnfContext *context, GCancellable *cancellable, GError **error)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    DnfState *state_local;
    gboolean ret;

    /* ensure transaction exists */
    dnf_context_ensure_transaction(context);

    /* connect if set */
    dnf_state_reset(priv->state);
    if (cancellable != NULL)
        dnf_state_set_cancellable(priv->state, cancellable);

    ret = dnf_state_set_steps(priv->state, error,
                              5,        /* depsolve */
                              50,       /* download */
                              45,       /* commit */
                              -1);
    if (!ret)
        return FALSE;

    /* depsolve */
    state_local = dnf_state_get_child(priv->state);
    ret = dnf_transaction_depsolve(priv->transaction,
                                   priv->goal,
                                   state_local,
                                   error);
    if (!ret)
        return FALSE;

    /* this section done */
    if (!dnf_state_done(priv->state, error))
        return FALSE;

    /* download */
    state_local = dnf_state_get_child(priv->state);
    ret = dnf_transaction_download(priv->transaction,
                                   state_local,
                                   error);
    if (!ret)
        return FALSE;

    /* this section done */
    if (!dnf_state_done(priv->state, error))
        return FALSE;

    /* commit set up transaction */
    state_local = dnf_state_get_child(priv->state);
    ret = dnf_transaction_commit(priv->transaction,
                                 priv->goal,
                                 state_local,
                                 error);
    if (!ret)
        return FALSE;

    /* this sack is no longer valid */
    g_object_unref(priv->sack);
    priv->sack = NULL;

    /* this section done */
    return dnf_state_done(priv->state, error);
}

/**
 * dnf_context_install:
 * @context: a #DnfContext instance.
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
dnf_context_install (DnfContext *context, const gchar *name, GError **error)
{
    DnfContextPrivate *priv = GET_PRIVATE (context);
    g_autoptr(GPtrArray) selector_matches = NULL;
    HySelector selector = NULL;
    HySubject subject = NULL;
    gboolean ret = FALSE;

    /* create sack and add sources */
    if (priv->sack == NULL) {
        dnf_state_reset (priv->state);
        if (!dnf_context_setup_sack(context, priv->state, error))
            goto out;
    }

    subject = hy_subject_create(name);
    selector = hy_subject_get_best_selector(subject, priv->sack, NULL, FALSE, NULL);
    selector_matches = hy_selector_matches(selector);
    if (selector_matches->len == 0) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_PACKAGE_NOT_FOUND,
                    "No package matches '%s'", name);
        return FALSE;
    }

    if (!hy_goal_install_selector(priv->goal, selector, error))
        goto out;

    ret = TRUE;
 out:
    if (selector)
        hy_selector_free(selector);
    if (subject)
        hy_subject_free(subject);
    return ret;
}

/**
 * dnf_context_remove:
 * @context: a #DnfContext instance.
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
dnf_context_remove(DnfContext *context, const gchar *name, GError **error)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    GPtrArray *pkglist;
    hy_autoquery HyQuery query = NULL;
    gboolean ret = TRUE;
    guint i;

    /* create sack and add repos */
    if (priv->sack == NULL) {
        dnf_state_reset(priv->state);
        ret = dnf_context_setup_sack(context, priv->state, error);
        if (!ret)
            return FALSE;
    }

    /* find installed packages to remove */
    query = hy_query_create(priv->sack);
    hy_query_filter_latest_per_arch(query, TRUE);
    hy_query_filter(query, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
    hy_query_filter(query, HY_PKG_ARCH, HY_NEQ, "src");
    hy_query_filter(query, HY_PKG_NAME, HY_EQ, name);
    pkglist = hy_query_run(query);

    /* add each package */
    for (i = 0; i < pkglist->len; i++) {
        auto pkg = static_cast<DnfPackage *>(g_ptr_array_index(pkglist, i));
        hy_goal_erase(priv->goal, pkg);
    }
    g_ptr_array_unref(pkglist);
    return TRUE;
}

/**
 * dnf_context_update:
 * @context: a #DnfContext instance.
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
dnf_context_update(DnfContext *context, const gchar *name, GError **error)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);

    /* create sack and add repos */
    if (priv->sack == NULL) {
        dnf_state_reset(priv->state);
        if (!dnf_context_setup_sack(context, priv->state, error))
            return FALSE;
    }

    g_auto(HySubject) subject = hy_subject_create(name);
    g_auto(HySelector) selector = hy_subject_get_best_selector(subject, priv->sack, NULL, FALSE,
                                                               NULL);
    g_autoptr(GPtrArray) selector_matches = hy_selector_matches(selector);
    if (selector_matches->len == 0) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_PACKAGE_NOT_FOUND,
                    "No package matches '%s'", name);
        return FALSE;
    }

    if (hy_goal_upgrade_selector(priv->goal, selector))
        return FALSE;

    return TRUE;
}

/**
 * dnf_context_update_all:
 * @context: a #DnfContext instance.
 * @error: A #GError or %NULL
 *
 * Update all packages.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.7.0
 **/
gboolean
dnf_context_update_all (DnfContext  *context,
                        GError     **error)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);

    /* create sack and add repos */
    if (priv->sack == NULL) {
        dnf_state_reset(priv->state);
        if (!dnf_context_setup_sack(context, priv->state, error))
            return FALSE;
    }

    /* update whole solvables */
    hy_goal_upgrade_all (priv->goal);
    return TRUE;
}

/**
 * dnf_context_repo_set_data:
 **/
static gboolean
dnf_context_repo_set_data(DnfContext *context,
                          const gchar *repo_id,
                          DnfRepoEnabled enabled,
                          GError **error)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    DnfRepo *repo = NULL;
    guint i;

    /* find a repo with a matching ID */
    for (i = 0; i < priv->repos->len; i++) {
        auto repo_tmp = static_cast<DnfRepo *>(g_ptr_array_index(priv->repos, i));
        if (g_strcmp0(dnf_repo_get_id(repo_tmp), repo_id) == 0) {
            repo = repo_tmp;
            break;
        }
    }

    /* nothing found */
    if (repo == NULL) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "repo %s not found", repo_id);
        return FALSE;
    }

    /* this is runtime only */
    dnf_repo_set_enabled(repo, enabled);
    return TRUE;
}

/**
 * dnf_context_repo_enable:
 * @context: a #DnfContext instance.
 * @repo_id: A repo_id, e.g. "fedora-rawhide"
 * @error: A #GError or %NULL
 *
 * Enables a specific repo.
 *
 * This must be done before dnf_context_setup() is called.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_context_repo_enable(DnfContext *context,
                        const gchar *repo_id,
                        GError **error)
{
    return dnf_context_repo_set_data(context, repo_id,
                                     DNF_REPO_ENABLED_PACKAGES |
                                     DNF_REPO_ENABLED_METADATA, error);
}

/**
 * dnf_context_repo_disable:
 * @context: a #DnfContext instance.
 * @repo_id: A repo_id, e.g. "fedora-rawhide"
 * @error: A #GError or %NULL
 *
 * Disables a specific repo.
 *
 * This must be done before dnf_context_setup() is called.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_context_repo_disable(DnfContext *context,
                         const gchar *repo_id,
                         GError **error)
{
    return dnf_context_repo_set_data(context, repo_id,
                                     DNF_REPO_ENABLED_NONE, error);
}

/**
 * dnf_context_commit:(skip)
 * @context: a #DnfContext instance.
 * @state: A #DnfState
 * @error: A #GError or %NULL
 *
 * Commits a context, which applies changes to the live system.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_context_commit(DnfContext *context, DnfState *state, GError **error)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);

    /* ensure transaction exists */
    dnf_context_ensure_transaction(context);

    /* run the transaction */
    return dnf_transaction_commit(priv->transaction,
                                  priv->goal,
                                  state,
                                  error);
}

/**
 * dnf_context_invalidate_full:
 * @context: a #DnfContext instance.
 * @message: the reason for invalidating
 * @flags: a #DnfContextInvalidateFlags, e.g. %DNF_CONTEXT_INVALIDATE_FLAG_ENROLLMENT
 *
 * Informs the context that the certain parts of the context may no longer be
 * in sync or valid.
 *
 * Since: 0.2.1
 **/
void
dnf_context_invalidate_full(DnfContext *context,
                 const gchar *message,
                 DnfContextInvalidateFlags flags)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_debug("Msg: %s", message);
    if (flags & DNF_CONTEXT_INVALIDATE_FLAG_RPMDB)
        g_signal_emit(context, signals [SIGNAL_INVALIDATE], 0, message);
    if (flags & DNF_CONTEXT_INVALIDATE_FLAG_ENROLLMENT)
        priv->enrollment_valid = FALSE;
}

/**
 * dnf_context_invalidate:
 * @context: a #DnfContext instance.
 * @message: the reason for invalidating
 *
 * Emits a signal which signals that the context is invalid.
 *
 * Since: 0.1.0
 **/
void
dnf_context_invalidate(DnfContext *context, const gchar *message)
{
    dnf_context_invalidate_full(context, message,
                                DNF_CONTEXT_INVALIDATE_FLAG_RPMDB);
}

/**
 * dnf_context_clean_cache:
 * @context: a #DnfContext instance.
 * @flags: #DnfContextCleanFlags flag, e.g. %DNF_CONTEXT_CLEAN_EXPIRE_CACHE
 * @error: a #GError instance
 *
 * Clean the cache content in the current cache directory based
 * on the context flags. A valid cache directory and lock directory
 * is expected to be set prior to using this function. Use it with care.
 *
 * Currently support four different clean flags:
 * 1: DNF_CONTEXT_CLEAN_EXPIRE_CACHE: Elminate the entries that give information about cache entries' age. i.e: 'repomd.xml' will be deleted in libdnf case
 * 2: DNF_CONTEXT_CLEAN_PACKAGES: Eliminate any cached packages. i.e: 'packages' folder will be deleted
 * 3: DNF_CONTEXT_CLEAN_METADATA: Eliminate all of the files which libdnf uses to determine remote availability of packages. i.e: 'repodata'folder and 'metalink.xml' will be deleted
 * 4: DNF_CONTEXT_CLEAN_ALL: Does all the actions above and clean up other files that are generated due to various reasons. e.g: cache directories from previous version of operating system
 *
 * Note: when DNF_CONTEXT_CLEAN_ALL flag is seen, the other flags will be ignored
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.9.4
 **/
gboolean
dnf_context_clean_cache(DnfContext *context,
                        DnfContextCleanFlags flags,
                        GError **error)
{
    g_autoptr(GPtrArray) suffix_list = g_ptr_array_new();
    const gchar* directory_location;
    gboolean ret;
    guint lock_id = 0;

    /* Set up the context if it hasn't been set earlier */
    if (!dnf_context_setup(context, NULL, error))
        return FALSE;

    DnfContextPrivate *priv = GET_PRIVATE(context);
    /* We expect cache directories to be set when cleaning cache entries */
    if (priv->cache_dir == NULL) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_INTERNAL_ERROR,
                            "No cache dir set");
        return FALSE;
    }

    /* When clean all flags show up, we remove everything from cache directory */
    if (flags & DNF_CONTEXT_CLEAN_ALL) {
        return dnf_remove_recursive(priv->cache_dir, error);
    }

    /* We acquire the metadata related lock */
    lock_id = dnf_lock_take(priv->lock,
                            DNF_LOCK_TYPE_METADATA,
                            DNF_LOCK_MODE_PROCESS,
                            error);
    if (lock_id == 0)
        return FALSE;

    /* After the above setup is done, we prepare file extensions based on flag types */
    if (flags & DNF_CONTEXT_CLEAN_PACKAGES)
        g_ptr_array_add(suffix_list, (char*) "packages");
    if (flags & DNF_CONTEXT_CLEAN_METADATA) {
        g_ptr_array_add(suffix_list, (char*) "metalink.xml");
        g_ptr_array_add(suffix_list, (char*) "repodata");
    }
    if (flags & DNF_CONTEXT_CLEAN_EXPIRE_CACHE)
        g_ptr_array_add(suffix_list, (char*) "repomd.xml");

    /* Add a NULL terminator for future looping */
    g_ptr_array_add(suffix_list, NULL);

    /* We then start looping all of the repos to perform file deletion */
    for (guint counter = 0; counter < priv->repos->len; counter++) {
        auto src = static_cast<DnfRepo *>(g_ptr_array_index(priv->repos, counter));
        gboolean deleteable_repo = dnf_repo_get_kind(src) == DNF_REPO_KIND_REMOTE;
        directory_location = dnf_repo_get_location(src);

        /* We check if the repo is qualified to be cleaned */
        if (deleteable_repo &&
            g_file_test(directory_location, G_FILE_TEST_EXISTS)) {
            ret = dnf_delete_files_matching(directory_location,
                                            (const char* const*) suffix_list->pdata,
                                            error);
            if(!ret)
                goto out;
            }
        }

    out:
        /* release the acquired lock */
        if (!dnf_lock_release(priv->lock, lock_id, error))
            return FALSE;
        return ret;
}
/**
 * dnf_context_new:
 *
 * Creates a new #DnfContext.
 *
 * Returns: (transfer full): a #DnfContext
 *
 * Since: 0.1.0
 **/
DnfContext *
dnf_context_new(void)
{
    return DNF_CONTEXT(g_object_new(DNF_TYPE_CONTEXT, NULL));
}
