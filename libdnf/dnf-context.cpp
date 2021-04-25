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
 * This object is a high level interface that does not allow the user
 * to use objects from librepo, rpm or hawkey directly.
 */

#include "config.h"
#include "conf/Const.hpp"
#include "dnf-context.hpp"
#include "libdnf/conf/ConfigParser.hpp"
#include "conf/Option.hpp"
#include "bgettext/bgettext-lib.h"
#include "tinyformat/tinyformat.hpp"
#include "goal/Goal.hpp"

#include <memory>
#include <set>
#include <vector>
#include <unordered_set>
#include <gio/gio.h>
#include <rpm/rpmlib.h>
#include <rpm/rpmmacro.h>
#include <rpm/rpmts.h>
#include <rpm/rpmdb.h>
#include <librepo/librepo.h>
#ifdef RHSM_SUPPORT
#include <rhsm/rhsm.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#include <fnmatch.h>
#include <unistd.h>

#include "catch-error.hpp"
#include "log.hpp"
#include "tinyformat/tinyformat.hpp"
#include "dnf-lock.h"
#include "dnf-package.h"
#include "dnf-repo-loader.h"
#include "dnf-sack-private.hpp"
#include "dnf-state.h"
#include "dnf-transaction.h"
#include "dnf-utils.h"
#include "dnf-sack.h"
#include "hy-query.h"
#include "hy-subject.h"
#include "hy-selector.h"
#include "dnf-repo.hpp"
#include "goal/Goal.hpp"
#include "plugin/plugin-private.hpp"
#include "utils/GLibLogger.hpp"
#include "utils/os-release.hpp"


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
    { "armhfp",     { "armv6hl", "armv7hl", "armv7hnl", "armv8hl", "armv8hnl", "armv8hcnl", NULL } },
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
    { "riscv32",    { "riscv32", NULL } },
    { "riscv64",    { "riscv64", NULL } },
    { "riscv128",   { "riscv128", NULL } },
    { "s390",       { "s390", NULL } },
    { "s390x",      { "s390x", NULL } },
    { "sh3",        { "sh3", NULL } },
    { "sh4",        { "sh4", "sh4a", NULL } },
    { "sparc",      { "sparc", "sparc64", "sparc64v", "sparcv8",
                      "sparcv9", "sparcv9v", NULL } },
    { "x86_64",     { "x86_64", "amd64", "ia32e", NULL } },
    { NULL,         { NULL } }
};

const gchar *
find_base_arch(const char *native) {
    for (int i = 0; arch_map[i].base != NULL; i++) {
        for (int j = 0; arch_map[i].native[j] != NULL; j++) {
            if (g_strcmp0(arch_map[i].native[j], native) == 0) {
                return arch_map[i].base;
            }
        }
    }
    return NULL;
}

typedef struct
{
    gchar            **repos_dir;
    gchar            **vars_dir;
    gchar            **installonlypkgs;
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
    gchar            **native_arches{NULL};
    gchar            *http_proxy;
    gchar            *user_agent;
    gchar            *arch;
    guint            cache_age;     /*seconds*/
    gboolean         cacheOnly{false};
    gboolean         check_disk_space;
    gboolean         check_transaction;
    gboolean         only_trusted;
    gboolean         enable_filelists;
    gboolean         enrollment_valid;
    gboolean         write_history;
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
    std::map<std::string, std::string> *vars;
    bool             varsCached;
    libdnf::Plugins *plugins;
} DnfContextPrivate;

struct PluginHookContextInitData : public libdnf::PluginInitData {
    PluginHookContextInitData(PluginMode mode, DnfContext * context)
    : PluginInitData(mode), context(context) {}

    DnfContext * context;
};

enum {
    SIGNAL_INVALIDATE,
    SIGNAL_LAST
};

static libdnf::GLibLogger glibLogger(G_LOG_DOMAIN);
static std::string pluginsDir = DEFAULT_PLUGINS_DIRECTORY;
static std::unique_ptr<std::string> configFilePath;
static std::set<std::string> pluginsEnabled;
static std::set<std::string> pluginsDisabled;
static guint signals [SIGNAL_LAST] = { 0 };

#ifdef RHSM_SUPPORT
static bool disableInternalRhsmPlugin = false;
#endif

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

    priv->plugins->free();
    delete priv->plugins;
    delete priv->vars;

    g_strfreev(priv->repos_dir);
    g_strfreev(priv->vars_dir);
    g_strfreev(priv->installonlypkgs);
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

static void
dnf_context_plugins_disable_enable(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);

    // apply pluginsDisabled and pluginsEnabled
    std::set<std::string> patternDisableFound;
    std::set<std::string> patternEnableFound;
    for (size_t i = 0; i < priv->plugins->count(); ++i) {
        auto pluginInfo = priv->plugins->getPluginInfo(i);
        bool enabled = true;
        for (const auto & patternSkip : pluginsDisabled) {
            if (fnmatch(patternSkip.c_str(), pluginInfo->name, 0) == 0) {
                enabled = false;
                patternDisableFound.insert(patternSkip);
            }
        }
        for (const auto & patternEnable : pluginsEnabled) {
            if (fnmatch(patternEnable.c_str(), pluginInfo->name, 0) == 0) {
                enabled = true;
                patternEnableFound.insert(patternEnable);
            }
        }
        if (!enabled) {
            priv->plugins->enablePlugin(i, false);
        }
    }

#ifdef RHSM_SUPPORT
    // apply pluginsDisabled and pluginsEnabled to the internal RHSM plugin
    const char * RhsmPluginName = "subscription-manager";
    for (const auto & patternSkip : pluginsDisabled) {
        if (fnmatch(patternSkip.c_str(), RhsmPluginName, 0) == 0) {
            disableInternalRhsmPlugin = true;
            patternDisableFound.insert(patternSkip);
        }
    }
    for (const auto & patternEnable : pluginsEnabled) {
        if (fnmatch(patternEnable.c_str(), RhsmPluginName, 0) == 0) {
            disableInternalRhsmPlugin = false;
            patternEnableFound.insert(patternEnable);
        }
    }
#endif

    // Log non matched disable patterns
    std::string nonMatched;
    for (const auto & pattern : pluginsDisabled) {
        if (patternDisableFound.count(pattern) == 0) {
            if (nonMatched.length() > 0) {
                nonMatched += ", ";
            }
            nonMatched += pattern;
        }
    }
    if (!nonMatched.empty()) {
        g_warning("No matches found for the following disable plugin patterns: %s", nonMatched.c_str());
    }

    // Log non matched enable patterns
    nonMatched.clear();
    for (const auto & pattern : pluginsEnabled) {
        if (patternEnableFound.count(pattern) == 0) {
            if (nonMatched.length() > 0) {
                nonMatched += ", ";
            }
            nonMatched += pattern;
        }
    }
    if (!nonMatched.empty()) {
        g_warning("No matches found for the following enable plugin patterns: %s", nonMatched.c_str());
    }
}

/**
 * dnf_context_init:
 **/
static void
dnf_context_init(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);

    libdnf::Log::setLogger(&glibLogger);

    priv->install_root = g_strdup("/");
    priv->check_disk_space = TRUE;
    priv->check_transaction = TRUE;
    priv->enable_filelists = TRUE;
    priv->write_history = TRUE;
    priv->state = dnf_state_new();
    priv->lock = dnf_lock_new();
    priv->cache_age = 60 * 60 * 24 * 7; /* 1 week */
    priv->override_macros = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                  g_free, g_free);
    priv->user_agent = g_strdup(libdnf::getUserAgent().c_str());

    priv->vars = new std::map<std::string, std::string>;

    priv->plugins = new libdnf::Plugins;

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
dnf_context_globals_init (GError **error) try
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
} CATCH_TO_GERROR(FALSE)

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
 * dnf_context_get_config_file_path:
 *
 * Gets the path to the global configuration file.
 *
 * Returns: Path to the global configuration file.
 *
 * Since: 0.42.0
 **/
const gchar *
dnf_context_get_config_file_path()
{
    return configFilePath ? configFilePath->c_str() : libdnf::CONF_FILENAME;
}

/**
 * dnf_context_is_set_config_file_path:
 *
 * Gets state of config_file_path.
 *
 * Returns: TRUE if config_file_path is set, FALSE if default is used.
 *
 * Since: 0.44.1
 **/
