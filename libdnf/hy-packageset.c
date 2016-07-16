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
 * SECTION:dnf-packageset
 * @short_description: SHORT DESCRIPTION
 * @include: libdnf.h
 * @stability: Unstable
 *
 * Long description.
 *
 * See also: #DnfContext
 */


#include <solv/bitmap.h>
#include <solv/util.h>

#include "hy-package-private.h"
#include "hy-packageset-private.h"
#include "dnf-sack-private.h"

typedef struct
{
    DnfSack     *sack;
    Map          map;
} DnfPackageSetPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(DnfPackageSet, dnf_packageset, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (dnf_packageset_get_instance_private (o))

/**
 * dnf_packageset_finalize:
 **/
static void
dnf_packageset_finalize(GObject *object)
{
    DnfPackageSet *pset = DNF_PACKAGE_SET(object);
    DnfPackageSetPrivate *priv = GET_PRIVATE(pset);

    map_free(&priv->map);

    G_OBJECT_CLASS(dnf_packageset_parent_class)->finalize(object);
}

/**
 * dnf_packageset_init:
 **/
static void
dnf_packageset_init(DnfPackageSet *pset)
{
}

/**
 * dnf_packageset_class_init:
 **/
static void
dnf_packageset_class_init(DnfPackageSetClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_packageset_finalize;
}

/**
 * dnf_packageset_new:
 * @sack: A #DnfSack
 *
 * Creates a new #DnfPackageSet.
 *
 * Returns:(transfer full): a #DnfPackageSet *
 *
 * Since: 0.7.0
 **/
DnfPackageSet *
dnf_packageset_new(DnfSack *sack)
{
    DnfPackageSet *pset;
    pset = g_object_new(DNF_TYPE_PACKAGE_SET, NULL);
    DnfPackageSetPrivate *priv = GET_PRIVATE(pset);
    priv->sack = sack;
    map_init(&priv->map, dnf_sack_get_pool(sack)->nsolvables);
    return DNF_PACKAGE_SET(pset);
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
    unsigned int enabled;
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
 * dnf_packageset_get_pkgid: (skip):
 * @pset: a #DnfPackageSet instance.
 *
 * Gets an internal ID for an id.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
Id
dnf_packageset_get_pkgid(DnfPackageSet *pset, int index, Id previous)
{
    DnfPackageSetPrivate *priv = GET_PRIVATE(pset);
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
 * dnf_packageset_from_bitmap: (skip):
 * @pset: a #DnfPackageSet instance.
 *
 * Creates a set from a bitmap.
 *
 * Returns: a new #DnfPackageSet
 *
 * Since: 0.7.0
 */
DnfPackageSet *
dnf_packageset_from_bitmap(DnfSack *sack, Map *m)
{
    DnfPackageSet *pset = dnf_packageset_new(sack);
    DnfPackageSetPrivate *priv = GET_PRIVATE(pset);
    map_free(&priv->map);
    map_init_clone(&priv->map, m);
    return pset;
}

/**
 * dnf_packageset_get_map: (skip):
 * @pset: a #DnfPackageSet instance.
 *
 * Gets the map.
 *
 * Returns: A #Map, or %NULL
 *
 * Since: 0.7.0
 */
Map *
dnf_packageset_get_map(DnfPackageSet *pset)
{
    DnfPackageSetPrivate *priv = GET_PRIVATE(pset);
    return &priv->map;
}

/**
 * dnf_packageset_clone:
 * @pset: a #DnfPackageSet instance.
 *
 * Clones the packageset.
 *
 * Returns: a new #DnfPackageSet
 *
 * Since: 0.7.0
 */
DnfPackageSet *
dnf_packageset_clone(DnfPackageSet *pset)
{
    DnfPackageSetPrivate *priv = GET_PRIVATE(pset);
    DnfPackageSet *new = dnf_packageset_new(priv->sack);
    DnfPackageSetPrivate *priv_new = GET_PRIVATE(new);
    map_free(&priv_new->map);
    map_init_clone(&priv_new->map, &priv->map);
    return new;
}

/**
 * dnf_packageset_add:
 * @pset: a #DnfPackageSet instance.
 * @pkg: a #DnfPackage
 *
 * Adds a new package to the set.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
void
dnf_packageset_add(DnfPackageSet *pset, DnfPackage *pkg)
{
    DnfPackageSetPrivate *priv = GET_PRIVATE(pset);
    MAPSET(&priv->map, dnf_package_get_id(pkg));
}

/**
 * dnf_packageset_count:
 * @pset: a #DnfPackageSet instance.
 *
 * Returns the size of the set.
 *
 * Returns: integer, or 0 for empty
 *
 * Since: 0.7.0
 */
unsigned
dnf_packageset_count(DnfPackageSet *pset)
{
    DnfPackageSetPrivate *priv = GET_PRIVATE(pset);
    return map_count(&priv->map);
}

/**
 * dnf_packageset_get_clone:
 * @pset: a #DnfPackageSet instance.
 *
 * Gets the package in the set.
 *
 * Returns: (transfer full): a #DnfPackage, or %NULL for invalid
 *
 * Since: 0.7.0
 */
DnfPackage *
dnf_packageset_get_clone(DnfPackageSet *pset, int index)
{
    DnfPackageSetPrivate *priv = GET_PRIVATE(pset);
    Id id = map_index2id(&priv->map, index, -1);
    if (id < 0)
        return NULL;
    return dnf_package_new(priv->sack, id);
}

/**
 * dnf_packageset_has:
 * @pset: a #DnfPackageSet instance.
 * @pkg: a #DnfPackage.
 *
 * Finds out if a package is in the set.
 *
 * Returns: %TRUE if it exists
 *
 * Since: 0.7.0
 */
int
dnf_packageset_has(DnfPackageSet *pset, DnfPackage *pkg)
{
    DnfPackageSetPrivate *priv = GET_PRIVATE(pset);
    return MAPTST(&priv->map, dnf_package_get_id(pkg));
}
