/*
 * Copyright (C) 2012-2014 Red Hat, Inc.
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

#ifndef __HY_PACKAGE_H
#define __HY_PACKAGE_H

#include <glib-object.h>
#include "hy-types.h"
#include "dnf-packagedelta.h"
#include "dnf-reldep-list.h"

G_BEGIN_DECLS

#define DNF_TYPE_PACKAGE (dnf_package_get_type ())
G_DECLARE_DERIVABLE_TYPE (DnfPackage, dnf_package, DNF, PACKAGE, GObject)

struct _DnfPackageClass
{
        GObjectClass            parent_class;
        /*< private >*/
        void (*_dnf_reserved1)  (void);
        void (*_dnf_reserved2)  (void);
        void (*_dnf_reserved3)  (void);
        void (*_dnf_reserved4)  (void);
        void (*_dnf_reserved5)  (void);
        void (*_dnf_reserved6)  (void);
        void (*_dnf_reserved7)  (void);
        void (*_dnf_reserved8)  (void);
};

DnfPackage  *dnf_package_new            (DnfSack    *sack, Id id);

gboolean     dnf_package_get_identical  (DnfPackage *pkg1, DnfPackage *pkg2);
gboolean     dnf_package_installed      (DnfPackage *pkg);
int          dnf_package_cmp            (DnfPackage *pkg1, DnfPackage *pkg2);
int          dnf_package_evr_cmp        (DnfPackage *pkg1, DnfPackage *pkg2);

const char  *dnf_package_get_location   (DnfPackage *pkg);
const char  *dnf_package_get_baseurl    (DnfPackage *pkg);
const char  *dnf_package_get_nevra      (DnfPackage *pkg);
const char  *dnf_package_get_sourcerpm  (DnfPackage *pkg);
const char  *dnf_package_get_version    (DnfPackage *pkg);
const char  *dnf_package_get_release    (DnfPackage *pkg);

Id           dnf_package_get_id         (DnfPackage *pkg);
const char  *dnf_package_get_name       (DnfPackage *pkg);
const char  *dnf_package_get_arch       (DnfPackage *pkg);
const unsigned char *dnf_package_get_chksum(DnfPackage *pkg, int *type);
const char  *dnf_package_get_description(DnfPackage *pkg);
const char  *dnf_package_get_evr        (DnfPackage *pkg);
const char  *dnf_package_get_group      (DnfPackage *pkg);
const char  *dnf_package_get_license    (DnfPackage *pkg);
const unsigned char *dnf_package_get_hdr_chksum(DnfPackage *pkg, int *type);
const char  *dnf_package_get_packager   (DnfPackage *pkg);
const char  *dnf_package_get_reponame   (DnfPackage *pkg);
const char  *dnf_package_get_summary    (DnfPackage *pkg);
const char  *dnf_package_get_url        (DnfPackage *pkg);
guint64      dnf_package_get_downloadsize(DnfPackage *pkg);
guint64      dnf_package_get_epoch      (DnfPackage *pkg);
guint64      dnf_package_get_hdr_end    (DnfPackage *pkg);
guint64      dnf_package_get_installsize(DnfPackage *pkg);
guint64      dnf_package_get_medianr    (DnfPackage *pkg);
guint64      dnf_package_get_rpmdbid    (DnfPackage *pkg);
guint64      dnf_package_get_size       (DnfPackage *pkg);
guint64      dnf_package_get_buildtime  (DnfPackage *pkg);
guint64      dnf_package_get_installtime(DnfPackage *pkg);

DnfReldepList *dnf_package_get_conflicts    (DnfPackage *pkg);
DnfReldepList *dnf_package_get_enhances     (DnfPackage *pkg);
DnfReldepList *dnf_package_get_obsoletes    (DnfPackage *pkg);
DnfReldepList *dnf_package_get_provides     (DnfPackage *pkg);
DnfReldepList *dnf_package_get_recommends   (DnfPackage *pkg);
DnfReldepList *dnf_package_get_requires     (DnfPackage *pkg);
DnfReldepList *dnf_package_get_requires_pre (DnfPackage *pkg);
DnfReldepList *dnf_package_get_suggests     (DnfPackage *pkg);
DnfReldepList *dnf_package_get_supplements  (DnfPackage *pkg);
char       **dnf_package_get_files      (DnfPackage *pkg);
GPtrArray   *dnf_package_get_advisories (DnfPackage *pkg, int cmp_type);

DnfPackageDelta *dnf_package_get_delta_from_evr(DnfPackage *pkg, const char *from_evr);

G_END_DECLS

#endif /* __HY_PACKAGE_H */
