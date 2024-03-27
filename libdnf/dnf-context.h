/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2014-2015 Richard Hughes <richard@hughsie.com>
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

#ifndef __DNF_CONTEXT_H
#define __DNF_CONTEXT_H

#include "dnf-types.h"
#include "plugin/plugin.h"

#ifndef __GI_SCANNER__
#include "dnf-transaction.h"
#include "dnf-sack.h"
#endif

#include <stdbool.h>

G_BEGIN_DECLS

#define DNF_TYPE_CONTEXT (dnf_context_get_type ())
G_DECLARE_DERIVABLE_TYPE (DnfContext, dnf_context, DNF, CONTEXT, GObject)

struct _DnfContextClass
{
        GObjectClass            parent_class;
        void                    (*invalidate)           (DnfContext     *context,
                                                         const gchar    *message);
        /*< private >*/
        void (*_dnf_reserved1)  (void);
        void (*_dnf_reserved2)  (void);
        void (*_dnf_reserved3)  (void);
        void (*_dnf_reserved4)  (void);
        void (*_dnf_reserved5)  (void);
        void (*_dnf_reserved6)  (void);
        void (*_dnf_reserved7)  (void);
        void (*_dnf_reserved8)  (void);
};

/**
 * DnfContextCleanFlags:
 * @DNF_CONTEXT_CLEAN_EXPIRE_CACHE:            Clean the indicator file for cache directories' age
 * @DNF_CONTEXT_CLEAN_PACKAGES:                Clean the packages section for cache
 * @DNF_CONTEXT_CLEAN_METADATA:                Clean the metadata section for cache directories
 * @DNF_CONTEXT_CLEAN_ALL:                     Clean out all of the cache directories
 *
 * The clean flags for cache directories cleaning.
 **/
typedef enum {
        DNF_CONTEXT_CLEAN_EXPIRE_CACHE          = (1 << 0),
        DNF_CONTEXT_CLEAN_PACKAGES              = (1 << 1),
        DNF_CONTEXT_CLEAN_METADATA              = (1 << 2),
        DNF_CONTEXT_CLEAN_ALL                   = (1 << 3),
} DnfContextCleanFlags;

/**
 * DnfContextInvalidateFlags:
 * @DNF_CONTEXT_INVALIDATE_FLAG_NONE:           No caches are invalid
 * @DNF_CONTEXT_INVALIDATE_FLAG_RPMDB:          The rpmdb cache is invalid
 * @DNF_CONTEXT_INVALIDATE_FLAG_ENROLLMENT:     Any enrollment may be invalid
 *
 * The update flags.
 **/
typedef enum {
        DNF_CONTEXT_INVALIDATE_FLAG_NONE        = 0,
        DNF_CONTEXT_INVALIDATE_FLAG_RPMDB       = 1,
        DNF_CONTEXT_INVALIDATE_FLAG_ENROLLMENT  = 2,
        /*< private >*/
        DNF_CONTEXT_INVALIDATE_FLAG_LAST
} DnfContextInvalidateFlags;

/**
 * DnfContextSetupSackFlags:
 * @DNF_CONTEXT_SETUP_SACK_FLAG_NONE:             No special behaviours
 * @DNF_CONTEXT_SETUP_SACK_FLAG_SKIP_RPMDB:       Don't load system's rpmdb
 * @DNF_CONTEXT_SETUP_SACK_FLAG_SKIP_FILELISTS:   Don't load filelists
 * @DNF_CONTEXT_SETUP_SACK_FLAG_LOAD_UPDATEINFO:  Load updateinfo if available
 *
 * The sack setup flags.
 *
 * Since: 0.13.0
 **/
typedef enum {
        DNF_CONTEXT_SETUP_SACK_FLAG_NONE            = (1 << 0),
        DNF_CONTEXT_SETUP_SACK_FLAG_SKIP_RPMDB      = (1 << 1),
        DNF_CONTEXT_SETUP_SACK_FLAG_SKIP_FILELISTS  = (1 << 2),
        DNF_CONTEXT_SETUP_SACK_FLAG_LOAD_UPDATEINFO = (1 << 3),
} DnfContextSetupSackFlags;

gboolean         dnf_context_globals_init               (GError **error);

DnfContext      *dnf_context_new                        (void);

/* utils */
const gchar     *find_base_arch                         (const char *native);

