/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2014 Red Hat, Inc.
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

#ifndef __DNF_ADVISORYREF_H
#define __DNF_ADVISORYREF_H

#ifdef __cplusplus
namespace libdnf {
    struct AdvisoryRef;
}
typedef struct libdnf::AdvisoryRef DnfAdvisoryRef;
#else
typedef struct AdvisoryRef DnfAdvisoryRef;
#endif


typedef enum {
        DNF_REFERENCE_KIND_UNKNOWN    = 0,
        DNF_REFERENCE_KIND_BUGZILLA   = 1,
        DNF_REFERENCE_KIND_CVE        = 2,
        DNF_REFERENCE_KIND_VENDOR     = 3
} DnfAdvisoryRefKind;

#ifdef __cplusplus
extern "C" {
#endif

void dnf_advisoryref_free(DnfAdvisoryRef *advisoryref);
DnfAdvisoryRefKind       dnf_advisoryref_get_kind       (DnfAdvisoryRef *advisoryref);
const char              *dnf_advisoryref_get_id         (DnfAdvisoryRef *advisoryref);
const char              *dnf_advisoryref_get_title      (DnfAdvisoryRef *advisoryref);
const char              *dnf_advisoryref_get_url        (DnfAdvisoryRef *advisoryref);
int                      dnf_advisoryref_compare        (DnfAdvisoryRef *left,
                                                         DnfAdvisoryRef *right);

#ifdef __cplusplus
}
#endif

#endif /* __DNF_ADVISORYREF_H */
