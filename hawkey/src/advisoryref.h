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

#ifndef HY_ADVISORYREF_H
#define HY_ADVISORYREF_H

#ifdef __cplusplus
extern "C" {
#endif

/* hawkey */
#include "types.h"

typedef enum {
	HY_REFERENCE_UNKNOWN = 0,
	HY_REFERENCE_BUGZILLA = 1,
	HY_REFERENCE_CVE = 2,
	HY_REFERENCE_VENDOR = 3
} HyAdvisoryRefType;

void hy_advisoryref_free(HyAdvisoryRef advisoryref);
HyAdvisoryRefType hy_advisoryref_get_type(HyAdvisoryRef advisoryref);
const char *hy_advisoryref_get_id(HyAdvisoryRef advisoryref);
const char *hy_advisoryref_get_title(HyAdvisoryRef advisoryref);
const char *hy_advisoryref_get_url(HyAdvisoryRef advisoryref);

void hy_advisoryreflist_free(HyAdvisoryRefList reflist);
int hy_advisoryreflist_count(HyAdvisoryRefList reflist);
HyAdvisoryRef hy_advisoryreflist_get_clone(HyAdvisoryRefList reflist, int index);

#ifdef __cplusplus
}
#endif

#endif /* HY_ADVISORYREF_H */