/* getters */
const gchar     *dnf_context_get_config_file_path       (void);
gboolean         dnf_context_is_set_config_file_path    (void);
const gchar * const *dnf_context_get_repos_dir          (DnfContext     *context);
const gchar     *dnf_context_get_repo_dir               (DnfContext     *context);
const gchar * const *dnf_context_get_vars_dir           (DnfContext     *context);
const gchar     *dnf_context_get_base_arch              (DnfContext     *context);
const gchar     *dnf_context_get_os_info                (DnfContext     *context);
const gchar     *dnf_context_get_arch_info              (DnfContext     *context);
const gchar     *dnf_context_get_release_ver            (DnfContext     *context);
const gchar     *dnf_context_get_platform_module        (DnfContext     *context);
const gchar     *dnf_context_get_cache_dir              (DnfContext     *context);
const gchar     *dnf_context_get_arch                   (DnfContext     *context);
const gchar     *dnf_context_get_solv_dir               (DnfContext     *context);
const gchar     *dnf_context_get_lock_dir               (DnfContext     *context);
const gchar     *dnf_context_get_rpm_verbosity          (DnfContext     *context);
const gchar     *dnf_context_get_install_root           (DnfContext     *context);
const gchar     *dnf_context_get_source_root            (DnfContext     *context);
const gchar     **dnf_context_get_native_arches         (DnfContext     *context);
const gchar     **dnf_context_get_installonly_pkgs      (DnfContext     *context);
gboolean         dnf_context_get_best                   (void);
gboolean         dnf_context_get_install_weak_deps      (void);
gboolean         dnf_context_get_allow_vendor_change    (void);
gboolean         dnf_context_get_cache_only             (DnfContext     *context);
gboolean         dnf_context_get_check_disk_space       (DnfContext     *context);
gboolean         dnf_context_get_check_transaction      (DnfContext     *context);
gboolean         dnf_context_get_keep_cache             (DnfContext     *context);
gboolean         dnf_context_get_only_trusted           (DnfContext     *context);
gboolean         dnf_context_get_zchunk                 (DnfContext     *context);
gboolean         dnf_context_get_write_history          (DnfContext     *context);
guint            dnf_context_get_cache_age              (DnfContext     *context);
guint            dnf_context_get_installonly_limit      (DnfContext     *context);
guint            dnf_context_get_network_timeout_seconds(DnfContext     *context);
const gchar     *dnf_context_get_http_proxy             (DnfContext     *context);
gboolean         dnf_context_get_enable_filelists       (DnfContext     *context);
GPtrArray       *dnf_context_get_repos                  (DnfContext     *context);
#ifndef __GI_SCANNER__
DnfRepoLoader   *dnf_context_get_repo_loader            (DnfContext     *context);
DnfTransaction  *dnf_context_get_transaction            (DnfContext     *context);
DnfSack         *dnf_context_get_sack                   (DnfContext     *context);
HyGoal           dnf_context_get_goal                   (DnfContext     *context);
#endif
DnfState*        dnf_context_get_state                  (DnfContext     *context);
const char *     dnf_context_get_user_agent             (DnfContext     *context);

/* setters */
void             dnf_context_set_config_file_path       (const gchar    *config_file_path);
void             dnf_context_set_repos_dir              (DnfContext     *context,
                                                         const gchar * const *repos_dir);
void             dnf_context_set_repo_dir               (DnfContext     *context,
                                                         const gchar    *repo_dir);
void             dnf_context_set_vars_dir               (DnfContext     *context,
                                                         const gchar * const *vars_dir);
void             dnf_context_set_release_ver            (DnfContext     *context,
                                                         const gchar    *release_ver);
void             dnf_context_set_platform_module        (DnfContext     *context,
                                                         const gchar    *platform_module);
void             dnf_context_set_cache_dir              (DnfContext     *context,
                                                         const gchar    *cache_dir);
void             dnf_context_set_arch                   (DnfContext     *context,
                                                         const gchar    *base_arch);
void             dnf_context_set_solv_dir               (DnfContext     *context,
                                                         const gchar    *solv_dir);
void             dnf_context_set_vendor_cache_dir       (DnfContext     *context,
                                                         const gchar    *vendor_cache_dir);
void             dnf_context_set_vendor_solv_dir        (DnfContext     *context,
                                                         const gchar    *vendor_solv_dir);
void             dnf_context_set_lock_dir               (DnfContext     *context,
                                                         const gchar    *lock_dir);
void             dnf_context_set_rpm_verbosity          (DnfContext     *context,
                                                         const gchar    *rpm_verbosity);
