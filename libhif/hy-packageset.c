/*
 * Copyright (C) 2012-2013 Red Hat, Inc.
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

#include <assert.h>

// libsolv
#include <solv/bitmap.h>
#include <solv/util.h>

// hawkey
#include "hy-package-private.h"
#include "hy-packageset-private.h"
#include "hif-sack-private.h"

struct _HifPackageSet {
    HifSack *sack;
    Map map;
};

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

Id
packageset_get_pkgid(HifPackageSet pset, int index, Id previous)
{
    Id id = map_index2id(&pset->map, index, previous);
    assert(id >= 0);
    return id;
}

unsigned
map_count(Map *m)
{
    unsigned char *ti = m->map;
    unsigned char *end = ti + m->size;
    unsigned c = 0;

    while (ti < end)
        c += _BitCountLookup[*ti++];

    return c;
}

HifPackageSet
packageset_from_bitmap(HifSack *sack, Map *m)
{
    HifPackageSet pset = g_malloc0(sizeof(*pset));
    pset->sack = sack;
    map_init_clone(&pset->map, m);
    return pset;
}

Map *
packageset_get_map(HifPackageSet pset)
{
    return &pset->map;
}

HifPackageSet
hy_packageset_create(HifSack *sack)
{
    HifPackageSet pset = g_malloc0(sizeof(*pset));
    pset->sack = sack;
    map_init(&pset->map, hif_sack_get_pool(sack)->nsolvables);
    return pset;
}

HifPackageSet
hy_packageset_clone(HifPackageSet pset)
{
    HifPackageSet new = g_malloc(sizeof(*new));
    memcpy(new, pset, sizeof(*pset));
    map_init_clone(&new->map, &pset->map);
    return new;
}

void
hy_packageset_free(HifPackageSet pset)
{
    map_free(&pset->map);
    g_free(pset);
}

void
hy_packageset_add(HifPackageSet pset, HifPackage *pkg)
{
    MAPSET(&pset->map, hif_package_get_id(pkg));
    g_object_unref(pkg);
}

unsigned
hy_packageset_count(HifPackageSet pset)
{
    return map_count(&pset->map);
}

HifPackage *
hy_packageset_get_clone(HifPackageSet pset, int index)
{
    Id id = map_index2id(&pset->map, index, -1);
    if (id < 0)
        return NULL;
    return hif_package_new(pset->sack, id);
}

int
hy_packageset_has(HifPackageSet pset, HifPackage *pkg)
{
    return MAPTST(&pset->map, hif_package_get_id(pkg));
}
