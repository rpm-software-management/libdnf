/*
 * Copyright (C) 2014 Red Hat, Inc.
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

#ifndef HY_ADVISORYPKG_INTERNAL_H
#define HY_ADVISORYPKG_INTERNAL_H

// hawkey
#include "advisorypkg.h"

HyAdvisoryPkg advisorypkg_create(void);
void advisorypkg_set_string(HyAdvisoryPkg advisorypkg, int which, const char* str_val);
HyAdvisoryPkg advisorypkg_clone(HyAdvisoryPkg advisorypkg);
int advisorypkg_identical(HyAdvisoryPkg left, HyAdvisoryPkg right);

HyAdvisoryPkgList advisorypkglist_create(void);
void advisorypkglist_add(HyAdvisoryPkgList pkglist, HyAdvisoryPkg advisorypkg);

#endif // HY_ADVISORYPKG_INTERNAL_H