gboolean
dnf_context_is_set_config_file_path()
{
    return configFilePath != nullptr;
}

/**
 * dnf_context_get_repos_dir:
 * @context: a #DnfContext instance.
 *
 * Gets NULL terminated array of paths to the repositories directories.
 *
 * Returns: the NULL terminated array of paths, e.g. ["/etc/yum.repos.d", NULL]
 *
 * Since: 0.42.0
 **/
const gchar * const *
dnf_context_get_repos_dir(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    if (!priv->repos_dir) {
        auto & reposDir = libdnf::getGlobalMainConfig().reposdir().getValue();
        priv->repos_dir = g_new(gchar*, reposDir.size() + 1);
        for (size_t i = 0; i < reposDir.size(); ++i)
            priv->repos_dir[i] = g_strdup(reposDir[i].c_str());
        priv->repos_dir[reposDir.size()] = NULL;
    }
    return priv->repos_dir;
}

/**
 * dnf_context_get_repo_dir:
 * @context: a #DnfContext instance.
 *
 * Gets the repo directory.
 *
 * Returns: the path to repo directory, e.g. "/etc/yum.repos.d"
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_context_get_repo_dir(DnfContext *context)
{
    static std::string reposDirStr;
    DnfContextPrivate *priv = GET_PRIVATE(context);
    dnf_context_get_repos_dir(context); // ensure retrieving of the value from the global config
    reposDirStr = priv->repos_dir[0] ? priv->repos_dir[0] : "";
    return reposDirStr.c_str();
}

/**
 * dnf_context_get_vars_dir:
 * @context: a #DnfContext instance.
 *
 * Gets the repo variables directories.
 *
 * Returns: the NULL terminated array of directories, e.g. ["/etc/dnf/vars", NULL]
 *
 * Since: 0.28.1
 **/
const gchar * const *
dnf_context_get_vars_dir(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    if (!priv->vars_dir) {
        auto & varsDir = libdnf::getGlobalMainConfig().varsdir().getValue();
        priv->vars_dir = g_new(gchar*, varsDir.size() + 1);
        for (size_t i = 0; i < varsDir.size(); ++i)
            priv->vars_dir[i] = g_strdup(varsDir[i].c_str());
        priv->vars_dir[varsDir.size()] = NULL;
    }
    return priv->vars_dir;
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
    const char *value;

    if (priv->base_arch)
        return priv->base_arch;

    /* get info from RPM */
    rpmGetOsInfo(&value, NULL);
    priv->os_info = g_strdup(value);
    rpmGetArchInfo(&value, NULL);
    priv->arch_info = g_strdup(value);
    priv->base_arch = g_strdup(find_base_arch(value));

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
 * dnf_context_get_cache_only:
 * @context: a #DnfContext instance.
 *
 * Gets cache only mode status.
 *
 * Returns: %TRUE if cache only mode is enabled
 *
 * Since: 0.21.0
 **/
gboolean
dnf_context_get_cache_only(DnfContext * context)
{
    auto priv = GET_PRIVATE(context);
    return priv->cacheOnly;
}

/**
 * dnf_context_get_best:
 *
 * Gets best global configuration value.
 *
 * Returns: %TRUE if best mode is enabled
 *
 * Since: 0.37.0
 **/
gboolean
dnf_context_get_best()
{
    auto & mainConf = libdnf::getGlobalMainConfig();
    return mainConf.best().getValue();
}

/**
 * dnf_context_get_install_weak_deps:
 *
 * Gets install_weak_deps global configuration value.
 *
 * Returns: %TRUE if installation of weak dependencies is allowed
 *
 * Since: 0.40.0
 */
gboolean
dnf_context_get_install_weak_deps()
{
    auto & mainConf = libdnf::getGlobalMainConfig();
    return mainConf.install_weak_deps().getValue();
}

/**
 * dnf_context_get_allow_vendor_change:
 *
 * Gets allow_vendor_change global configuration value.
 *
 * Returns: %TRUE if changing vendors in a transaction is allowed
 *
 * Since: 0.55.2
 */
gboolean
dnf_context_get_allow_vendor_change()
{
    auto & mainConf = libdnf::getGlobalMainConfig();
    return mainConf.allow_vendor_change().getValue();
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
    auto & mainConf = libdnf::getGlobalMainConfig();
    return mainConf.keepcache().getValue();
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
 * dnf_context_get_zchunk
 * @context: a #DnfContext instance is not used. It is global option.
 *
 * Gets whether zchunk is enabled
 *
 * Returns: %TRUE if zchunk is enabled
 *
 * Since: 0.21.0
 **/
gboolean
dnf_context_get_zchunk(DnfContext *context)
{
    auto & mainConf = libdnf::getGlobalMainConfig();
    return mainConf.zchunk().getValue();
}

/**
 * dnf_context_get_write_history
 * @context: a #DnfContext instance.
 *
 * Gets whether writing to history database is enabled.
 *
 * Returns: %TRUE if writing to history database is enabled
 *
 * Since: 0.27.2
 **/
gboolean
dnf_context_get_write_history(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->write_history;
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
 * The return value is valid until the value of the global configuration "installonlypkgs" changes.
 * E.g. using dnf_conf_main_set_option() or dnf_conf_add_setopt().
 *
 * Returns: (transfer none): array of package names
 */
const gchar **
dnf_context_get_installonly_pkgs(DnfContext *context)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    auto & mainConf = libdnf::getGlobalMainConfig();
    auto & packages = mainConf.installonlypkgs().getValue();

    // If "installonlypkgs" is not initialized (first run), set "differs" to true.
    bool differs = !priv->installonlypkgs;

    // Test if they are not different.
    if (!differs) {
        size_t i = 0;
        while (i < packages.size()) {
            if (!priv->installonlypkgs[i] || packages[i].compare(priv->installonlypkgs[i]) != 0) {
                differs = true;
                break;
            }
            ++i;
        }
        if (priv->installonlypkgs[i]) {
            differs = true;
        }
    }

    // Re-initialize "installonlypkgs" only if it differs from the values in mainConf.
    if (differs) {
        g_strfreev(priv->installonlypkgs);
        priv->installonlypkgs = g_new0(gchar*, packages.size() + 1);

        for (size_t i = 0; i < packages.size(); ++i) {
            priv->installonlypkgs[i] = g_strdup(packages[i].c_str());
        }
    }

    return const_cast<const gchar **>(priv->installonlypkgs);
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
    auto & mainConf = libdnf::getGlobalMainConfig();
    return mainConf.installonly_limit().getValue();
}

/**
 * dnf_context_set_config_file_path:
 * @config_file_path: the path, e.g. "/etc/dnf/dnf.conf"
 *
 * Sets the path to the global configuration file. The empty string means no read configuration
 * file. NULL resets the path to the default value. Must be used before
 * dnf_context_get_*, and dnf_context_setup functions are called.
 *
 * Since: 0.42.0
 **/
void
dnf_context_set_config_file_path(const gchar * config_file_path)
{
    if (config_file_path) {
        configFilePath.reset(new std::string(config_file_path));
    } else {
        configFilePath.reset();
    }
}

/**
 * dnf_context_set_repos_dir:
 * @context: a #DnfContext instance.
 * @repos_dir: the NULL terminated array of paths, e.g. ["/etc/yum.repos.d", NULL]
 *
 * Sets the repositories directories.
 *
 * Since: 0.42.0
 **/
void
dnf_context_set_repos_dir(DnfContext *context, const gchar * const *repos_dir)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_strfreev(priv->repos_dir);
    if (repos_dir) {
        guint len = 1;
        for (auto iter = repos_dir; *iter; ++iter) {
            ++len;
        }
        priv->repos_dir = g_new(gchar*, len);
        for (guint i = 0; i < len; ++i) {
            priv->repos_dir[i] = g_strdup(repos_dir[i]);
        }
    } else {
        priv->repos_dir = NULL;
    }
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
    g_strfreev(priv->repos_dir);
    if (repo_dir) {
        priv->repos_dir = g_new0(gchar*, 2);
        priv->repos_dir[0] = g_strdup(repo_dir);
    } else {
        priv->repos_dir = NULL;
    }
}

/**
 * dnf_context_set_vars_dir:
 * @context: a #DnfContext instance.
 * @vars_dir: the vars directories, e.g. ["/etc/dnf/vars"]
 *
 * Sets the repo variables directory.
 *
 * Since: 0.28.1
 **/
void
dnf_context_set_vars_dir(DnfContext *context, const gchar * const *vars_dir)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    g_strfreev(priv->vars_dir);
    priv->vars_dir = g_strdupv(const_cast<gchar **>(vars_dir));
    priv->varsCached = false;
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
    g_free(priv->arch);
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
 * dnf_context_set_best:
 * @gboolean: a value for best configuration
 *
 * Sets best global configuration value.
 *
 * Since: 0.37.0
 **/
void
dnf_context_set_best(gboolean best)
{
    auto & mainConf = libdnf::getGlobalMainConfig(false);
    mainConf.best().set(libdnf::Option::Priority::RUNTIME, best);
}

/**
 * dnf_context_set_install_weak_deps:
 *
 * Sets install_weak_deps global configuration value.
 *
 * Since: 0.40.0
 */
void
dnf_context_set_install_weak_deps(gboolean enabled)
{
    auto & mainConf = libdnf::getGlobalMainConfig(false);
    mainConf.install_weak_deps().set(libdnf::Option::Priority::RUNTIME, enabled);
}

/**
 * dnf_context_set_allow_vendor_change:
 *
 * Sets allow_vendor_change global configuration value.
 *
 * Since: 0.55.2
 */
void
dnf_context_set_allow_vendor_change(gboolean vendorchange)
{
    auto & mainConf = libdnf::getGlobalMainConfig(false);
    mainConf.allow_vendor_change().set(libdnf::Option::Priority::RUNTIME, vendorchange);
}

/**
 * dnf_context_set_cache_only:
 * @context: a #DnfContext instance.
 * @cache_only: %TRUE to use only metadata from cache
 *
 * Enables or disables cache only mode.
 *
 * Since: 0.21.0
 **/
void
dnf_context_set_cache_only(DnfContext * context, gboolean cache_only)
{
    auto priv = GET_PRIVATE(context);
    priv->cacheOnly = cache_only;
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
    auto & mainConf = libdnf::getGlobalMainConfig(false);
    mainConf.keepcache().set(libdnf::Option::Priority::RUNTIME, keep_cache);
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
 * dnf_context_set_zchunk:
 * @context: a #DnfContext instance is not used. It is global option.
 * @only_trusted: %TRUE use zchunk metadata if available
 *
 * Enables or disables the use of zchunk metadata if available.
 *
 * Since: 0.21.0
 **/
void
dnf_context_set_zchunk(DnfContext *context, gboolean zchunk)
{
    auto & mainConf = libdnf::getGlobalMainConfig(false);
    mainConf.zchunk().set(libdnf::Option::Priority::RUNTIME, zchunk);
}

/**
 * dnf_context_set_write_history:
 * @context: a #DnfContext instance.
 * @value: %TRUE write history database
 *
 * Enables or disables writing to history database.
 *
 * Since: 0.27.2
 **/
void
dnf_context_set_write_history(DnfContext *context, gboolean value)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    priv->write_history = value;
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
dnf_context_set_os_release(DnfContext *context, GError **error) try
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
} CATCH_TO_GERROR(FALSE)

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
dnf_context_setup_sack(DnfContext *context, DnfState *state, GError **error) try
{
    return dnf_context_setup_sack_with_flags(context, state,
                                             DNF_CONTEXT_SETUP_SACK_FLAG_NONE, error);
} CATCH_TO_GERROR(FALSE)

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
                                  GError                  **error) try
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    gboolean ret;
    g_autofree gchar *solv_dir_real = nullptr;
    gboolean vendorchange;

    /* create empty sack */
    solv_dir_real = dnf_realpath(priv->solv_dir);
    vendorchange = dnf_context_get_allow_vendor_change();
    priv->sack = dnf_sack_new();
    dnf_sack_set_cachedir(priv->sack, solv_dir_real);
    dnf_sack_set_rootdir(priv->sack, priv->install_root);
    dnf_sack_set_allow_vendor_change(priv->sack, vendorchange);
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
                                       DNF_SACK_LOAD_FLAG_NONE,
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
        try {
            dnf_sack_filter_modules_v2(sack, nullptr, hotfixRepos.data(), priv->install_root,
                priv->platform_module, false, false, false);
        } catch (libdnf::ModulePackageContainer::ConflictException & exception) {
            g_set_error(error, DNF_ERROR, DNF_ERROR_FAILED, "%s", exception.what());
            return FALSE;
        }
    }

    /* create goal */
    if (priv->goal != nullptr)
        hy_goal_free(priv->goal);
    priv->goal = hy_goal_create(priv->sack);
    return TRUE;
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_context_ensure_exists:
 **/
