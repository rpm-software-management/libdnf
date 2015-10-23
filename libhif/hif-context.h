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

#if !defined (__LIBHIF_H) && !defined (HIF_COMPILATION)
#error "Only <libhif.h> can be included directly."
#endif

#ifndef __HIF_CONTEXT_H
#define __HIF_CONTEXT_H

#include "hif-types.h"

#ifndef __GI_SCANNER__
#include "hy-goal.h"
#include "hy-sack.h"
#endif

G_BEGIN_DECLS

#define HIF_TYPE_CONTEXT (hif_context_get_type ())
G_DECLARE_DERIVABLE_TYPE (HifContext, hif_context, HIF, CONTEXT, GObject)

struct _HifContextClass
{
        GObjectClass            parent_class;
        void                    (*invalidate)           (HifContext     *context,
                                                         const gchar    *message);
        /*< private >*/
        void (*_hif_reserved1)  (void);
        void (*_hif_reserved2)  (void);
        void (*_hif_reserved3)  (void);
        void (*_hif_reserved4)  (void);
        void (*_hif_reserved5)  (void);
        void (*_hif_reserved6)  (void);
        void (*_hif_reserved7)  (void);
        void (*_hif_reserved8)  (void);
};

/**
 * HifContextInvalidateFlags:
 * @HIF_CONTEXT_INVALIDATE_FLAG_NONE:           No caches are invalid
 * @HIF_CONTEXT_INVALIDATE_FLAG_RPMDB:          The rpmdb cache is invalid
 * @HIF_CONTEXT_INVALIDATE_FLAG_ENROLLMENT:     Any enrollment may be invalid
 *
 * The update flags.
 **/
typedef enum {
        HIF_CONTEXT_INVALIDATE_FLAG_NONE        = 0,
        HIF_CONTEXT_INVALIDATE_FLAG_RPMDB       = 1,
        HIF_CONTEXT_INVALIDATE_FLAG_ENROLLMENT  = 2,
        /*< private >*/
        HIF_CONTEXT_INVALIDATE_FLAG_LAST
} HifContextInvalidateFlags;

HifContext      *hif_context_new                        (void);

/* getters */
const gchar     *hif_context_get_repo_dir               (HifContext     *context);
const gchar     *hif_context_get_base_arch              (HifContext     *context);
const gchar     *hif_context_get_os_info                (HifContext     *context);
const gchar     *hif_context_get_arch_info              (HifContext     *context);
const gchar     *hif_context_get_release_ver            (HifContext     *context);
const gchar     *hif_context_get_cache_dir              (HifContext     *context);
const gchar     *hif_context_get_solv_dir               (HifContext     *context);
const gchar     *hif_context_get_lock_dir               (HifContext     *context);
const gchar     *hif_context_get_rpm_verbosity          (HifContext     *context);
const gchar     *hif_context_get_install_root           (HifContext     *context);
const gchar     **hif_context_get_native_arches         (HifContext     *context);
const gchar     **hif_context_get_installonly_pkgs      (HifContext     *context);
gboolean         hif_context_get_check_disk_space       (HifContext     *context);
gboolean         hif_context_get_check_transaction      (HifContext     *context);
gboolean         hif_context_get_keep_cache             (HifContext     *context);
gboolean         hif_context_get_only_trusted           (HifContext     *context);
gboolean         hif_context_get_yumdb_enabled          (HifContext     *context);
guint            hif_context_get_cache_age              (HifContext     *context);
guint            hif_context_get_installonly_limit      (HifContext     *context);
const gchar     *hif_context_get_http_proxy             (HifContext     *context);
GPtrArray       *hif_context_get_sources                (HifContext     *context);
#ifndef __GI_SCANNER__
HifRepos        *hif_context_get_repos                  (HifContext     *context);
HifTransaction  *hif_context_get_transaction            (HifContext     *context);
HySack           hif_context_get_sack                   (HifContext     *context);
HyGoal           hif_context_get_goal                   (HifContext     *context);
#endif
HifState*        hif_context_get_state                  (HifContext     *context);

/* setters */
void             hif_context_set_repo_dir               (HifContext     *context,
                                                         const gchar    *repo_dir);
void             hif_context_set_release_ver            (HifContext     *context,
                                                         const gchar    *release_ver);
void             hif_context_set_cache_dir              (HifContext     *context,
                                                         const gchar    *cache_dir);
void             hif_context_set_solv_dir               (HifContext     *context,
                                                         const gchar    *solv_dir);
void             hif_context_set_vendor_cache_dir       (HifContext     *context,
                                                         const gchar    *vendor_cache_dir);
void             hif_context_set_vendor_solv_dir        (HifContext     *context,
                                                         const gchar    *vendor_solv_dir);
void             hif_context_set_lock_dir               (HifContext     *context,
                                                         const gchar    *lock_dir);
void             hif_context_set_rpm_verbosity          (HifContext     *context,
                                                         const gchar    *rpm_verbosity);
void             hif_context_set_install_root           (HifContext     *context,
                                                         const gchar    *install_root);
void             hif_context_set_check_disk_space       (HifContext     *context,
                                                         gboolean        check_disk_space);
void             hif_context_set_check_transaction      (HifContext     *context,
                                                         gboolean        check_transaction);
void             hif_context_set_keep_cache             (HifContext     *context,
                                                         gboolean        keep_cache);
void             hif_context_set_only_trusted           (HifContext     *context,
                                                         gboolean        only_trusted);
void             hif_context_set_yumdb_enabled          (HifContext     *context,
                                                         gboolean        enable_yumdb);
void             hif_context_set_cache_age              (HifContext     *context,
                                                         guint           cache_age);

void             hif_context_set_rpm_macro              (HifContext     *context,
                                                         const gchar    *key,
                                                         const gchar    *value);
void             hif_context_set_http_proxy             (HifContext     *context,
                                                         const gchar    *proxyurl);

/* object methods */
gboolean         hif_context_setup                      (HifContext     *context,
                                                         GCancellable   *cancellable,
                                                         GError         **error);
gboolean         hif_context_setup_enrollments          (HifContext     *context,
                                                         GError         **error);
gboolean         hif_context_setup_sack                 (HifContext     *context,
                                                         HifState       *state,
                                                         GError         **error);
gboolean         hif_context_commit                     (HifContext     *context,
                                                         HifState       *state,
                                                         GError         **error);
void             hif_context_invalidate                 (HifContext     *context,
                                                         const gchar    *message);
void             hif_context_invalidate_full            (HifContext     *context,
                                                         const gchar    *message,
                                                         HifContextInvalidateFlags flags);
gboolean         hif_context_install                    (HifContext     *context,
                                                         const gchar    *name,
                                                         GError         **error);
gboolean         hif_context_remove                     (HifContext     *context,
                                                         const gchar    *name,
                                                         GError         **error);
gboolean         hif_context_update                     (HifContext     *context,
                                                         const gchar    *name,
                                                         GError         **error);
gboolean         hif_context_repo_enable                (HifContext     *context,
                                                         const gchar    *repo_id,
                                                         GError         **error);
gboolean         hif_context_repo_disable               (HifContext     *context,
                                                         const gchar    *repo_id,
                                                         GError         **error);
gboolean         hif_context_run                        (HifContext     *context,
                                                         GCancellable   *cancellable,
                                                         GError         **error);

G_END_DECLS

#endif /* __HIF_CONTEXT_H */
