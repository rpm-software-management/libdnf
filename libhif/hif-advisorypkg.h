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

#ifndef __HIF_ADVISORYPKG_H
#define __HIF_ADVISORYPKG_H

#include <solv/pool.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define HIF_TYPE_ADVISORY_PKG (hif_advisorypkg_get_type ())
G_DECLARE_DERIVABLE_TYPE (HifAdvisoryPkg, hif_advisorypkg, HIF, ADVISORY_PKG, GObject)

struct _HifAdvisoryPkgClass
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

const char      *hif_advisorypkg_get_name         (HifAdvisoryPkg *advisorypkg);
const char      *hif_advisorypkg_get_evr          (HifAdvisoryPkg *advisorypkg);
const char      *hif_advisorypkg_get_arch         (HifAdvisoryPkg *advisorypkg);
const char      *hif_advisorypkg_get_filename     (HifAdvisoryPkg *advisorypkg);

int              hif_advisorypkg_compare          (HifAdvisoryPkg *left,
                                                   HifAdvisoryPkg *right);
gboolean         hif_advisorypkg_compare_solvable (HifAdvisoryPkg *advisorypkg,
                                                   Pool           *pool,
                                                   Solvable       *s);

G_END_DECLS

#endif /* __HIF_ADVISORYPKG_H */
