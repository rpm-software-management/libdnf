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

#ifndef HY_SELECTOR_H
#define HY_SELECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

HySelector hy_selector_create(HySack sack);
void hy_selector_free(HySelector sltr);
int hy_selector_set(HySelector sltr, int keyname, int cmp_type,
		    const char *match);
HyPackageList hy_selector_matches(HySelector sltr);

#ifdef __cplusplus
}
#endif

#endif
