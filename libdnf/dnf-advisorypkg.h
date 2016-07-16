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

#ifndef __DNF_ADVISORYPKG_H
#define __DNF_ADVISORYPKG_H

#include <solv/pool.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define DNF_TYPE_ADVISORY_PKG (dnf_advisorypkg_get_type ())
G_DECLARE_DERIVABLE_TYPE (DnfAdvisoryPkg, dnf_advisorypkg, DNF, ADVISORY_PKG, GObject)

struct _DnfAdvisoryPkgClass
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

const char      *dnf_advisorypkg_get_name         (DnfAdvisoryPkg *advisorypkg);
const char      *dnf_advisorypkg_get_evr          (DnfAdvisoryPkg *advisorypkg);
const char      *dnf_advisorypkg_get_arch         (DnfAdvisoryPkg *advisorypkg);
const char      *dnf_advisorypkg_get_filename     (DnfAdvisoryPkg *advisorypkg);

int              dnf_advisorypkg_compare          (DnfAdvisoryPkg *left,
                                                   DnfAdvisoryPkg *right);
gboolean         dnf_advisorypkg_compare_solvable (DnfAdvisoryPkg *advisorypkg,
                                                   Pool           *pool,
                                                   Solvable       *s);

G_END_DECLS

#endif /* __DNF_ADVISORYPKG_H */
