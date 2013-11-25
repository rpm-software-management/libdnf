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

#ifndef HY_PACKAGE_INTERNAL_H
#define HY_PACKAGE_INTERNAL_H

// libsolv
#include <solv/pool.h>
#include <solv/solvable.h>

// hawkey
#include "package.h"

struct _HyPackage {
    int nrefs;
    Id id;
    HySack sack;
    void *userdata;
    HyUserdataDestroy destroy_func;
};

struct _HyPackageDelta {
    char *location;
    char *baseurl;
    unsigned long long downloadsize;
    int checksum_type;
    unsigned char *checksum;
};

HyPackage package_clone(HyPackage pkg);
HyPackage package_create(HySack sack, Id id);
static inline Id package_id(HyPackage pkg) { return pkg->id; }
Pool *package_pool(HyPackage pkg);
static inline HySack package_sack(HyPackage pkg) { return pkg->sack; }
HyPackage package_from_solvable(HySack sack, Solvable *s);
HyPackageDelta delta_create(void);

#endif // HY_PACKAGE_INTERNAL_H
