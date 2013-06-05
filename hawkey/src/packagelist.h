/*
 * Copyright (C) 2012-2013 Red Hat, Inc.
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

#ifndef HY_PACKAGELIST_H
#define HY_PACKAGELIST_H

#ifdef __cplusplus
extern "C" {
#endif

// hawkey
#include "types.h"

HyPackageList hy_packagelist_create(void);
void hy_packagelist_free(HyPackageList plist);
int hy_packagelist_count(HyPackageList plist);
HyPackage hy_packagelist_get(HyPackageList plist, int index);
HyPackage hy_packagelist_get_clone(HyPackageList plist, int index);
int hy_packagelist_has(HyPackageList plist, HyPackage pkg);
void hy_packagelist_push(HyPackageList plist, HyPackage pkg);

#define FOR_PACKAGELIST(pkg, pkglist, i)						\
    for (i = 0; (pkg = hy_packagelist_get(pkglist, i)) != NULL; ++i)

#ifdef __cplusplus
}
#endif

#endif /* HY_PACKAGELIST_H */