void             dnf_context_set_install_root           (DnfContext     *context,
                                                         const gchar    *install_root);
void             dnf_context_set_source_root            (DnfContext     *context,
                                                         const gchar    *source_root);
void             dnf_context_set_best                   (gboolean        best);
void             dnf_context_set_install_weak_deps      (gboolean        enabled);
void             dnf_context_set_allow_vendor_change    (gboolean        vendorchange);
void             dnf_context_set_cache_only             (DnfContext     *context,
                                                         gboolean        cache_only);
void             dnf_context_set_check_disk_space       (DnfContext     *context,
                                                         gboolean        check_disk_space);
void             dnf_context_set_check_transaction      (DnfContext     *context,
                                                         gboolean        check_transaction);
void             dnf_context_set_keep_cache             (DnfContext     *context,
                                                         gboolean        keep_cache);
void             dnf_context_set_enable_filelists       (DnfContext     *context,
                                                         gboolean        enable_filelists);
void             dnf_context_set_only_trusted           (DnfContext     *context,
                                                         gboolean        only_trusted);
void             dnf_context_set_zchunk                 (DnfContext     *context,
                                                         gboolean        only_trusted);
void             dnf_context_set_write_history          (DnfContext     *context,
                                                         gboolean        value);
void             dnf_context_set_cache_age              (DnfContext     *context,
                                                         guint           cache_age);
void             dnf_context_set_network_timeout_seconds(DnfContext     *context,
                                                         guint           seconds);

void             dnf_context_set_rpm_macro              (DnfContext     *context,
                                                         const gchar    *key,
                                                         const gchar    *value);
void             dnf_context_set_http_proxy             (DnfContext     *context,
                                                         const gchar    *proxyurl);
void             dnf_context_set_user_agent             (DnfContext     *context,
                                                         const gchar    *user_agent);

/* object methods */
gboolean         dnf_context_setup                      (DnfContext     *context,
                                                         GCancellable   *cancellable,
                                                         GError         **error);
gboolean         dnf_context_setup_enrollments          (DnfContext     *context,
                                                         GError         **error);
gboolean         dnf_context_setup_sack                 (DnfContext     *context,
                                                         DnfState       *state,
                                                         GError         **error);
gboolean         dnf_context_setup_sack_with_flags      (DnfContext      *context,
                                                         DnfState        *state,
                                                         DnfContextSetupSackFlags flags,
                                                         GError          **error);
gboolean         dnf_context_commit                     (DnfContext     *context,
                                                         DnfState       *state,
                                                         GError         **error);
void             dnf_context_invalidate                 (DnfContext     *context,
                                                         const gchar    *message);
void             dnf_context_invalidate_full            (DnfContext     *context,
                                                         const gchar    *message,
                                                         DnfContextInvalidateFlags flags);
gboolean         dnf_context_clean_cache                (DnfContext     *context,
                                                         DnfContextCleanFlags      flags,
                                                         GError         **error);
gboolean         dnf_context_install                    (DnfContext     *context,
                                                         const gchar    *name,
                                                         GError         **error);
gboolean         dnf_context_remove                     (DnfContext     *context,
                                                         const gchar    *name,
                                                         GError         **error);
gboolean         dnf_context_update                     (DnfContext     *context,
                                                         const gchar    *name,
                                                         GError         **error);
gboolean         dnf_context_update_all                 (DnfContext     *context,
                                                         GError         **error);
gboolean         dnf_context_distrosync                 (DnfContext     *context,
                                                         const gchar    *name,
                                                         GError         **error);
gboolean         dnf_context_distrosync_all             (DnfContext     *context,
                                                         GError         **error);
gboolean         dnf_context_repo_enable                (DnfContext     *context,
                                                         const gchar    *repo_id,
                                                         GError         **error);
gboolean         dnf_context_repo_disable               (DnfContext     *context,
                                                         const gchar    *repo_id,
                                                         GError         **error);
gboolean         dnf_context_run                        (DnfContext     *context,
                                                         GCancellable   *cancellable,
                                                         GError         **error);

/* plugins support */
void             dnf_context_disable_plugins            (const gchar    *plugin_name_pattern);
void             dnf_context_enable_plugins             (const gchar    *plugin_name_pattern);
gchar**          dnf_context_get_disabled_plugins       (void);
gchar**          dnf_context_get_enabled_plugins        (void);
gboolean         dnf_context_get_plugins_all_disabled   (void);
void             dnf_context_set_plugins_all_disabled   (gboolean        disabled);
const char *     dnf_context_get_plugins_dir            (DnfContext     *context);
void             dnf_context_set_plugins_dir            (DnfContext     *context,
                                                         const char     *plugins_dir);
