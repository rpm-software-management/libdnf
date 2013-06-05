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

#ifndef HY_RELDEP_H
#define HY_RELDEP_H

#ifdef __cplusplus
extern "C" {
#endif

/* hawkey */
#include "types.h"

HyReldep hy_reldep_create(HySack sack, const char *name, int cmp_type,
			  const char *evr);
void hy_reldep_free(HyReldep reldep);
HyReldep hy_reldep_clone(HyReldep reldep);
char *hy_reldep_str(HyReldep reldep);

HyReldepList hy_reldeplist_create(HySack sack);
void hy_reldeplist_free(HyReldepList reldeplist);
void hy_reldeplist_add(HyReldepList reldeplist, HyReldep reldep);
int hy_reldeplist_count(HyReldepList reldeplist);
HyReldep hy_reldeplist_get_clone(HyReldepList reldeplist, int index);

#ifdef __cplusplus
}
#endif

#endif /* HY_RELDEP_H */
