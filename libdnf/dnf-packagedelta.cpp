/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2012-2014 Red Hat, Inc.
 * Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or(at your option) any later version.
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

/**
 * SECTION:dnf-packagedelta
 * @short_description: Package delta
 * @include: libdnf.h
 * @stability: Unstable
 *
 * An object representing a package delta.
 *
 * See also: #DnfContext
 */


#include <solv/repodata.h>
#include <solv/util.h>

#include "dnf-packagedelta-private.hpp"
#include "hy-iutil-private.hpp"

typedef struct
{
    char            *location;
    char            *baseurl;
    guint64          downloadsize;
    int              checksum_type;
    unsigned char   *checksum;
} DnfPackageDeltaPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(DnfPackageDelta, dnf_packagedelta, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (static_cast<DnfPackageDeltaPrivate *>(dnf_packagedelta_get_instance_private (o)))

/**
 * dnf_packagedelta_finalize:
 **/
static void
dnf_packagedelta_finalize(GObject *object)
{
    DnfPackageDelta *packagedelta = DNF_PACKAGEDELTA(object);
    DnfPackageDeltaPrivate *priv = GET_PRIVATE(packagedelta);

    g_free(priv->location);
    g_free(priv->baseurl);
    g_free(priv->checksum);

    G_OBJECT_CLASS(dnf_packagedelta_parent_class)->finalize(object);
}

/**
 * dnf_packagedelta_init:
 **/
static void
dnf_packagedelta_init(DnfPackageDelta *packagedelta)
{
}

/**
 * dnf_packagedelta_class_init:
 **/
static void
dnf_packagedelta_class_init(DnfPackageDeltaClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_packagedelta_finalize;
}

/**
 * dnf_packagedelta_new:
 *
 * Creates a new #DnfPackageDelta.
 *
 * Returns:(transfer full): a #DnfPackageDelta
 *
 * Since: 0.7.0
 **/
DnfPackageDelta *
dnf_packagedelta_new(Pool *pool)
{
    Id checksum_type;

    auto delta = DNF_PACKAGEDELTA(g_object_new(DNF_TYPE_PACKAGEDELTA, NULL));
    auto priv = GET_PRIVATE(delta);

    /* obtain info */
    priv->location = g_strdup(pool_lookup_deltalocation(pool, SOLVID_POS, 0));
    priv->baseurl = g_strdup(pool_lookup_str(pool, SOLVID_POS, DELTA_LOCATION_BASE));
    priv->downloadsize = pool_lookup_num(pool, SOLVID_POS, DELTA_DOWNLOADSIZE, 0);
    auto checksum = pool_lookup_bin_checksum(pool, SOLVID_POS, DELTA_CHECKSUM, &checksum_type);
    if (checksum) {
        priv->checksum_type = checksumt_l2h(checksum_type);
        priv->checksum = static_cast<unsigned char *>(
            solv_memdup((void*)checksum, checksum_type2length(priv->checksum_type)));
    }

    return delta;
}

/**
 * dnf_packagedelta_get_location:
 * @delta: a #DnfPackageDelta instance.
 *
 * Gets the delta location.
 *
 * Returns: location URL
 *
 * Since: 0.7.0
 */
const char *
dnf_packagedelta_get_location(DnfPackageDelta *delta)
{
    DnfPackageDeltaPrivate *priv = GET_PRIVATE(delta);
    return priv->location;
}

/**
 * dnf_packagedelta_get_baseurl:
 * @delta: a #DnfPackageDelta instance.
 *
 * Gets the delta baseurl.
 *
 * Returns: string URL
 *
 * Since: 0.7.0
 */
const char *
dnf_packagedelta_get_baseurl(DnfPackageDelta *delta)
{
    DnfPackageDeltaPrivate *priv = GET_PRIVATE(delta);
    return priv->baseurl;
}

/**
 * dnf_packagedelta_get_downloadsize:
 * @delta: a #DnfPackageDelta instance.
 *
 * Gets the delta download size.
 *
 * Returns: size in bytes
 *
 * Since: 0.7.0
 */
guint64
dnf_packagedelta_get_downloadsize(DnfPackageDelta *delta)
{
    DnfPackageDeltaPrivate *priv = GET_PRIVATE(delta);
    return priv->downloadsize;
}

/**
 * dnf_packagedelta_get_chksum:
 * @delta: a #DnfPackageDelta instance.
 * @type: the checksum type
 *
 * Returns the checksum of the delta.
 *
 * Returns: checksum data
 *
 * Since: 0.7.0
 */
const unsigned char *
dnf_packagedelta_get_chksum(DnfPackageDelta *delta, int *type)
{
    DnfPackageDeltaPrivate *priv = GET_PRIVATE(delta);
    if (type)
        *type = priv->checksum_type;
    return priv->checksum;
}