static gboolean
dnf_context_ensure_exists(const gchar *directory, GError **error) try
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
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_utils_copy_files:
 */
static gboolean
dnf_utils_copy_files(const gchar *src, const gchar *dest, GError **error) try
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
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_context_copy_vendor_cache:
 *
 * The vendor cache is typically structured like this:
 * /usr/share/PackageKit/metadata/fedora/repodata/primary.xml.gz
 * /usr/share/PackageKit/metadata/updates/repodata/primary.xml.gz
 **/
static gboolean
dnf_context_copy_vendor_cache(DnfContext *context, GError **error) try
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
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_context_copy_vendor_solv:
 *
 * The solv cache is typically structured like this:
 * /usr/share/PackageKit/hawkey/@System.solv
 * /usr/share/PackageKit/hawkey/fedora-filenames.solvx
 **/
static gboolean
dnf_context_copy_vendor_solv(DnfContext *context, GError **error) try
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
} CATCH_TO_GERROR(FALSE)

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
dnf_context_setup_enrollments(DnfContext *context, GError **error) try
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
    /* Do not use the "hardcoded plugin" if this plugin or all plugins are disabled. */
    if (disableInternalRhsmPlugin || !libdnf::getGlobalMainConfig().plugins().getValue()) {
        return TRUE;
    }

    /* If /var/lib/rhsm exists, then we can assume that
     * subscription-manager is installed, and thus don't need to
     * manage redhat.repo via librhsm.
     */
    if (access("/var/lib/rhsm", F_OK) == 0) {
        return TRUE;
    }
    g_autoptr(RHSMContext) rhsm_ctx = rhsm_context_new ();
    auto repo_dir = dnf_context_get_repo_dir(context);
    g_autofree gchar *repofname = g_build_filename (repo_dir,
                                                    "redhat.repo",
                                                    NULL);
    g_autoptr(GKeyFile) repofile = rhsm_utils_yum_repo_from_context (rhsm_ctx);

    bool sameContent = false;
    do {
        int fd;
        if ((fd = open(repofname, O_RDONLY)) == -1)
            break;
        gsize length;
        g_autofree gchar *data = g_key_file_to_data(repofile, &length, NULL);
        auto fileLen = lseek(fd, 0, SEEK_END);
        if (fileLen != static_cast<long>(length)) {
            close(fd);
            break;
        }
        if (fileLen > 0) {
            std::unique_ptr<char[]> buf(new char[fileLen]);
            lseek(fd, 0, SEEK_SET);
            auto readed = read(fd, buf.get(), fileLen);
            close(fd);
            if (readed != fileLen || memcmp(buf.get(), data, readed) != 0)
                break;
        } else {
            close(fd);
        }
        sameContent = true;
    } while (false);

    if (!sameContent) {
        if (!g_key_file_save_to_file(repofile, repofname, error))
            return FALSE;
    }