bool             dnf_context_plugin_hook                (DnfContext     *context,
                                                         PluginHookId    id,
                                                         DnfPluginHookData *hookData,
                                                         DnfPluginError *error);
/// String must be dealocated by g_free()
gchar *          dnf_context_get_module_report          (DnfContext * context);

/**
 * dnf_context_reset_modules:
 * @context: DnfContext
 * @sack: DnfSack
 * @module_names: Names of modules to reset
 * @error: Error
 *
 * Reset modules, commit modular changes, and recalculate module filtration.
 * Note you likely want to use dnf_context_module_reset instead which matches
 * the behaviour of other modular APIs to not commit modular changes to disk
 * until dnf_context_run(). Returns FALSE when an error is set.
 *
 * Since: 0.38.1
 **/
gboolean         dnf_context_reset_modules              (DnfContext * context,
                                                         DnfSack * sack,
                                                         const char ** module_names,
                                                         GError ** error);

/**
 * dnf_context_reset_all_modules:
 * @context: DnfContext
 * @sack: DnfSack
 * @error: Error
 *
 * Reset all modules and recalculate module filtration.
 * Returns FALSE when an error is set.
 *
 * Since: 0.46.2
 **/
gboolean         dnf_context_reset_all_modules          (DnfContext * context,
                                                         DnfSack * sack,
                                                         GError ** error);

/**
 * dnf_context_module_enable:
 * @context: DnfContext
 * @module_specs: Module specs that should be enabled
 * @error: Error
 *
 * Enable modules, recalculate module filtration, but do not commit modular changes.
 * To commit modular changes, call dnf_context_run().
 * Returns FALSE when an error is set.
 *
 * Since: 0.55.0
 **/
gboolean         dnf_context_module_enable              (DnfContext * context,
                                                         const char ** module_specs,
                                                         GError ** error);

/**
 * dnf_context_module_install:
 * @context: DnfContext
 * @module_specs: Module specs that should be installed
 * @error: Error
 *
 * Enable modules and mark for installation but do not commit modular changes.
 * To commit modular changes, call dnf_context_run().
 * Returns FALSE when an error is set.
 *
 * Since: 0.63.0
 **/
gboolean           dnf_context_module_install            (DnfContext * context,
                                                          const char ** module_specs,
                                                          GError ** error);

/**
 * dnf_context_module_disable:
 * @context: DnfContext
 * @module_specs: Module specs that should be enabled
 * @error: Error
 *
 * Disable modules, recalculate module filtration, but do not commit modular changes.
 * To commit modular changes it requires to call dnf_context_run()
 * Returns FALSE when an error is set.
 *
 * Since: 0.55.0
 **/
gboolean         dnf_context_module_disable             (DnfContext * context,
                                                         const char ** module_specs,
                                                         GError ** error);

/**
 * dnf_context_module_disable_all:
 * @context: DnfContext
 * @module_specs: Module specs that should be enabled
 * @error: Error
 *
 * Disable all modules, recalculate module filtration, but do not commit modular
 * changes.  To commit modular changes it requires to call dnf_context_run()
 * Returns FALSE when an error is set.
 *
 * Since: 0.64.0
 **/
gboolean         dnf_context_module_disable_all         (DnfContext * context,
                                                         GError ** error);

/**
 * dnf_context_module_reset:
 * @context: DnfContext
 * @module_specs: Module specs that should be enabled
 * @error: Error
 *
 * Reset modules, recalculate module filtration, but do not commit modular changes.
 * To commit modular changes it requires to call dnf_context_run()
 * Returns FALSE when an error is set.
 *
 * Since: 0.55.0
 **/
gboolean         dnf_context_module_reset               (DnfContext * context,
                                                         const char ** module_specs,
                                                         GError ** error);
/**
 * dnf_context_module_switched_check:
 * @context: DnfContext
 * @error: Error
 *
 * Ceck if any module is switched and return FALSE and sets an error
 * Returns FALSE when an error is set.
 *
 * Since: 0.55.0
 **/
gboolean         dnf_context_module_switched_check      (DnfContext * context,
                                                         GError ** error);

G_END_DECLS

#endif /* __DNF_CONTEXT_H */
