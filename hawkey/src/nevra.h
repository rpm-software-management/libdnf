/*
 * Copyright (C) 2013 Red Hat, Inc.
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

#ifndef HY_NEVRA_H
#define HY_NEVRA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

enum _hy_nevra_param_e {
    HY_NEVRA_NAME = 0,
    HY_NEVRA_VERSION = 1,
    HY_NEVRA_RELEASE = 2,
    HY_NEVRA_ARCH = 3
};

HyNevra hy_nevra_create();
void hy_nevra_free(HyNevra nevra);
HyNevra hy_nevra_clone(HyNevra nevra);
int hy_nevra_cmp(HyNevra nevra1, HyNevra nevra2);
const char *hy_nevra_get_string(HyNevra nevra, int which);
int hy_nevra_get_epoch(HyNevra nevra);
int hy_nevra_set_epoch(HyNevra nevra, int epoch);
void hy_nevra_set_string(HyNevra nevra, int which, const char* str_val);
HyQuery hy_nevra_to_query(HyNevra nevra, HySack sack);
int hy_nevra_evr_cmp(HyNevra nevra1, HyNevra nevra2, HySack sack);
char *hy_nevra_get_evr(HyNevra nevra);

#ifdef __cplusplus
}
#endif

#endif
