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

#ifndef HY_ADVISORY_H
#define HY_ADVISORY_H

#ifdef __cplusplus
extern "C" {
#endif

/* hawkey */
#include "types.h"

typedef enum {
	HY_ADVISORY_UNKNOWN = 0,	/* ordered by rough importance */
	HY_ADVISORY_SECURITY = 1,
	HY_ADVISORY_BUGFIX = 2,
	HY_ADVISORY_ENHANCEMENT = 3
} HyAdvisoryType;

void hy_advisory_free(HyAdvisory advisory);
const char *hy_advisory_get_title(HyAdvisory advisory);
const char *hy_advisory_get_id(HyAdvisory advisory);
HyAdvisoryType hy_advisory_get_type(HyAdvisory advisory);
const char *hy_advisory_get_description(HyAdvisory advisory);
const char *hy_advisory_get_rights(HyAdvisory advisory);
unsigned long long hy_advisory_get_updated(HyAdvisory advisory);
HyAdvisoryPkgList hy_advisory_get_packages(HyAdvisory advisory);
HyAdvisoryRefList hy_advisory_get_references(HyAdvisory advisory);

void hy_advisorylist_free(HyAdvisoryList advisorylist);
int hy_advisorylist_count(HyAdvisoryList advisorylist);
HyAdvisory hy_advisorylist_get_clone(HyAdvisoryList advisorylist, int index);

// deprecated in 0.4.18, eligible for dropping after 2014-10-15 AND no sooner
// than in 0.4.21, use hy_advisorypkg_get_string instead
HyStringArray hy_advisory_get_filenames(HyAdvisory advisory);

#ifdef __cplusplus
}
#endif

#endif /* HY_ADVISORY_H */
