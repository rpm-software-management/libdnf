/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2009-2015 Richard Hughes <richard@hughsie.com>
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

#ifndef __HIF_STATE_H
#define __HIF_STATE_H

#include <glib-object.h>
#include <gio/gio.h>

#include "hif-types.h"
#include "hif-lock.h"

#define HIF_TYPE_STATE                  (hif_state_get_type())
#define HIF_STATE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST((obj), HIF_TYPE_STATE, HifState))
#define HIF_STATE_CLASS(cls)            (G_TYPE_CHECK_CLASS_CAST((cls), HIF_TYPE_STATE, HifStateClass))
#define HIF_IS_STATE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE((obj), HIF_TYPE_STATE))
#define HIF_IS_STATE_CLASS(cls)         (G_TYPE_CHECK_CLASS_TYPE((cls), HIF_TYPE_STATE))
#define HIF_STATE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS((obj), HIF_TYPE_STATE, HifStateClass))

G_BEGIN_DECLS

typedef struct _HifStateClass           HifStateClass;

struct _HifState
{
        GObject                         parent;
};

/**
 * HifStateAction:
 * @HIF_STATE_ACTION_UNKNOWN:                   Unknown status
 * @HIF_STATE_ACTION_DOWNLOAD_PACKAGES:         Downloading packages
 * @HIF_STATE_ACTION_DOWNLOAD_METADATA:         Downloading metadata
 * @HIF_STATE_ACTION_LOADING_CACHE:             Loading cache
 * @HIF_STATE_ACTION_TEST_COMMIT:               Testing transaction
 * @HIF_STATE_ACTION_REQUEST:                   Requesting data
 * @HIF_STATE_ACTION_REMOVE:                    Removing packages
 * @HIF_STATE_ACTION_INSTALL:                   Installing packages
 * @HIF_STATE_ACTION_UPDATE:                    Updating packages
 * @HIF_STATE_ACTION_CLEANUP:                   Cleaning packages
 * @HIF_STATE_ACTION_OBSOLETE:                  Obsoleting packages
 * @HIF_STATE_ACTION_REINSTALL:                 Reinstall packages
 * @HIF_STATE_ACTION_DOWNGRADE:                 Downgrading packages
 * @HIF_STATE_ACTION_QUERY:                     Querying for results
 *
 * The action enum code.
 **/
typedef enum {
        HIF_STATE_ACTION_UNKNOWN                = 0,    /* Since: 0.1.0 */
        HIF_STATE_ACTION_DOWNLOAD_PACKAGES      = 8,    /* Since: 0.1.0 */
        HIF_STATE_ACTION_DOWNLOAD_METADATA      = 20,   /* Since: 0.1.0 */
        HIF_STATE_ACTION_LOADING_CACHE          = 27,   /* Since: 0.1.0 */
        HIF_STATE_ACTION_TEST_COMMIT            = 15,   /* Since: 0.1.0 */
        HIF_STATE_ACTION_REQUEST                = 17,   /* Since: 0.1.0 */
        HIF_STATE_ACTION_REMOVE                 = 6,    /* Since: 0.1.0 */
        HIF_STATE_ACTION_INSTALL                = 9,    /* Since: 0.1.0 */
        HIF_STATE_ACTION_UPDATE                 = 10,   /* Since: 0.1.2 */
        HIF_STATE_ACTION_CLEANUP                = 11,   /* Since: 0.1.2 */
        HIF_STATE_ACTION_OBSOLETE               = 12,   /* Since: 0.1.2 */
        HIF_STATE_ACTION_REINSTALL              = 13,   /* Since: 0.1.6 */
        HIF_STATE_ACTION_DOWNGRADE              = 14,   /* Since: 0.1.6 */
        HIF_STATE_ACTION_QUERY                  = 4,    /* Since: 0.1.2 */
        /*< private >*/
        HIF_STATE_ACTION_LAST
} HifStateAction;

