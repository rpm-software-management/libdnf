/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2010-2015 Richard Hughes <richard@hughsie.com>
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

#ifndef __DNF_DB_H
#define __DNF_DB_H

#include <glib-object.h>

#include "hy-package.h"
#include "dnf-context.h"

G_BEGIN_DECLS

#define DNF_TYPE_DB (dnf_db_get_type ())
G_DECLARE_DERIVABLE_TYPE (DnfDb, dnf_db, DNF, DB, GObject)

struct _DnfDbClass
{
        GObjectClass            parent_class;
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

DnfDb           *dnf_db_new                     (DnfContext     *context);

void             dnf_db_set_enabled             (DnfDb          *db,
                                                 gboolean        enabled);

/* getters */
gchar           *dnf_db_get_string              (DnfDb          *db,
                                                 DnfPackage *      package,
                                                 const gchar    *key,
                                                 GError         **error);

/* setters */
gboolean         dnf_db_set_string              (DnfDb          *db,
                                                 DnfPackage *      package,
                                                 const gchar    *key,
                                                 const gchar    *value,
                                                 GError         **error);

/* object methods */
gboolean         dnf_db_remove                  (DnfDb          *db,
                                                 DnfPackage *      package,
                                                 const gchar    *key,
                                                 GError         **error);
gboolean         dnf_db_remove_all              (DnfDb          *db,
                                                 DnfPackage *      package,
                                                 GError         **error);
void             dnf_db_ensure_origin_pkg       (DnfDb          *db,
                                                 DnfPackage *      pkg);
void             dnf_db_ensure_origin_pkglist   (DnfDb          *db,
                                                 GPtrArray *  pkglist);

G_END_DECLS

#endif /* __DNF_DB_H */
