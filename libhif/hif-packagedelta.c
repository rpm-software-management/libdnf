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
 * SECTION:hif-packagedelta
 * @short_description: Package delta
 * @include: libhif.h
 * @stability: Unstable
 *
 * An object representing a package delta.
 *
 * See also: #HifContext
 */

#include "config.h"

#include <solv/repodata.h>
#include <solv/util.h>

#include "hif-packagedelta-private.h"
#include "hy-iutil.h"

typedef struct
{
    char            *location;
    char            *baseurl;
    guint64          downloadsize;
    int              checksum_type;
    unsigned char   *checksum;
} HifPackageDeltaPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(HifPackageDelta, hif_packagedelta, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (hif_packagedelta_get_instance_private (o))

/**
 * hif_packagedelta_finalize:
 **/
static void
hif_packagedelta_finalize(GObject *object)
{
    HifPackageDelta *packagedelta = HIF_PACKAGEDELTA(object);
    HifPackageDeltaPrivate *priv = GET_PRIVATE(packagedelta);

    g_free(priv->location);
    g_free(priv->baseurl);
    g_free(priv->checksum);

    G_OBJECT_CLASS(hif_packagedelta_parent_class)->finalize(object);
}

/**
 * hif_packagedelta_init:
 **/
static void
hif_packagedelta_init(HifPackageDelta *packagedelta)
{
}

/**
 * hif_packagedelta_class_init:
 **/
static void
hif_packagedelta_class_init(HifPackageDeltaClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = hif_packagedelta_finalize;
}

/**
 * hif_packagedelta_new:
 *
 * Creates a new #HifPackageDelta.
 *
 * Returns:(transfer full): a #HifPackageDelta
 *
 * Since: 0.7.0
 **/
HifPackageDelta *
hif_packagedelta_new(Pool *pool)
{
    HifPackageDeltaPrivate *priv;
    HifPackageDelta *delta;
    Id checksum_type;
    const unsigned char *checksum;

    delta = g_object_new(HIF_TYPE_PACKAGEDELTA, NULL);
    priv = GET_PRIVATE(delta);

    /* obtain info */
    priv->location = g_strdup(pool_lookup_deltalocation(pool, SOLVID_POS, 0));
    priv->baseurl = g_strdup(pool_lookup_str(pool, SOLVID_POS, DELTA_LOCATION_BASE));
    priv->downloadsize = pool_lookup_num(pool, SOLVID_POS, DELTA_DOWNLOADSIZE, 0);
    checksum = pool_lookup_bin_checksum(pool, SOLVID_POS, DELTA_CHECKSUM, &checksum_type);
    if (checksum) {
        priv->checksum_type = checksumt_l2h(checksum_type);
        priv->checksum = solv_memdup((void*)checksum, checksum_type2length(priv->checksum_type));
    }

    return HIF_PACKAGEDELTA(delta);
}

/**
 * hif_packagedelta_get_location:
 * @delta: a #HifPackageDelta instance.
 *
 * Gets the delta location.
 *
 * Returns: location URL
 *
 * Since: 0.7.0
 */
const char *
hif_packagedelta_get_location(HifPackageDelta *delta)
{
    HifPackageDeltaPrivate *priv = GET_PRIVATE(delta);
    return priv->location;
}

/**
 * hif_packagedelta_get_baseurl:
 * @delta: a #HifPackageDelta instance.
 *
 * Gets the delta baseurl.
 *
 * Returns: string URL
 *
 * Since: 0.7.0
 */
const char *
hif_packagedelta_get_baseurl(HifPackageDelta *delta)
{
    HifPackageDeltaPrivate *priv = GET_PRIVATE(delta);
    return priv->baseurl;
}

/**
 * hif_packagedelta_get_downloadsize:
 * @delta: a #HifPackageDelta instance.
 *
 * Gets the delta download size.
 *
 * Returns: size in bytes
 *
 * Since: 0.7.0
 */
guint64
hif_packagedelta_get_downloadsize(HifPackageDelta *delta)
{
    HifPackageDeltaPrivate *priv = GET_PRIVATE(delta);
    return priv->downloadsize;
}

/**
 * hif_packagedelta_get_chksum:
 * @delta: a #HifPackageDelta instance.
 * @type: the checksum type
 *
 * Returns the checksum of the delta.
 *
 * Returns: checksum data
 *
 * Since: 0.7.0
 */
const unsigned char *
hif_packagedelta_get_chksum(HifPackageDelta *delta, int *type)
{
    HifPackageDeltaPrivate *priv = GET_PRIVATE(delta);
    if (type)
        *type = priv->checksum_type;
    return priv->checksum;
}
