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

#ifndef __DNF_ADVISORYREF_H
#define __DNF_ADVISORYREF_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DNF_TYPE_ADVISORYREF (dnf_advisoryref_get_type ())
G_DECLARE_DERIVABLE_TYPE (DnfAdvisoryRef, dnf_advisoryref, DNF, ADVISORYREF, GObject)

struct _DnfAdvisoryRefClass
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

typedef enum {
        DNF_REFERENCE_KIND_UNKNOWN    = 0,
        DNF_REFERENCE_KIND_BUGZILLA   = 1,
        DNF_REFERENCE_KIND_CVE        = 2,
        DNF_REFERENCE_KIND_VENDOR     = 3
} DnfAdvisoryRefKind;

DnfAdvisoryRefKind       dnf_advisoryref_get_kind       (DnfAdvisoryRef *advisoryref);
const char              *dnf_advisoryref_get_id         (DnfAdvisoryRef *advisoryref);
const char              *dnf_advisoryref_get_title      (DnfAdvisoryRef *advisoryref);
const char              *dnf_advisoryref_get_url        (DnfAdvisoryRef *advisoryref);
int                      dnf_advisoryref_compare        (DnfAdvisoryRef *left,
                                                         DnfAdvisoryRef *right);

G_END_DECLS

#endif /* __DNF_ADVISORYREF_H */
