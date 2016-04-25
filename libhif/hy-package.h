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
#include "hif-packagedelta.h"
#include "hif-reldep-list.h"

G_BEGIN_DECLS

#define HIF_TYPE_PACKAGE (hif_package_get_type ())
G_DECLARE_DERIVABLE_TYPE (HifPackage, hif_package, HIF, PACKAGE, GObject)

struct _HifPackageClass
{
        GObjectClass            parent_class;
        /*< private >*/
        void (*_hif_reserved1)  (void);
        void (*_hif_reserved2)  (void);
        void (*_hif_reserved3)  (void);
        void (*_hif_reserved4)  (void);
        void (*_hif_reserved5)  (void);
        void (*_hif_reserved6)  (void);
        void (*_hif_reserved7)  (void);
        void (*_hif_reserved8)  (void);
};

gboolean     hif_package_get_identical  (HifPackage *pkg1, HifPackage *pkg2);
gboolean     hif_package_installed      (HifPackage *pkg);
int          hif_package_cmp            (HifPackage *pkg1, HifPackage *pkg2);
int          hif_package_evr_cmp        (HifPackage *pkg1, HifPackage *pkg2);

char        *hif_package_get_location   (HifPackage *pkg);
const char  *hif_package_get_baseurl    (HifPackage *pkg);
char        *hif_package_get_nevra      (HifPackage *pkg);
char        *hif_package_get_sourcerpm  (HifPackage *pkg);
char        *hif_package_get_version    (HifPackage *pkg);
char        *hif_package_get_release    (HifPackage *pkg);

const char  *hif_package_get_name       (HifPackage *pkg);
const char  *hif_package_get_arch       (HifPackage *pkg);
const unsigned char *hif_package_get_chksum(HifPackage *pkg, int *type);
const char  *hif_package_get_description(HifPackage *pkg);
const char  *hif_package_get_evr        (HifPackage *pkg);
const char  *hif_package_get_group      (HifPackage *pkg);
const char  *hif_package_get_license    (HifPackage *pkg);
const unsigned char *hif_package_get_hdr_chksum(HifPackage *pkg, int *type);
const char  *hif_package_get_packager   (HifPackage *pkg);
const char  *hif_package_get_reponame   (HifPackage *pkg);
const char  *hif_package_get_summary    (HifPackage *pkg);
const char  *hif_package_get_url        (HifPackage *pkg);
guint64      hif_package_get_downloadsize(HifPackage *pkg);
guint64      hif_package_get_epoch      (HifPackage *pkg);
guint64      hif_package_get_hdr_end    (HifPackage *pkg);
guint64      hif_package_get_installsize(HifPackage *pkg);
guint64      hif_package_get_medianr    (HifPackage *pkg);
guint64      hif_package_get_rpmdbid    (HifPackage *pkg);
guint64      hif_package_get_size       (HifPackage *pkg);
guint64      hif_package_get_buildtime  (HifPackage *pkg);
guint64      hif_package_get_installtime(HifPackage *pkg);

HifReldepList *hif_package_get_conflicts    (HifPackage *pkg);
HifReldepList *hif_package_get_enhances     (HifPackage *pkg);
HifReldepList *hif_package_get_obsoletes    (HifPackage *pkg);
HifReldepList *hif_package_get_provides     (HifPackage *pkg);
HifReldepList *hif_package_get_recommends   (HifPackage *pkg);
HifReldepList *hif_package_get_requires     (HifPackage *pkg);
HifReldepList *hif_package_get_requires_pre (HifPackage *pkg);
HifReldepList *hif_package_get_suggests     (HifPackage *pkg);
HifReldepList *hif_package_get_supplements  (HifPackage *pkg);
char       **hif_package_get_files      (HifPackage *pkg);
GPtrArray   *hif_package_get_advisories (HifPackage *pkg, int cmp_type);

HifPackageDelta *hif_package_get_delta_from_evr(HifPackage *pkg, const char *from_evr);

G_END_DECLS

#endif /* __HY_PACKAGE_H */