#endif

    priv->enrollment_valid = TRUE;
    return TRUE;
} CATCH_TO_GERROR(FALSE)

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
           GError **error) try
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    guint i;
    guint j;
    GHashTableIter hashiter;
    gpointer hashkey, hashval;
    g_autoptr(GString) buf = NULL;
    g_autofree char *rpmdb_path = NULL;
    g_autoptr(GFile) file_rpmdb = NULL;

    if (libdnf::getGlobalMainConfig().plugins().getValue() && !pluginsDir.empty()) {
        priv->plugins->loadPlugins(pluginsDir);
    }
    dnf_context_plugins_disable_enable(context);
    PluginHookContextInitData initData(PLUGIN_MODE_CONTEXT, context);
    priv->plugins->init(PLUGIN_MODE_CONTEXT, &initData);

    if (!dnf_context_plugin_hook(context, PLUGIN_HOOK_ID_CONTEXT_PRE_CONF, nullptr, nullptr))
        return FALSE;

    /* check essential things are set */
    if (priv->solv_dir == NULL) {
        g_set_error_literal(error,
                     DNF_ERROR,
                     DNF_ERROR_INTERNAL_ERROR,
                     "solv_dir not set");
        return FALSE;
    }

    /* ensure directories exist */
    auto repo_dir = dnf_context_get_repo_dir(context);
    if (repo_dir[0]) {
        if (!dnf_context_ensure_exists(repo_dir, error))
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
    if (!priv->native_arches) {
        priv->native_arches = g_new0(gchar *, MAX_NATIVE_ARCHES);
        for (i = 0; arch_map[i].base != NULL; i++) {
            if (g_strcmp0(arch_map[i].base, priv->base_arch) == 0) {
                for (j = 0; arch_map[i].native[j] != NULL; j++)
                    priv->native_arches[j] = g_strdup(arch_map[i].native[j]);
                priv->native_arches[j++] = g_strdup("noarch");
                break;
            }
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

    if (!dnf_context_plugin_hook(context, PLUGIN_HOOK_ID_CONTEXT_CONF, nullptr, nullptr))
        return FALSE;

    return TRUE;
} CATCH_TO_GERROR(FALSE)

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
dnf_context_run(DnfContext *context, GCancellable *cancellable, GError **error) try
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
} CATCH_TO_GERROR(FALSE)

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
dnf_context_install (DnfContext *context, const gchar *name, GError **error) try
{
    DnfContextPrivate *priv = GET_PRIVATE (context);
    g_autoptr(GPtrArray) selector_matches = NULL;

    /* create sack and add sources */
    if (priv->sack == NULL) {
        dnf_state_reset (priv->state);
        if (!dnf_context_setup_sack(context, priv->state, error))
            return FALSE;
    }

    g_auto(HySubject) subject = hy_subject_create(name);
    g_auto(HySelector) selector = hy_subject_get_best_selector(subject, priv->sack, NULL, FALSE, NULL);
    selector_matches = hy_selector_matches(selector);
    if (selector_matches->len == 0) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_PACKAGE_NOT_FOUND,
                    "No package matches '%s'", name);
        return FALSE;
    }

    if (!hy_goal_install_selector(priv->goal, selector, error))
        return FALSE;

    return TRUE;
} CATCH_TO_GERROR(FALSE)

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
dnf_context_remove(DnfContext *context, const gchar *name, GError **error) try
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
    query->installed();
    hy_query_filter(query, HY_PKG_NAME, HY_EQ, name);
    pkglist = hy_query_run(query);

    /* add each package */
    for (i = 0; i < pkglist->len; i++) {
        auto pkg = static_cast<DnfPackage *>(g_ptr_array_index(pkglist, i));
        hy_goal_erase(priv->goal, pkg);
    }
    g_ptr_array_unref(pkglist);
    return TRUE;
} CATCH_TO_GERROR(FALSE)

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
dnf_context_update(DnfContext *context, const gchar *name, GError **error) try
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
} CATCH_TO_GERROR(FALSE)

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
                        GError     **error) try
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
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_context_distrosync:
 * @context: a #DnfContext instance.
 * @name: A package or group name, e.g. "firefox" or "@gnome-desktop"
 * @error: A #GError or %NULL
 *
 * Finds an installed and remote package and marks it to be synchronized with remote version.
 *
 * If multiple packages are available then the newest package is synchronized to.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.62.0
 **/
gboolean
dnf_context_distrosync(DnfContext *context, const gchar *name, GError **error) try
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

    if (hy_goal_distupgrade_selector(priv->goal, selector))
        return FALSE;

    return TRUE;
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_context_distrosync_all:
 * @context: a #DnfContext instance.
 * @error: A #GError or %NULL
 *
 * Distro-sync all packages.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.62.0
 **/
gboolean
dnf_context_distrosync_all (DnfContext  *context,
                        GError     **error) try
{
    DnfContextPrivate *priv = GET_PRIVATE(context);

    /* create sack and add repos */
    if (priv->sack == NULL) {
        dnf_state_reset(priv->state);
        if (!dnf_context_setup_sack(context, priv->state, error))
            return FALSE;
    }

    /* distrosync whole solvables */
    hy_goal_distupgrade_all (priv->goal);
    return TRUE;
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_context_repo_set_data:
 **/
static gboolean
dnf_context_repo_set_data(DnfContext *context,
                          const gchar *repo_id,
                          DnfRepoEnabled enabled,
                          GError **error) try
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    bool found = false;

    /* set repos with a matching ID */
    for (guint i = 0; i < priv->repos->len; ++i) {
        auto repo = static_cast<DnfRepo *>(g_ptr_array_index(priv->repos, i));
        if (fnmatch(repo_id, dnf_repo_get_id(repo), 0) == 0) {
            dnf_repo_set_enabled(repo, enabled);
            found = true;
        }
    }

    /* nothing found */
    if (!found) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "repo %s not found", repo_id);
        return FALSE;
    }

    return TRUE;
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_context_repo_enable:
 * @context: a #DnfContext instance.
 * @repo_id: A repo_id, e.g. "fedora-rawhide"
 * @error: A #GError or %NULL
 *
 * Enables a specific repo(s).
 * Wildcard pattern is supported.
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
                        GError **error) try
{
    return dnf_context_repo_set_data(context, repo_id,
                                     DNF_REPO_ENABLED_PACKAGES |
                                     DNF_REPO_ENABLED_METADATA, error);
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_context_repo_disable:
 * @context: a #DnfContext instance.
 * @repo_id: A repo_id, e.g. "fedora-rawhide"
 * @error: A #GError or %NULL
 *
 * Disables a specific repo(s).
 * Wildcard pattern is supported.
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
                         GError **error) try
{
    return dnf_context_repo_set_data(context, repo_id,
                                     DNF_REPO_ENABLED_NONE, error);
} CATCH_TO_GERROR(FALSE)

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
dnf_context_commit(DnfContext *context, DnfState *state, GError **error) try
{
    DnfContextPrivate *priv = GET_PRIVATE(context);

    /* ensure transaction exists */
    dnf_context_ensure_transaction(context);

    /* run the transaction */
    return dnf_transaction_commit(priv->transaction,
                                  priv->goal,
                                  state,
                                  error);
} CATCH_TO_GERROR(FALSE)

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
 * 1: DNF_CONTEXT_CLEAN_EXPIRE_CACHE: Eliminate the entries that give information about cache entries' age. i.e: 'repomd.xml' will be deleted in libdnf case
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
                        GError **error) try
{
    g_autoptr(GPtrArray) suffix_list = g_ptr_array_new();
    const gchar* directory_location;
    gboolean ret = TRUE;
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
} CATCH_TO_GERROR(FALSE)
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

/**
 * dnf_context_disable_plugins:
 * @plugin_name: a name pattern of plugins to disable.
 *
 * Adds name pattern into list of disabled plugins.
 * NULL or empty string means clear the list.
 * The list has lower priority than list of enabled plugins.
 *
 * Since: 0.41.0
 **/
void
dnf_context_disable_plugins(const gchar * plugin_name_pattern)
{
    if (!plugin_name_pattern || plugin_name_pattern[0] == '\0') {
        pluginsDisabled.clear();
    } else {
        pluginsDisabled.insert(plugin_name_pattern);
    }
}

/**
 * dnf_context_enable_plugins:
 * @plugin_name: a name pattern of plugins to enable.
 *
 * Adds name pattern into list of enabled plugins.
 * NULL or empty string means clear the list.
 * The list has higher priority than list of disabled plugins.
 *
 * Since: 0.41.0
 **/
void
dnf_context_enable_plugins(const gchar * plugin_name_pattern)
{
    if (!plugin_name_pattern || plugin_name_pattern[0] == '\0') {
        pluginsEnabled.clear();
    } else {
        pluginsEnabled.insert(plugin_name_pattern);
    }
}

/**
 * dnf_context_get_disabled_plugins:
 *
 * List of name patterns of disabled plugins.
 * 
 * Returns: NULL terminated array. Caller must free it.
 *
 * Since: 0.41.0
 **/
gchar **
dnf_context_get_disabled_plugins()
{
    gchar ** ret = g_new0(gchar*, pluginsDisabled.size() + 1);
    size_t i = 0;
    for (auto & namePattern : pluginsDisabled) {
        ret[i++] = g_strdup(namePattern.c_str());
    }
    return ret;
}

