/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2012-2013 Red Hat, Inc.
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
 * SECTION:hif-packageset
 * @short_description: SHORT DESCRIPTION
 * @include: libhif.h
 * @stability: Unstable
 *
 * Long description.
 *
 * See also: #HifContext
 */


#include <solv/bitmap.h>
#include <solv/util.h>

#include "hy-package-private.h"
#include "hy-packageset-private.h"
#include "hif-sack-private.h"

typedef struct
{
    HifSack     *sack;
    Map          map;
} HifPackageSetPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(HifPackageSet, hif_packageset, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (hif_packageset_get_instance_private (o))

/**
 * hif_packageset_finalize:
 **/
static void
hif_packageset_finalize(GObject *object)
{
    HifPackageSet *pset = HIF_PACKAGE_SET(object);
    HifPackageSetPrivate *priv = GET_PRIVATE(pset);

    map_free(&priv->map);

    G_OBJECT_CLASS(hif_packageset_parent_class)->finalize(object);
}

/**
 * hif_packageset_init:
 **/
static void
hif_packageset_init(HifPackageSet *pset)
{
}

/**
 * hif_packageset_class_init:
 **/
static void
hif_packageset_class_init(HifPackageSetClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = hif_packageset_finalize;
}

/**
 * hif_packageset_new:
 * @sack: A #HifSack
 *
 * Creates a new #HifPackageSet.
 *
 * Returns:(transfer full): a #HifPackageSet *
 *
 * Since: 0.7.0
 **/
HifPackageSet *
hif_packageset_new(HifSack *sack)
{
    HifPackageSet *pset;
    pset = g_object_new(HIF_TYPE_PACKAGE_SET, NULL);
    HifPackageSetPrivate *priv = GET_PRIVATE(pset);
    priv->sack = sack;
    map_init(&priv->map, hif_sack_get_pool(sack)->nsolvables);
    return HIF_PACKAGE_SET(pset);
}

// see http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetTable
static const unsigned char _BitCountLookup[256] =
{
#   define B2(n) n,     n+1,     n+1,     n+2
#   define B4(n) B2(n), B2(n+1), B2(n+1), B2(n+2)
#   define B6(n) B4(n), B4(n+1), B4(n+1), B4(n+2)
    B6(0), B6(1), B6(1), B6(2)
};

static Id
map_index2id(Map *map, unsigned index, Id previous)
{
    unsigned char *ti = map->map;
    unsigned char *end = ti + map->size;
    int enabled;
    Id id;

    if (previous >= 0) {
        ti += previous >> 3;
        unsigned char byte = *ti; // byte with the previous match
        byte >>= (previous & 7) + 1; // shift away all previous 1 bits
        enabled = _BitCountLookup[byte]; // are there any 1 bits left?

        for (id = previous + 1; enabled; byte >>= 1, id++)
            if (byte & 0x01)
                return id;
        index = 0; // we are looking for the immediately following index
        ti++;
    }

    while (ti < end) {
        enabled = _BitCountLookup[*ti];

        if (index >= enabled ){
            index -= enabled;
            ti++;
            continue;
        }
        id = (ti - map->map) << 3;

        index++;
        for (unsigned char byte = *ti; index; byte >>= 1) {
            if ((byte & 0x01))
                index--;
            if (index)
                id++;
        }
        return id;
    }
    return -1;
}

/**
 * hif_packageset_get_pkgid: (skip):
 * @pset: a #HifPackageSet instance.
 *
 * Gets an internal ID for an id.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
Id
hif_packageset_get_pkgid(HifPackageSet *pset, int index, Id previous)
{
    HifPackageSetPrivate *priv = GET_PRIVATE(pset);
    Id id = map_index2id(&priv->map, index, previous);
    g_assert(id >= 0);
    return id;
}

static unsigned
map_count(Map *m)
{
    unsigned char *ti = m->map;
    unsigned char *end = ti + m->size;
    unsigned c = 0;

    while (ti < end)
        c += _BitCountLookup[*ti++];

    return c;
}

/**
 * hif_packageset_from_bitmap: (skip):
 * @pset: a #HifPackageSet instance.
 *
 * Creates a set from a bitmap.
 *
 * Returns: a new #HifPackageSet
 *
 * Since: 0.7.0
 */
HifPackageSet *
hif_packageset_from_bitmap(HifSack *sack, Map *m)
{
    HifPackageSet *pset = hif_packageset_new(sack);
    HifPackageSetPrivate *priv = GET_PRIVATE(pset);
    map_free(&priv->map);
    map_init_clone(&priv->map, m);
    return pset;
}

/**
 * hif_packageset_get_map: (skip):
 * @pset: a #HifPackageSet instance.
 *
 * Gets the map.
 *
 * Returns: A #Map, or %NULL
 *
 * Since: 0.7.0
 */
Map *
hif_packageset_get_map(HifPackageSet *pset)
{
    HifPackageSetPrivate *priv = GET_PRIVATE(pset);
    return &priv->map;
}

/**
 * hif_packageset_clone:
 * @pset: a #HifPackageSet instance.
 *
 * Clones the packageset.
 *
 * Returns: a new #HifPackageSet
 *
 * Since: 0.7.0
 */
HifPackageSet *
hif_packageset_clone(HifPackageSet *pset)
{
    HifPackageSetPrivate *priv = GET_PRIVATE(pset);
    HifPackageSet *new = hif_packageset_new(priv->sack);
    HifPackageSetPrivate *priv_new = GET_PRIVATE(new);
    map_free(&priv_new->map);
    map_init_clone(&priv_new->map, &priv->map);
    return new;
}

/**
 * hif_packageset_add:
 * @pset: a #HifPackageSet instance.
 * @pkg: a #HifPackage
 *
 * Adds a new package to the set.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
void
hif_packageset_add(HifPackageSet *pset, HifPackage *pkg)
{
    HifPackageSetPrivate *priv = GET_PRIVATE(pset);
    MAPSET(&priv->map, hif_package_get_id(pkg));
}

/**
 * hif_packageset_count:
 * @pset: a #HifPackageSet instance.
 *
 * Returns the size of the set.
 *
 * Returns: integer, or 0 for empty
 *
 * Since: 0.7.0
 */
unsigned
hif_packageset_count(HifPackageSet *pset)
{
    HifPackageSetPrivate *priv = GET_PRIVATE(pset);
    return map_count(&priv->map);
}

/**
 * hif_packageset_get_clone:
 * @pset: a #HifPackageSet instance.
 *
 * Gets the package in the set.
 *
 * Returns: (transfer full): a #HifPackage, or %NULL for invalid
 *
 * Since: 0.7.0
 */
HifPackage *
hif_packageset_get_clone(HifPackageSet *pset, int index)
{
    HifPackageSetPrivate *priv = GET_PRIVATE(pset);
    Id id = map_index2id(&priv->map, index, -1);
    if (id < 0)
        return NULL;
    return hif_package_new(priv->sack, id);
}

/**
 * hif_packageset_has:
 * @pset: a #HifPackageSet instance.
 * @pkg: a #HifPackage.
 *
 * Finds out if a package is in the set.
 *
 * Returns: %TRUE if it exists
 *
 * Since: 0.7.0
 */
int
hif_packageset_has(HifPackageSet *pset, HifPackage *pkg)
{
    HifPackageSetPrivate *priv = GET_PRIVATE(pset);
    return MAPTST(&priv->map, hif_package_get_id(pkg));
}
