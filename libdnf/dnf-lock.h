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

#ifndef __DNF_LOCK_H
#define __DNF_LOCK_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DNF_TYPE_LOCK (dnf_lock_get_type ())
G_DECLARE_DERIVABLE_TYPE (DnfLock, dnf_lock, DNF, LOCK, GObject)

struct _DnfLockClass
{
        GObjectClass            parent_class;
        void                    (* state_changed)       (DnfLock        *lock,
                                                         guint           state_bitfield);
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
 * DnfLockType:
 * @DNF_LOCK_TYPE_RPMDB:                        The rpmdb lock
 * @DNF_LOCK_TYPE_REPO:                         The repodir lock
 * @DNF_LOCK_TYPE_METADATA:                     The metadata lock
 * @DNF_LOCK_TYPE_CONFIG:                       The config lock
 *
 * The lock type.
 **/
typedef enum {
        DNF_LOCK_TYPE_RPMDB,
        DNF_LOCK_TYPE_REPO,
        DNF_LOCK_TYPE_METADATA,
        DNF_LOCK_TYPE_CONFIG,
        /*< private >*/
        DNF_LOCK_TYPE_LAST
} DnfLockType;

/**
 * DnfLockMode:
 * @DNF_LOCK_MODE_THREAD:                       For all threads in this process
 * @DNF_LOCK_MODE_PROCESS:                      For all processes
 *
 * The lock mode.
 **/
typedef enum {
        DNF_LOCK_MODE_THREAD,
        DNF_LOCK_MODE_PROCESS,
        /*< private >*/
        DNF_LOCK_MODE_LAST
} DnfLockMode;

DnfLock         *dnf_lock_new                   (void);

/* getters */
guint            dnf_lock_get_state             (DnfLock        *lock);
void             dnf_lock_set_lock_dir          (DnfLock        *lock,
                                                 const gchar    *lock_dir);

/* object methods */
guint            dnf_lock_take                  (DnfLock        *lock,
                                                 DnfLockType     type,
                                                 DnfLockMode     mode,
                                                 GError         **error);
gboolean         dnf_lock_release               (DnfLock        *lock,
                                                 guint           id,
                                                 GError         **error);
void             dnf_lock_release_noerror       (DnfLock        *lock,
                                                 guint           id);
const gchar     *dnf_lock_type_to_string        (DnfLockType     lock_type);

G_END_DECLS

#endif /* __DNF_LOCK_H */