/**
 * dnf_context_get_enabled_plugins:
 *
 * List of name patterns of enabled plugins.
 * 
 * Returns: NULL terminated array. Caller must free it.
 *
 * Since: 0.41.0
 **/
gchar **
dnf_context_get_enabled_plugins()
{
    gchar ** ret = g_new0(gchar*, pluginsEnabled.size() + 1);
    size_t i = 0;
    for (auto & namePattern : pluginsEnabled) {
        ret[i++] = g_strdup(namePattern.c_str());
    }
    return ret;
}

/**
 * dnf_context_get_plugins_all_disabled:
 * @context: a #DnfContext instance.
 *
 * Gets plugins global configuration value.
 *
 * Returns: %TRUE if plugins are disabled
 *
 * Since: 0.41.0
 **/
gboolean
dnf_context_get_plugins_all_disabled()
{
    auto & mainConf = libdnf::getGlobalMainConfig();
    return !mainConf.plugins().getValue();
}

/**
 * dnf_context_set_plugins_all_disabled:
 * @context: a #DnfContext instance.
 *
 * Disable or enable plugins.
 * Sets plugins global configuration value.
 * To take effect must be called before dnf_context_setup().
 *
 * Since: 0.41.0
 **/
void
dnf_context_set_plugins_all_disabled(gboolean disabled)
{
    auto & mainConf = libdnf::getGlobalMainConfig(false);
    mainConf.plugins().set(libdnf::Option::Priority::RUNTIME, !disabled);
}

/**
 * dnf_context_get_plugins_dir:
 * @context: a #DnfContext instance.
 *
 * Gets path to plugin directory.
 *
 * Returns: (transfer none): the plugins dir
 *
 * Since: 0.21.0
 **/
const char *
dnf_context_get_plugins_dir(DnfContext * context)
{
    return pluginsDir.c_str();
}

/**
 * dnf_context_set_plugins_dir:
 * @context: a #DnfContext instance.
 *
 * Sets path to plugin directory.
 * To take effect must be called before dnf_context_setup().
 * Empty path disable loading of plugins at all.
 *
 * Since: 0.21.0
 **/
void
dnf_context_set_plugins_dir(DnfContext * context, const char * plugins_dir)
{
    pluginsDir = plugins_dir ? plugins_dir : "";
}

bool
dnf_context_plugin_hook(DnfContext * context, PluginHookId id, DnfPluginHookData * hookData, DnfPluginError * error)
{
    DnfContextPrivate *priv = GET_PRIVATE(context);
    return priv->plugins->hook(id, hookData, error);
}

gchar *
dnf_context_get_module_report(DnfContext * context)
{
    DnfContextPrivate *priv = GET_PRIVATE (context);

    /* create sack and add sources */
    if (priv->sack == nullptr) {
        return nullptr;
    }
    auto container = dnf_sack_get_module_container(priv->sack);
    if (container == nullptr) {
        return nullptr;
    }
    auto report = container->getReport();
    if (report.empty()) {
        return nullptr;
    }
    return g_strdup(report.c_str());
}

DnfContext *
pluginGetContext(DnfPluginInitData * data)
{
    if (!data) {
        auto logger(libdnf::Log::getLogger());
        logger->error(tfm::format("%s: was called with data == nullptr", __func__));
        return nullptr;
    }
    if (data->mode != PLUGIN_MODE_CONTEXT) {
        auto logger(libdnf::Log::getLogger());
        logger->error(tfm::format("%s: was called with pluginMode == %i", __func__, data->mode));
        return nullptr;
    }
    return (static_cast<PluginHookContextInitData *>(data)->context);
}

static std::vector<std::tuple<libdnf::ModulePackageContainer::ModuleErrorType, std::string, std::string>>
recompute_modular_filtering(libdnf::ModulePackageContainer * moduleContainer, DnfSack * sack, std::vector<const char *> & hotfixRepos)
{
    auto solver_errors = dnf_sack_filter_modules_v2(
        sack, moduleContainer, hotfixRepos.data(), nullptr, nullptr, true, false, false);
    if (solver_errors.second == libdnf::ModulePackageContainer::ModuleErrorType::NO_ERROR) {
        return {};
    }
    auto formated_problem = libdnf::Goal::formatAllProblemRules(solver_errors.first);
    std::vector<std::tuple<libdnf::ModulePackageContainer::ModuleErrorType, std::string, std::string>> ret;
    ret.emplace_back(std::make_tuple(solver_errors.second, std::move(formated_problem), std::string()));
    return ret;
}

static gboolean
recompute_modular_filtering(DnfContext * context, DnfSack * sack, GError ** error)
{
    DnfContextPrivate * priv = GET_PRIVATE(context);
    std::vector<const char *> hotfixRepos;
    // don't filter RPMs from repos with the 'module_hotfixes' flag set
    for (unsigned int i = 0; i < priv->repos->len; i++) {
        auto repo = static_cast<DnfRepo *>(g_ptr_array_index(priv->repos, i));
        if (dnf_repo_get_module_hotfixes(repo)) {
            hotfixRepos.push_back(dnf_repo_get_id(repo));
        }
    }
    hotfixRepos.push_back(nullptr);
    try {
        dnf_sack_filter_modules_v2(sack, nullptr, hotfixRepos.data(), priv->install_root,
            priv->platform_module, false, false, false);
    } catch (libdnf::ModulePackageContainer::ConflictException & exception) {
        g_set_error(error, DNF_ERROR, DNF_ERROR_FAILED, "%s", exception.what());
        return FALSE;
    }
    return TRUE;
}

gboolean
dnf_context_reset_modules(DnfContext * context, DnfSack * sack, const char ** module_names, GError ** error) try
{
    assert(sack);
    assert(module_names);

    auto container = dnf_sack_get_module_container(sack);
    if (!container) {
        return TRUE;
    }
    for (const char ** names = module_names; *names != NULL; ++names) {
        try {
            container->reset(*names);
        } catch (libdnf::ModulePackageContainer::NoModuleException & exception) {
            g_set_error(error, DNF_ERROR, DNF_ERROR_FAILED, "%s", exception.what());
            return FALSE;
        }
    }
    container->save();
    container->updateFailSafeData();
    return recompute_modular_filtering(context, sack, error);
} CATCH_TO_GERROR(FALSE)

gboolean
dnf_context_reset_all_modules(DnfContext * context, DnfSack * sack, GError ** error) try
{
    assert(sack);

    auto container = dnf_sack_get_module_container(sack);
    if (!container) {
        return TRUE;
    }
    auto all_modules = container->getModulePackages();
    std::unordered_set<std::string> names;
    for (auto module: all_modules) {
        names.emplace(module->getName());
    }
    for (auto & name: names) {
        container->reset(name);
    }
    //container->save();
    //container->updateFailSafeData();
    return recompute_modular_filtering(context, sack, error);
} CATCH_TO_GERROR(FALSE)

/// module dict { name : {stream : [modules] }
static std::map<std::string, std::map<std::string, std::vector<libdnf::ModulePackage *>>> create_module_dict(
    const std::vector<libdnf::ModulePackage *> & modules)
{
    std::map<std::string, std::map<std::string, std::vector<libdnf::ModulePackage *>>> data;
    for (auto * module : modules) {
        data[module->getName()][module->getStream()].push_back(module);
    }
    return data;
}

