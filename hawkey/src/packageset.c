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
#include "package_internal.h"
#include "packageset_internal.h"
#include "sack_internal.h"

struct _HyPackageSet {
    HySack sack;
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
packageset_get_pkgid(HyPackageSet pset, int index, Id previous)
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

HyPackageSet
packageset_from_bitmap(HySack sack, Map *m)
{
    HyPackageSet pset = solv_calloc(1, sizeof(*pset));
    pset->sack = sack;
    map_init_clone(&pset->map, m);
    return pset;
}

Map *
packageset_get_map(HyPackageSet pset)
{
    return &pset->map;
}

HyPackageSet
hy_packageset_create(HySack sack)
{
    HyPackageSet pset = solv_calloc(1, sizeof(*pset));
    pset->sack = sack;
    map_init(&pset->map, sack_pool(sack)->nsolvables);
    return pset;
}

HyPackageSet
hy_packageset_clone(HyPackageSet pset)
{
    HyPackageSet new = solv_malloc(sizeof(*new));
    memcpy(new, pset, sizeof(*pset));
    map_init_clone(&new->map, &pset->map);
    return new;
}

void
hy_packageset_free(HyPackageSet pset)
{
    map_free(&pset->map);
    solv_free(pset);
}

void
hy_packageset_add(HyPackageSet pset, HyPackage pkg)
{
    MAPSET(&pset->map, package_id(pkg));
    hy_package_free(pkg);
}

unsigned
hy_packageset_count(HyPackageSet pset)
{
    return map_count(&pset->map);
}

HyPackage
hy_packageset_get_clone(HyPackageSet pset, int index)
{
    Id id = map_index2id(&pset->map, index, -1);
    if (id < 0)
	return NULL;
    return package_create(pset->sack, id);
}

int
hy_packageset_has(HyPackageSet pset, HyPackage pkg)
{
    return MAPTST(&pset->map, package_id(pkg));
}
