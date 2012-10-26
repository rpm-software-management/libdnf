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
map_index2id(Map *map, unsigned index)
{
    unsigned char *ti = map->map;
    unsigned char *end = ti + map->size;
    int enabled;

    while (ti < end) {
	enabled = _BitCountLookup[*ti];

	if (index >= enabled) {
	    index -= enabled;
	    ti++;
	    continue;
	}
	Id total = (ti - map->map) << 3;
	for (unsigned char byte = *ti ; !(byte & 0x01); byte >>= 1)
	    ++total;
	return total;
    }
    return -1;
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
    map_init(&pset->map, hy_sack_count(sack));
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
    unsigned char *ti = pset->map.map;
    unsigned char *end = ti + pset->map.size;
    unsigned c = 0;

    while (ti < end)
	c += _BitCountLookup[*ti++];

    return c;
}

HyPackage
hy_packageset_get_clone(HyPackageSet pset, int index)
{
    Id id = map_index2id(&pset->map, index);
    if (id < 0)
	return NULL;
    return package_create(sack_pool(pset->sack), id);
}

int
hy_packageset_has(HyPackageSet pset, HyPackage pkg)
{
    return MAPTST(&pset->map, package_id(pkg));
}
