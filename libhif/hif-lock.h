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

#ifndef __HIF_LOCK_H
#define __HIF_LOCK_H

#include <glib-object.h>

G_BEGIN_DECLS

#define HIF_TYPE_LOCK (hif_lock_get_type ())
G_DECLARE_DERIVABLE_TYPE (HifLock, hif_lock, HIF, LOCK, GObject)

struct _HifLockClass
{
        GObjectClass            parent_class;
        void                    (* state_changed)       (HifLock        *lock,
                                                         guint           state_bitfield);
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
 * HifLockType:
 * @HIF_LOCK_TYPE_RPMDB:                        The rpmdb lock
 * @HIF_LOCK_TYPE_REPO:                         The repodir lock
 * @HIF_LOCK_TYPE_METADATA:                     The metadata lock
 * @HIF_LOCK_TYPE_CONFIG:                       The config lock
 *
 * The lock type.
 **/
typedef enum {
        HIF_LOCK_TYPE_RPMDB,
        HIF_LOCK_TYPE_REPO,
        HIF_LOCK_TYPE_METADATA,
        HIF_LOCK_TYPE_CONFIG,
        /*< private >*/
        HIF_LOCK_TYPE_LAST
} HifLockType;

/**
 * HifLockMode:
 * @HIF_LOCK_MODE_THREAD:                       For all threads in this process
 * @HIF_LOCK_MODE_PROCESS:                      For all processes
 *
 * The lock mode.
 **/
typedef enum {
        HIF_LOCK_MODE_THREAD,
        HIF_LOCK_MODE_PROCESS,
        /*< private >*/
        HIF_LOCK_MODE_LAST
} HifLockMode;

HifLock         *hif_lock_new                   (void);

/* getters */
guint            hif_lock_get_state             (HifLock        *lock);
void             hif_lock_set_lock_dir          (HifLock        *lock,
                                                 const gchar    *lock_dir);

/* object methods */
guint            hif_lock_take                  (HifLock        *lock,
                                                 HifLockType     type,
                                                 HifLockMode     mode,
                                                 GError         **error);
gboolean         hif_lock_release               (HifLock        *lock,
                                                 guint           id,
                                                 GError         **error);
void             hif_lock_release_noerror       (HifLock        *lock,
                                                 guint           id);
const gchar     *hif_lock_type_to_string        (HifLockType     lock_type);

G_END_DECLS

#endif /* __HIF_LOCK_H */