struct _HifStateClass
{
        GObjectClass    parent_class;
        void            (* percentage_changed)          (HifState       *state,
                                                         guint           value);
        void            (* allow_cancel_changed)        (HifState       *state,
                                                         gboolean        allow_cancel);
        void            (* action_changed)              (HifState       *state,
                                                         HifStateAction  action,
                                                         const gchar    *action_hint);
        void            (* package_progress_changed)    (HifState       *state,
                                                         const gchar    *package_id,
                                                         HifStateAction  action,
                                                         guint           percentage);
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

#define hif_state_done(state, error)                    hif_state_done_real(state, error, G_STRLOC)
#define hif_state_finished(state, error)                hif_state_finished_real(state, error, G_STRLOC)
#define hif_state_set_number_steps(state, steps)        hif_state_set_number_steps_real(state, steps, G_STRLOC)
#define hif_state_set_steps(state, error, value, args...)       hif_state_set_steps_real(state, error, G_STRLOC, value, ## args)

typedef gboolean (*HifStateErrorHandlerCb)              (const GError           *error,
                                                         gpointer                user_data);

GType            hif_state_get_type                     (void);
HifState        *hif_state_new                          (void);

/* getters */
guint            hif_state_get_percentage               (HifState               *state);
HifStateAction   hif_state_get_action                   (HifState               *state);
const gchar     *hif_state_get_action_hint              (HifState               *state);
GCancellable    *hif_state_get_cancellable              (HifState               *state);
gboolean         hif_state_get_allow_cancel             (HifState               *state);
guint64          hif_state_get_speed                    (HifState               *state);

/* setters */
void             hif_state_set_cancellable              (HifState               *state,
                                                         GCancellable           *cancellable);
void             hif_state_set_allow_cancel             (HifState               *state,
                                                         gboolean                allow_cancel);
void             hif_state_set_speed                    (HifState               *state,
                                                         guint64                 speed);
void             hif_state_set_report_progress          (HifState               *state,
                                                         gboolean                report_progress);
gboolean         hif_state_set_number_steps_real        (HifState               *state,
                                                         guint                   steps,
                                                         const gchar            *strloc);
gboolean         hif_state_set_steps_real               (HifState               *state,
                                                         GError                 **error,
                                                         const gchar            *strloc,
                                                         gint                    value, ...);
gboolean         hif_state_set_percentage               (HifState               *state,
                                                         guint                   percentage);
void             hif_state_set_package_progress         (HifState               *state,
                                                         const gchar            *package_id,
                                                         HifStateAction          action,
                                                         guint                   percentage);

/* object methods */
HifState        *hif_state_get_child                    (HifState               *state);
gboolean         hif_state_action_start                 (HifState               *state,
                                                         HifStateAction          action,
                                                         const gchar            *action_hint);
gboolean         hif_state_action_stop                  (HifState               *state);
gboolean         hif_state_check                        (HifState               *state,
                                                         GError                  **error)
                                                         G_GNUC_WARN_UNUSED_RESULT;
gboolean         hif_state_done_real                    (HifState               *state,
                                                         GError                  **error,
                                                         const gchar            *strloc)
                                                         G_GNUC_WARN_UNUSED_RESULT;
gboolean         hif_state_finished_real                (HifState               *state,
                                                         GError                  **error,
                                                         const gchar            *strloc)
                                                         G_GNUC_WARN_UNUSED_RESULT;
gboolean         hif_state_reset                        (HifState               *state);
void             hif_state_set_enable_profile           (HifState               *state,
                                                         gboolean                enable_profile);
#ifndef __GI_SCANNER__
gboolean         hif_state_take_lock                    (HifState               *state,
                                                         HifLockType             lock_type,
                                                         HifLockMode             lock_mode,
                                                         GError                 **error);
#endif
gboolean         hif_state_release_locks                (HifState               *state);

G_END_DECLS

#endif /* __HIF_STATE_H */
