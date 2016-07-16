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

#ifndef __DNF_STATE_H
#define __DNF_STATE_H

#include <glib-object.h>
#include <gio/gio.h>

#include "dnf-types.h"
#include "dnf-lock.h"

G_BEGIN_DECLS

#define DNF_TYPE_STATE (dnf_state_get_type ())
G_DECLARE_DERIVABLE_TYPE (DnfState, dnf_state, DNF, STATE, GObject)

/**
 * DnfStateAction:
 * @DNF_STATE_ACTION_UNKNOWN:                   Unknown status
 * @DNF_STATE_ACTION_DOWNLOAD_PACKAGES:         Downloading packages
 * @DNF_STATE_ACTION_DOWNLOAD_METADATA:         Downloading metadata
 * @DNF_STATE_ACTION_LOADING_CACHE:             Loading cache
 * @DNF_STATE_ACTION_TEST_COMMIT:               Testing transaction
 * @DNF_STATE_ACTION_REQUEST:                   Requesting data
 * @DNF_STATE_ACTION_REMOVE:                    Removing packages
 * @DNF_STATE_ACTION_INSTALL:                   Installing packages
 * @DNF_STATE_ACTION_UPDATE:                    Updating packages
 * @DNF_STATE_ACTION_CLEANUP:                   Cleaning packages
 * @DNF_STATE_ACTION_OBSOLETE:                  Obsoleting packages
 * @DNF_STATE_ACTION_REINSTALL:                 Reinstall packages
 * @DNF_STATE_ACTION_DOWNGRADE:                 Downgrading packages
 * @DNF_STATE_ACTION_QUERY:                     Querying for results
 *
 * The action enum code.
 **/
typedef enum {
        DNF_STATE_ACTION_UNKNOWN                = 0,    /* Since: 0.1.0 */
        DNF_STATE_ACTION_DOWNLOAD_PACKAGES      = 8,    /* Since: 0.1.0 */
        DNF_STATE_ACTION_DOWNLOAD_METADATA      = 20,   /* Since: 0.1.0 */
        DNF_STATE_ACTION_LOADING_CACHE          = 27,   /* Since: 0.1.0 */
        DNF_STATE_ACTION_TEST_COMMIT            = 15,   /* Since: 0.1.0 */
        DNF_STATE_ACTION_REQUEST                = 17,   /* Since: 0.1.0 */
        DNF_STATE_ACTION_REMOVE                 = 6,    /* Since: 0.1.0 */
        DNF_STATE_ACTION_INSTALL                = 9,    /* Since: 0.1.0 */
        DNF_STATE_ACTION_UPDATE                 = 10,   /* Since: 0.1.2 */
        DNF_STATE_ACTION_CLEANUP                = 11,   /* Since: 0.1.2 */
        DNF_STATE_ACTION_OBSOLETE               = 12,   /* Since: 0.1.2 */
        DNF_STATE_ACTION_REINSTALL              = 13,   /* Since: 0.1.6 */
        DNF_STATE_ACTION_DOWNGRADE              = 14,   /* Since: 0.1.6 */
        DNF_STATE_ACTION_QUERY                  = 4,    /* Since: 0.1.2 */
        /*< private >*/
        DNF_STATE_ACTION_LAST
} DnfStateAction;

struct _DnfStateClass
{
        GObjectClass    parent_class;
        void            (* percentage_changed)          (DnfState       *state,
                                                         guint           value);
        void            (* allow_cancel_changed)        (DnfState       *state,
                                                         gboolean        allow_cancel);
        void            (* action_changed)              (DnfState       *state,
                                                         DnfStateAction  action,
                                                         const gchar    *action_hint);
        void            (* package_progress_changed)    (DnfState       *state,
                                                         const gchar    *dnf_package_get_id,
                                                         DnfStateAction  action,
                                                         guint           percentage);
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

#define dnf_state_done(state, error)                    dnf_state_done_real(state, error, G_STRLOC)
#define dnf_state_finished(state, error)                dnf_state_finished_real(state, error, G_STRLOC)
#define dnf_state_set_number_steps(state, steps)        dnf_state_set_number_steps_real(state, steps, G_STRLOC)
#define dnf_state_set_steps(state, error, value, args...)       dnf_state_set_steps_real(state, error, G_STRLOC, value, ## args)

typedef gboolean (*DnfStateErrorHandlerCb)              (const GError           *error,
                                                         gpointer                user_data);

DnfState        *dnf_state_new                          (void);

/* getters */
guint            dnf_state_get_percentage               (DnfState               *state);
DnfStateAction   dnf_state_get_action                   (DnfState               *state);
const gchar     *dnf_state_get_action_hint              (DnfState               *state);
GCancellable    *dnf_state_get_cancellable              (DnfState               *state);
gboolean         dnf_state_get_allow_cancel             (DnfState               *state);
guint64          dnf_state_get_speed                    (DnfState               *state);

/* setters */
void             dnf_state_set_cancellable              (DnfState               *state,
                                                         GCancellable           *cancellable);
void             dnf_state_set_allow_cancel             (DnfState               *state,
                                                         gboolean                allow_cancel);
void             dnf_state_set_speed                    (DnfState               *state,
                                                         guint64                 speed);
void             dnf_state_set_report_progress          (DnfState               *state,
                                                         gboolean                report_progress);
gboolean         dnf_state_set_number_steps_real        (DnfState               *state,
                                                         guint                   steps,
                                                         const gchar            *strloc);
gboolean         dnf_state_set_steps_real               (DnfState               *state,
                                                         GError                 **error,
                                                         const gchar            *strloc,
                                                         gint                    value, ...);
gboolean         dnf_state_set_percentage               (DnfState               *state,
                                                         guint                   percentage);
void             dnf_state_set_package_progress         (DnfState               *state,
                                                         const gchar            *dnf_package_get_id,
                                                         DnfStateAction          action,
                                                         guint                   percentage);

/* object methods */
DnfState        *dnf_state_get_child                    (DnfState               *state);
gboolean         dnf_state_action_start                 (DnfState               *state,
                                                         DnfStateAction          action,
                                                         const gchar            *action_hint);
gboolean         dnf_state_action_stop                  (DnfState               *state);
gboolean         dnf_state_check                        (DnfState               *state,
                                                         GError                  **error)
                                                         G_GNUC_WARN_UNUSED_RESULT;
gboolean         dnf_state_done_real                    (DnfState               *state,
                                                         GError                  **error,
                                                         const gchar            *strloc)
                                                         G_GNUC_WARN_UNUSED_RESULT;
gboolean         dnf_state_finished_real                (DnfState               *state,
                                                         GError                  **error,
                                                         const gchar            *strloc)
                                                         G_GNUC_WARN_UNUSED_RESULT;
gboolean         dnf_state_reset                        (DnfState               *state);
void             dnf_state_set_enable_profile           (DnfState               *state,
                                                         gboolean                enable_profile);
#ifndef __GI_SCANNER__
gboolean         dnf_state_take_lock                    (DnfState               *state,
                                                         DnfLockType             lock_type,
                                                         DnfLockMode             lock_mode,
                                                         GError                 **error);
#endif
gboolean         dnf_state_release_locks                (DnfState               *state);

G_END_DECLS

#endif /* __DNF_STATE_H */