/// Modify module_dict => Keep only single relevant stream
/// If more streams it keeps enabled or default stream
static std::vector<std::tuple<libdnf::ModulePackageContainer::ModuleErrorType, std::string, std::string>> modify_module_dict_and_enable_stream(std::map<std::string, std::map<std::string, std::vector<libdnf::ModulePackage *>>> & module_dict, libdnf::ModulePackageContainer & container, bool enable)
{
    std::vector<std::tuple<libdnf::ModulePackageContainer::ModuleErrorType, std::string, std::string>> messages;
    for (auto module_dict_iter : module_dict) {
        auto & name = module_dict_iter.first;
        auto & stream_dict = module_dict_iter.second;
        auto moduleState = container.getModuleState(name);
        if (stream_dict.size() > 1) {
            if (moduleState != libdnf::ModulePackageContainer::ModuleState::ENABLED 
                && moduleState != libdnf::ModulePackageContainer::ModuleState::DEFAULT) {
                messages.emplace_back(std::make_tuple(
                    libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_ENABLE_MULTIPLE_STREAMS,
                    tfm::format(_("Cannot enable more streams from module '%s' at the same time"), name), name));
                return messages;
            }
            const auto enabledOrDefaultStream = moduleState == libdnf::ModulePackageContainer::ModuleState::ENABLED ?
            container.getEnabledStream(name) : container.getDefaultStream(name);
            auto modules_iter = stream_dict.find(enabledOrDefaultStream);
            if (modules_iter == stream_dict.end()) {
                messages.emplace_back(std::make_tuple(
                    libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_ENABLE_MULTIPLE_STREAMS,
                    tfm::format(_("Cannot enable more streams from module '%s' at the same time"), name), name));
                return messages;;
            }
            if (enable) {
                try {
                    container.enable(name, modules_iter->first);
                } catch (libdnf::ModulePackageContainer::EnableMultipleStreamsException &) {
                    messages.emplace_back(std::make_tuple(
                        libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_MODIFY_MULTIPLE_TIMES_MODULE_STATE,
                        tfm::format(_("Cannot enable module '%1$s' stream '%2$s': State of module already modified"),
                                    name, modules_iter->first), name));
                }
            }
            for (auto iter = stream_dict.begin(); iter != stream_dict.end(); ) {
                if (iter->first != enabledOrDefaultStream) {
                    stream_dict.erase(iter++);
                } else {
                    ++iter;
                }
            }
        } else if (enable) {
            for (auto iter : stream_dict) {
                try {
                    container.enable(name, iter.first);
                } catch (libdnf::ModulePackageContainer::EnableMultipleStreamsException &) {
                    messages.emplace_back(std::make_tuple(
                        libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_MODIFY_MULTIPLE_TIMES_MODULE_STATE,
                        tfm::format(_("Cannot enable module '%1$s' stream '%2$s': State of module already modified"),
                                     name, iter.first), name));
                }
            }
        }
        if (stream_dict.size() != 1) {
            throw std::runtime_error("modify_module_dict_and_enable_stream() did not modify output correctly!!!");
        }
    }
    return messages;
}

static std::pair<std::unique_ptr<libdnf::Nsvcap>, std::vector< libdnf::ModulePackage*>> resolve_module_spec(const std::string & module_spec, libdnf::ModulePackageContainer & container)
{
    std::unique_ptr<libdnf::Nsvcap> nsvcapObj(new libdnf::Nsvcap);
    for (std::size_t i = 0; HY_MODULE_FORMS_MOST_SPEC[i] != _HY_MODULE_FORM_STOP_; ++i) {
        if (nsvcapObj->parse(module_spec.c_str(), HY_MODULE_FORMS_MOST_SPEC[i])) {
            auto result_modules = container.query(nsvcapObj->getName(),
                                                  nsvcapObj->getStream(),
                                                  nsvcapObj->getVersion(),
                                                  nsvcapObj->getContext(),
                                                  nsvcapObj->getArch());
            if (!result_modules.empty()) {
                return std::make_pair(std::move(nsvcapObj), std::move(result_modules));
            }
        }
    }
    return {};
}

static bool
report_problems(const std::vector<std::tuple<libdnf::ModulePackageContainer::ModuleErrorType, std::string, std::string>> & messages)
{
    libdnf::ModulePackageContainer::ModuleErrorType typeMessage;
    std::string report;
    std::string argument;
    auto logger(libdnf::Log::getLogger());
    bool return_error = false;
    for (auto & message : messages) {
        std::tie(typeMessage, report, argument) = message;
        switch (typeMessage) {
            case libdnf::ModulePackageContainer::ModuleErrorType::NO_ERROR:
                break;
            case libdnf::ModulePackageContainer::ModuleErrorType::INFO:
                logger->notice(report);
                break;
            case libdnf::ModulePackageContainer::ModuleErrorType::ERROR_IN_DEFAULTS:
                logger->warning(tfm::format(_("Modular dependency problem with Defaults: %s"), report.c_str()));
                break;
            case libdnf::ModulePackageContainer::ModuleErrorType::ERROR:
                logger->error(tfm::format(_("Modular dependency problem: %s"), report.c_str()));
                return_error = true;
                break;
            case libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_RESOLVE_MODULES:
                logger->error(report);
                return_error = true;
                break;
            case libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_RESOLVE_MODULE_SPEC:
                logger->error(report);
                return_error = true;
                break;
            case libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_ENABLE_MULTIPLE_STREAMS:
                logger->error(report);
                return_error = true;
                break;
            case libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_MODIFY_MULTIPLE_TIMES_MODULE_STATE:
                logger->error(report);
                return_error = true;
                break;
        }
    }
    return return_error;
}

static std::vector<std::tuple<libdnf::ModulePackageContainer::ModuleErrorType, std::string, std::string>>
modules_reset_or_disable(libdnf::ModulePackageContainer & container, const char ** module_specs, bool reset)
{
    std::vector<std::tuple<libdnf::ModulePackageContainer::ModuleErrorType, std::string, std::string>> messages;

    for (const char ** specs = module_specs; *specs != NULL; ++specs) {
        auto resolved_spec = resolve_module_spec(*specs, container);
        if (!resolved_spec.first) {
            messages.emplace_back(
                std::make_tuple(libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_RESOLVE_MODULE_SPEC,
                                tfm::format(_("Unable to resolve argument '%s'"), *specs), *specs));
            continue;
        }
        if (!resolved_spec.first->getStream().empty() || !resolved_spec.first->getProfile().empty() ||
            !resolved_spec.first->getVersion().empty() || !resolved_spec.first->getContext().empty()) {
        messages.emplace_back(std::make_tuple(
            libdnf::ModulePackageContainer::ModuleErrorType::INFO,
            tfm::format(_("Only module name is required. Ignoring unneeded information in argument: '%s'"), *specs),
            *specs));
        }
        std::unordered_set<std::string> names;
        for (auto * module : resolved_spec.second) {
            names.insert(std::move(module->getName()));
        }
        for (auto & name : names) {
            if (reset) {
                try {
                    container.reset(name);
                } catch (libdnf::ModulePackageContainer::EnableMultipleStreamsException &) {
                    messages.emplace_back(std::make_tuple(
                        libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_MODIFY_MULTIPLE_TIMES_MODULE_STATE,
                        tfm::format(_("Cannot reset module '%s': State of module already modified"), name), name));
                    messages.emplace_back(
                        std::make_tuple(libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_RESOLVE_MODULE_SPEC,
                                        tfm::format(_("Unable to resolve argument '%s'"), *specs), *specs));
                }
            } else {
                try {
                    container.disable(name);
                } catch (libdnf::ModulePackageContainer::EnableMultipleStreamsException &) {
                    messages.emplace_back(std::make_tuple(
                        libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_MODIFY_MULTIPLE_TIMES_MODULE_STATE,
                        tfm::format(_("Cannot disable module '%s': State of module already modified"), name), name));
                    messages.emplace_back(
                        std::make_tuple(libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_RESOLVE_MODULE_SPEC,
                                        tfm::format(_("Unable to resolve argument '%s'"), *specs), *specs));
                }
            }
        }
    }

    return messages;
}

