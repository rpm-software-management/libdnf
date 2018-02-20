/*
 * Copyright (C) 2014-2018 Red Hat, Inc.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __DNF_ADVISORY_H
#define __DNF_ADVISORY_H

#include <glib-object.h>

#ifdef __cplusplus
namespace libdnf {
    struct Advisory;
}
typedef struct libdnf::Advisory DnfAdvisory;
#else
typedef struct Advisory DnfAdvisory;
#endif

typedef enum {
        DNF_ADVISORY_KIND_UNKNOWN       = 0,        /* ordered by rough importance */
        DNF_ADVISORY_KIND_SECURITY      = 1,
        DNF_ADVISORY_KIND_BUGFIX        = 2,
        DNF_ADVISORY_KIND_ENHANCEMENT   = 3,
        DNF_ADVISORY_KIND_NEWPACKAGE    = 4
} DnfAdvisoryKind;

#ifdef __cplusplus
extern "C" {
#endif

void dnf_advisory_free(DnfAdvisory *advisory);
const char          *dnf_advisory_get_title         (DnfAdvisory *advisory);
const char          *dnf_advisory_get_id            (DnfAdvisory *advisory);
DnfAdvisoryKind      dnf_advisory_get_kind          (DnfAdvisory *advisory);
const char          *dnf_advisory_get_description   (DnfAdvisory *advisory);
const char          *dnf_advisory_get_rights        (DnfAdvisory *advisory);
const char          *dnf_advisory_get_severity      (DnfAdvisory *advisory);
guint64              dnf_advisory_get_updated       (DnfAdvisory *advisory);
GPtrArray           *dnf_advisory_get_packages      (DnfAdvisory *advisory);
GPtrArray           *dnf_advisory_get_references    (DnfAdvisory *advisory);
int                  dnf_advisory_compare           (DnfAdvisory *left,
                                                     DnfAdvisory *right);
gboolean             dnf_advisory_match_id          (DnfAdvisory *advisory,
                                                     const char  *s);
gboolean             dnf_advisory_match_kind        (DnfAdvisory *advisory,
                                                     const char  *s);
gboolean             dnf_advisory_match_severity    (DnfAdvisory *advisory,
                                                     const char  *s);
gboolean             dnf_advisory_match_cve         (DnfAdvisory *advisory,
                                                     const char  *s);
gboolean             dnf_advisory_match_bug         (DnfAdvisory *advisory,
                                                     const char  *s);

#ifdef __cplusplus
}
#endif

#endif /* __DNF_ADVISORY_H */
