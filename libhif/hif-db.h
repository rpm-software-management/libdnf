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

#if !defined (__LIBHIF_H) && !defined (HIF_COMPILATION)
#error "Only <libhif.h> can be included directly."
#endif

#ifndef __HIF_DB_H
#define __HIF_DB_H

#include <glib-object.h>

#include "hy-package.h"
#include "hif-context.h"

G_BEGIN_DECLS

#define HIF_TYPE_DB (hif_db_get_type ())
G_DECLARE_DERIVABLE_TYPE (HifDb, hif_db, HIF, DB, GObject)

struct _HifDbClass
{
        GObjectClass            parent_class;
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

HifDb           *hif_db_new                     (HifContext     *context);

void             hif_db_set_enabled             (HifDb          *db,
                                                 gboolean        enabled);

/* getters */
gchar           *hif_db_get_string              (HifDb          *db,
                                                 HyPackage       package,
                                                 const gchar    *key,
                                                 GError         **error);

/* setters */
gboolean         hif_db_set_string              (HifDb          *db,
                                                 HyPackage       package,
                                                 const gchar    *key,
                                                 const gchar    *value,
                                                 GError         **error);

/* object methods */
gboolean         hif_db_remove                  (HifDb          *db,
                                                 HyPackage       package,
                                                 const gchar    *key,
                                                 GError         **error);
gboolean         hif_db_remove_all              (HifDb          *db,
                                                 HyPackage       package,
                                                 GError         **error);
void             hif_db_ensure_origin_pkg       (HifDb          *db,
                                                 HyPackage       pkg);
void             hif_db_ensure_origin_pkglist   (HifDb          *db,
                                                 GPtrArray *  pkglist);

G_END_DECLS

#endif /* __HIF_DB_H */