gboolean
dnf_context_module_enable(DnfContext * context, const char ** module_specs, GError ** error) try
{
    DnfContextPrivate *priv = GET_PRIVATE (context);

    /* create sack and add sources */
    if (priv->sack == nullptr) {
        dnf_state_reset (priv->state);
        if (!dnf_context_setup_sack(context, priv->state, error)) {
            return FALSE;
        }
    }

    DnfSack * sack = priv->sack;
    assert(sack);
    assert(module_specs);

    auto container = dnf_sack_get_module_container(sack);
    if (!container) {
        g_set_error_literal(error, DNF_ERROR, DNF_ERROR_FAILED, _("No modular data available"));
        return FALSE;
    }
    std::vector<std::tuple<libdnf::ModulePackageContainer::ModuleErrorType, std::string, std::string>> messages;

    std::vector<std::pair<const char *, std::map<std::string, std::map<std::string, std::vector<libdnf::ModulePackage *>>>>> all_resolved_module_dicts;
    for (const char ** specs = module_specs; *specs != NULL; ++specs) {
        auto resolved_spec = resolve_module_spec(*specs, *container);
        if (!resolved_spec.first) {
            messages.emplace_back(
                std::make_tuple(libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_RESOLVE_MODULE_SPEC,
                                tfm::format(_("Unable to resolve argument '%s'"), *specs), *specs));
            continue;
        }
        if (!resolved_spec.first->getProfile().empty() || !resolved_spec.first->getVersion().empty() ||
            !resolved_spec.first->getContext().empty()) {
        messages.emplace_back(std::make_tuple(libdnf::ModulePackageContainer::ModuleErrorType::INFO,
                                              tfm::format(_("Ignoring unneeded information in argument: '%s'"), *specs),
                                              *specs));
        }
        auto module_dict = create_module_dict(resolved_spec.second);
        auto message = modify_module_dict_and_enable_stream(module_dict, *container, true);
        if (!message.empty()) {
            messages.insert(
                messages.end(),std::make_move_iterator(message.begin()), std::make_move_iterator(message.end()));
            messages.emplace_back(
                std::make_tuple(libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_RESOLVE_MODULE_SPEC,
                                tfm::format(_("Unable to resolve argument '%s'"), *specs), *specs));
        } else {
            all_resolved_module_dicts.emplace_back(make_pair(*specs, std::move(module_dict)));
        }
    }

    std::vector<const char *> hotfixRepos;
    // don't filter RPMs from repos with the 'module_hotfixes' flag set
    for (unsigned int i = 0; i < priv->repos->len; i++) {
        auto repo = static_cast<DnfRepo *>(g_ptr_array_index(priv->repos, i));
        if (dnf_repo_get_module_hotfixes(repo)) {
            hotfixRepos.push_back(dnf_repo_get_id(repo));
        }
    }
    hotfixRepos.push_back(nullptr);
    auto solver_error = recompute_modular_filtering(container, sack, hotfixRepos);
    if (!solver_error.empty()) {
        messages.insert(
            messages.end(),std::make_move_iterator(solver_error.begin()), std::make_move_iterator(solver_error.end()));
    }
    for (auto & pair : all_resolved_module_dicts) {
        for (auto module_dict_iter : pair.second) {
            for (auto & stream_dict_iter : module_dict_iter.second) {
                try {
                    container->enableDependencyTree(stream_dict_iter.second);
                } catch (const libdnf::ModulePackageContainer::EnableMultipleStreamsException & exception) {
                    messages.emplace_back(std::make_tuple(
                        libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_MODIFY_MULTIPLE_TIMES_MODULE_STATE,
                        tfm::format(_("Problem during enablement of dependency tree for module '%1$s' stream '%2$s': %3$s"),
                                     module_dict_iter.first, stream_dict_iter.first, exception.what()), pair.first));
                    messages.emplace_back(std::make_tuple(
                        libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_RESOLVE_MODULE_SPEC,
                        tfm::format(_("Unable to resolve argument '%s'"), pair.first), pair.first));
                }
            }
        }
    }
    bool return_error = report_problems(messages);

    if (return_error) {
        g_set_error_literal(error, DNF_ERROR, DNF_ERROR_FAILED, _("Problems appeared for module enable request"));
        return FALSE;
    }
    return TRUE;
} CATCH_TO_GERROR(FALSE)

gboolean
dnf_context_module_install(DnfContext * context, const char ** module_specs, GError ** error) try
{
    DnfContextPrivate *priv = GET_PRIVATE (context);

    /* create sack and add sources */
    if (priv->sack == nullptr) {
        dnf_state_reset (priv->state);
        if (!dnf_context_setup_sack(context, priv->state, error)) {
            return FALSE;
        }
    }

    DnfSack * sack = priv->sack;
    assert(sack);
    assert(module_specs);

    auto container = dnf_sack_get_module_container(sack);
    if (!container) {
        g_set_error_literal(error, DNF_ERROR, DNF_ERROR_FAILED, _("No modular data available"));
        return FALSE;
    }
    std::vector<std::tuple<libdnf::ModulePackageContainer::ModuleErrorType, std::string, std::string>> messages;

    std::vector<std::string> nevras_to_install;
    for (const char ** specs = module_specs; *specs != NULL; ++specs) {
        auto resolved_spec = resolve_module_spec(*specs, *container);
        if (!resolved_spec.first) {
            messages.emplace_back(
                std::make_tuple(libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_RESOLVE_MODULE_SPEC,
                                tfm::format(_("Unable to resolve argument '%s'"), *specs), *specs));
            continue;
        }
        auto module_dict = create_module_dict(resolved_spec.second);
        auto message = modify_module_dict_and_enable_stream(module_dict, *container, true);
        if (!message.empty()) {
            messages.insert(
                messages.end(),std::make_move_iterator(message.begin()), std::make_move_iterator(message.end()));
            messages.emplace_back(
                std::make_tuple(libdnf::ModulePackageContainer::ModuleErrorType::CANNOT_RESOLVE_MODULE_SPEC,
                                tfm::format(_("Unable to resolve argument '%s'"), *specs), *specs));
        } else {
            for (auto module_dict_iter : module_dict) {
                auto & stream_dict = module_dict_iter.second;
                // this was checked in modify_module_dict_and_enable_stream()
                assert(stream_dict.size() == 1);
                for (const auto &iter : stream_dict) {
                    for (const auto &modpkg : iter.second) {
                        std::vector<libdnf::ModuleProfile> profiles;
                        if (resolved_spec.first->getProfile() != "") {
                            profiles = modpkg->getProfiles(resolved_spec.first->getProfile());
                        } else {
                            profiles.push_back(modpkg->getDefaultProfile());
                        }
                        std::set<std::string> pkgnames;
                        for (const auto &profile : profiles) {
                            container->install(modpkg, profile.getName());
                            for (const auto &pkgname : profile.getContent()) {
                                pkgnames.insert(pkgname);
                            }
                        }
                        for (const auto &nevra : modpkg->getArtifacts()) {
                            int epoch;
                            char *name, *version, *release, *arch;
                            if (hy_split_nevra(nevra.c_str(), &name, &epoch, &version, &release, &arch)) {
                                // this really should never happen; unless the modular repodata is corrupted
                                g_autofree char *errmsg = g_strdup_printf (_("Failed to parse module artifact NEVRA '%s'"), nevra.c_str());
                                g_set_error_literal(error, DNF_ERROR, DNF_ERROR_FAILED, errmsg);
                                return FALSE;
                            }
                            // ignore source packages
                            if (g_str_equal (arch, "src") || g_str_equal (arch, "nosrc"))
                                continue;
                            if (pkgnames.count(name) != 0) {
                                nevras_to_install.push_back(std::string(nevra));
                            }
                        }
                    }
                }
            }
        }
    }

    std::vector<const char *> hotfixRepos;
    // don't filter RPMs from repos with the 'module_hotfixes' flag set
    for (unsigned int i = 0; i < priv->repos->len; i++) {
        auto repo = static_cast<DnfRepo *>(g_ptr_array_index(priv->repos, i));
        if (dnf_repo_get_module_hotfixes(repo)) {
            hotfixRepos.push_back(dnf_repo_get_id(repo));
        }
    }
    hotfixRepos.push_back(nullptr);
    auto solver_error = recompute_modular_filtering(container, sack, hotfixRepos);
    if (!solver_error.empty()) {
        messages.insert(
            messages.end(),std::make_move_iterator(solver_error.begin()), std::make_move_iterator(solver_error.end()));
    }

    bool return_error = report_problems(messages);
    if (return_error) {
        g_set_error_literal(error, DNF_ERROR, DNF_ERROR_FAILED, _("Problems appeared for module install request"));
        return FALSE;
    } else {
        for (const auto &nevra : nevras_to_install) {
            if (!dnf_context_install (context, nevra.c_str(), error))
              return FALSE;
        }
    }
    return TRUE;
} CATCH_TO_GERROR(FALSE)

