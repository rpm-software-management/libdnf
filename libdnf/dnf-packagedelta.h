/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2012-2014 Red Hat, Inc.
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

#ifndef __DNF_PACKAGEDELTA_H
#define __DNF_PACKAGEDELTA_H

#include <glib-object.h>

#include <solv/pool.h>

G_BEGIN_DECLS

#define DNF_TYPE_PACKAGEDELTA (dnf_packagedelta_get_type ())
G_DECLARE_DERIVABLE_TYPE (DnfPackageDelta, dnf_packagedelta, DNF, PACKAGEDELTA, GObject)

struct _DnfPackageDeltaClass
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

const char      *dnf_packagedelta_get_location          (DnfPackageDelta *delta);
const char      *dnf_packagedelta_get_baseurl           (DnfPackageDelta *delta);
guint64          dnf_packagedelta_get_downloadsize      (DnfPackageDelta *delta);
const unsigned char *dnf_packagedelta_get_chksum        (DnfPackageDelta *delta, int *type);

G_END_DECLS

#endif /* __DNF_PACKAGEDELTA_H */
