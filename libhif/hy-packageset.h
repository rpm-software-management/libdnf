/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2012-2013 Red Hat, Inc.
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

#ifndef __HIF_PACKAGESET_H
#define __HIF_PACKAGESET_H

#include "hif-sack.h"
#include "hy-types.h"
#include "hy-package.h"

G_BEGIN_DECLS

#define HIF_TYPE_PACKAGE_SET (hif_packageset_get_type ())
G_DECLARE_DERIVABLE_TYPE (HifPackageSet, hif_packageset, HIF, PACKAGE_SET, GObject)

struct _HifPackageSetClass
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

HifPackageSet       *hif_packageset_new         (HifSack *sack);
HifPackageSet       *hif_packageset_clone       (HifPackageSet *pset);
void                 hif_packageset_add         (HifPackageSet *pset, HifPackage *pkg);
unsigned             hif_packageset_count       (HifPackageSet *pset);
HifPackage          *hif_packageset_get_clone   (HifPackageSet *pset, int index);
int                  hif_packageset_has         (HifPackageSet *pset, HifPackage *pkg);

G_END_DECLS

#endif /* __HIF_PACKAGESET_H */