static gboolean
context_modules_reset_or_disable(DnfContext * context, const char ** module_specs, GError ** error, bool reset)
{
    DnfContextPrivate *priv = GET_PRIVATE (context);

    /* create sack and add sources */
    if (priv->sack == nullptr) {
        dnf_state_reset (priv->state);
        if (!dnf_context_setup_sack(context, priv->state, error)) {
            return FALSE;
        }
    }

    DnfSack * sack = priv->sack;
    assert(module_specs);

    auto container = dnf_sack_get_module_container(sack);
    if (!container) {
        g_set_error_literal(error, DNF_ERROR, DNF_ERROR_FAILED, _("No modular data available"));
        return FALSE;
    }
    std::vector<std::tuple<libdnf::ModulePackageContainer::ModuleErrorType, std::string, std::string>> messages;

    auto disable_errors = modules_reset_or_disable(*container, module_specs, reset);
    if (!disable_errors.empty()) {
        messages.insert(
            messages.end(),
            std::make_move_iterator(disable_errors.begin()),
            std::make_move_iterator(disable_errors.end()));
    }

    std::vector<const char *> hotfixRepos;
    // don't filter RPMs from repos with the 'module_hotfixes' flag set
    for (unsigned int i = 0; i < priv->repos->len; i++) {
        auto repo = static_cast<DnfRepo *>(g_ptr_array_index(priv->repos, i));
        if (dnf_repo_get_module_hotfixes(repo)) {
            hotfixRepos.push_back(dnf_repo_get_id(repo));
        }
    }
    hotfixRepos.push_back(nullptr);
    auto solver_error = recompute_modular_filtering(container, sack, hotfixRepos);
    if (!solver_error.empty()) {
        messages.insert(
            messages.end(),std::make_move_iterator(solver_error.begin()), std::make_move_iterator(solver_error.end()));
    }
    bool return_error = report_problems(messages);

    if (return_error) {
        if (reset) {
            g_set_error_literal(error, DNF_ERROR, DNF_ERROR_FAILED, _("Problems appeared for module reset request"));
        } else {
            g_set_error_literal(error, DNF_ERROR, DNF_ERROR_FAILED, _("Problems appeared for module disable request"));
        }
        return FALSE;
    }
    return TRUE;
}


gboolean
dnf_context_module_disable(DnfContext * context, const char ** module_specs, GError ** error) try
{
    return context_modules_reset_or_disable(context, module_specs, error, false);
} CATCH_TO_GERROR(FALSE)

gboolean
dnf_context_module_reset(DnfContext * context, const char ** module_specs, GError ** error) try
{
    return context_modules_reset_or_disable(context, module_specs, error, true);
} CATCH_TO_GERROR(FALSE)

gboolean
dnf_context_module_switched_check(DnfContext * context, GError ** error) try
{
    DnfContextPrivate *priv = GET_PRIVATE (context);
    if (priv->sack == nullptr) {
        return TRUE;
    }
    auto container = dnf_sack_get_module_container(priv->sack);
    if (!container) {
        return TRUE;
    }
    auto switched = container->getSwitchedStreams();
    if (switched.empty()) {
        return TRUE;
    }
    auto logger(libdnf::Log::getLogger());
    const char * msg = _("The operation would result in switching of module '%s' stream '%s' to stream '%s'");
    for (auto item : switched) {
        logger->warning(tfm::format(msg, item.first.c_str(), item.second.first.c_str(), item.second.second.c_str()));
    }
    const char * msg_error = _("It is not possible to switch enabled streams of a module.\n"
                       "It is recommended to remove all installed content from the module, and "
                       "reset the module using 'microdnf module reset <module_name>' command. After "
                       "you reset the module, you can install the other stream.");
    g_set_error_literal(error, DNF_ERROR, DNF_ERROR_FAILED, msg_error);
    return FALSE;
} CATCH_TO_GERROR(FALSE)

namespace libdnf {

std::map<std::string, std::string> &
dnf_context_get_vars(DnfContext * context)
{
    return *GET_PRIVATE(context)->vars;
}

bool
dnf_context_get_vars_cached(DnfContext * context)
{
    return GET_PRIVATE(context)->varsCached;
}

void
dnf_context_load_vars(DnfContext * context)
{
    auto priv = GET_PRIVATE(context);
    priv->vars->clear();
    for (auto dir = dnf_context_get_vars_dir(context); *dir; ++dir)
        ConfigMain::addVarsFromDir(*priv->vars, std::string(priv->install_root) + *dir);
    ConfigMain::addVarsFromEnv(*priv->vars);
    priv->varsCached = true;
}

/* Context part of libdnf (microdnf, packagekit) needs to support global configuration
 * because of options such as best, zchunk.. This static std::unique_ptr is a hacky way
 * to do it without touching packagekit.
 */
static std::unique_ptr<libdnf::ConfigMain> globalMainConfig;
static std::mutex getGlobalMainConfigMutex;
static std::atomic_flag cfgMainLoaded = ATOMIC_FLAG_INIT;

static std::vector<Setopt> globalSetopts;
static bool globalSetoptsInSync = true;

bool
addSetopt(const char * key, Option::Priority priority, const char * value, GError ** error)
{
    auto dot = strrchr(key, '.');
    if (dot && *(dot+1) == '\0') {
        g_set_error(error, DNF_ERROR, DNF_ERROR_UNKNOWN_OPTION, "Last key character cannot be '.': %s", key);
        return false;
    }

    // Store option to vector. Use it later when configuration will be loaded.
    globalSetopts.push_back({static_cast<libdnf::Option::Priority>(priority), key, value});
    globalSetoptsInSync = false;

    return true;
}

const std::vector<Setopt> &
getGlobalSetopts()
{
    return globalSetopts;
}

static void
dnf_main_conf_apply_setopts()
{
    if (globalSetoptsInSync) {
        return;
    }

    // apply global main setopts
    auto & optBinds = globalMainConfig->optBinds();
    for (const auto & setopt : globalSetopts) {
        if (setopt.key.find('.') == std::string::npos) {
            try {
                auto & optionItem = optBinds.at(setopt.key);
                try {
                    optionItem.newString(setopt.priority, setopt.value);
                } catch (const std::exception & ex) {
                    g_warning("dnf_main_conf_apply_setopt: Invalid configuration value: %s = %s; %s", setopt.key.c_str(), setopt.value.c_str(), ex.what());
                }
            } catch (const std::exception &) {
                g_warning("dnf_main_conf_apply_setopt: Unknown configuration option: %s in %s = %s", setopt.key.c_str(), setopt.key.c_str(), setopt.value.c_str());
            }
        }
    }

    globalSetoptsInSync = true;
}

libdnf::ConfigMain & getGlobalMainConfig(bool canReadConfigFile)
{
    std::lock_guard<std::mutex> guard(getGlobalMainConfigMutex);

    if (!globalMainConfig) {
        globalMainConfig.reset(new libdnf::ConfigMain);
        // The gpgcheck was enabled by default in context part of libdnf. We stay "compatible".
        globalMainConfig->gpgcheck().set(libdnf::Option::Priority::DEFAULT, true);
    }

    if (canReadConfigFile && !cfgMainLoaded.test_and_set()) {
        if (configFilePath) {
            globalMainConfig->config_file_path().set(libdnf::Option::Priority::RUNTIME, *configFilePath);
            if (configFilePath->empty()) {
                return *globalMainConfig;
            }
        }

        libdnf::ConfigParser parser;
        const std::string cfgPath{globalMainConfig->config_file_path().getValue()};
        try {
            parser.read(cfgPath);
            const auto & cfgParserData = parser.getData();
            auto cfgParserDataIter = cfgParserData.find("main");
            if (cfgParserDataIter != cfgParserData.end()) {
                auto optBinds = globalMainConfig->optBinds();
                const auto & cfgParserMainSect = cfgParserDataIter->second;
                for (const auto & opt : cfgParserMainSect) {
                    auto optBindsIter = optBinds.find(opt.first);
                    if (optBindsIter != optBinds.end()) {
                        try {
                            optBindsIter->second.newString(libdnf::Option::Priority::MAINCONFIG, opt.second);
                        } catch (const std::exception & ex) {
                            g_warning("Config error in file \"%s\" section \"main\" key \"%s\": %s",
                                      cfgPath.c_str(), opt.first.c_str(), ex.what());
                        }
                    }
                }
            }
        } catch (const libdnf::ConfigParser::CantOpenFile & ex) {
            if (configFilePath) {
                // Only warning is logged. But error is reported to the caller during loading
                // repos (in dnf_repo_loader_refresh()).
                g_warning("Loading \"%s\": %s", cfgPath.c_str(), ex.what());
            }
        } catch (const std::exception & ex) {
            g_warning("Loading \"%s\": %s", cfgPath.c_str(), ex.what());
        }
    }

    dnf_main_conf_apply_setopts();
    return *globalMainConfig;
}

}

