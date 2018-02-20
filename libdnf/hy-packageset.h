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

#ifndef __DNF_PACKAGESET_H
#define __DNF_PACKAGESET_H

#include "dnf-sack.h"
#include "hy-types.h"

#ifdef __cplusplus
extern "C" {
#endif

DnfPackageSet       *dnf_packageset_new         (DnfSack *sack);
void                 dnf_packageset_free        (DnfPackageSet *pset);
DnfPackageSet       *dnf_packageset_clone       (DnfPackageSet *pset);
void                 dnf_packageset_add         (DnfPackageSet *pset, DnfPackage *pkg);
size_t               dnf_packageset_count       (DnfPackageSet *pset);
int                  dnf_packageset_has         (DnfPackageSet *pset, DnfPackage *pkg);

DnfPackageSet       *dnf_packageset_from_bitmap (DnfSack *sack, Map *m);
Map             *dnf_packageset_get_map        (DnfPackageSet *pset);

#ifdef __cplusplus
}
#endif

#endif /* __DNF_PACKAGESET_H */
