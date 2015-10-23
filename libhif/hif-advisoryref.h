/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
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

#ifndef __HIF_ADVISORYREF_H
#define __HIF_ADVISORYREF_H

#include <glib-object.h>

G_BEGIN_DECLS

#define HIF_TYPE_ADVISORYREF (hif_advisoryref_get_type ())
G_DECLARE_DERIVABLE_TYPE (HifAdvisoryRef, hif_advisoryref, HIF, ADVISORYREF, GObject)

struct _HifAdvisoryRefClass
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
        HIF_REFERENCE_KIND_UNKNOWN    = 0,
        HIF_REFERENCE_KIND_BUGZILLA   = 1,
        HIF_REFERENCE_KIND_CVE        = 2,
        HIF_REFERENCE_KIND_VENDOR     = 3
} HifAdvisoryRefKind;

HifAdvisoryRefKind       hif_advisoryref_get_kind       (HifAdvisoryRef *advisoryref);
const char              *hif_advisoryref_get_id         (HifAdvisoryRef *advisoryref);
const char              *hif_advisoryref_get_title      (HifAdvisoryRef *advisoryref);
const char              *hif_advisoryref_get_url        (HifAdvisoryRef *advisoryref);
int                      hif_advisoryref_compare        (HifAdvisoryRef *left,
                                                         HifAdvisoryRef *right);

G_END_DECLS

#endif /* __HIF_ADVISORYREF_H */
