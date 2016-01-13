/*
 * Copyright (C) 2014 Red Hat, Inc.
 * Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __HIF_ADVISORY_H
#define __HIF_ADVISORY_H

#include <glib-object.h>

G_BEGIN_DECLS

#define HIF_TYPE_ADVISORY (hif_advisory_get_type ())
G_DECLARE_DERIVABLE_TYPE (HifAdvisory, hif_advisory, HIF, ADVISORY, GObject)

struct _HifAdvisoryClass
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

typedef enum {
        HIF_ADVISORY_KIND_UNKNOWN       = 0,        /* ordered by rough importance */
        HIF_ADVISORY_KIND_SECURITY      = 1,
        HIF_ADVISORY_KIND_BUGFIX        = 2,
        HIF_ADVISORY_KIND_ENHANCEMENT   = 3
} HifAdvisoryKind;

const char          *hif_advisory_get_title         (HifAdvisory *advisory);
const char          *hif_advisory_get_id            (HifAdvisory *advisory);
HifAdvisoryKind      hif_advisory_get_kind          (HifAdvisory *advisory);
const char          *hif_advisory_get_description   (HifAdvisory *advisory);
const char          *hif_advisory_get_rights        (HifAdvisory *advisory);
guint64              hif_advisory_get_updated       (HifAdvisory *advisory);
GPtrArray           *hif_advisory_get_packages      (HifAdvisory *advisory);
GPtrArray           *hif_advisory_get_references    (HifAdvisory *advisory);
int                  hif_advisory_compare           (HifAdvisory *left,
                                                     HifAdvisory *right);

G_END_DECLS

#endif /* __HIF_ADVISORY_H */
