/*
 * Copyright (C) 2014 Red Hat, Inc.
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

//libsolv
#include <solv/repo.h>
#include <solv/util.h>

// hawkey
#include "advisory_internal.h"
#include "advisorypkg_internal.h"
#include "advisoryref_internal.h"
#include "iutil.h"

#define FILENAME_BLOCK 15

struct _HyAdvisory {
    Pool *pool;
    Id a_id;
};

struct _HyAdvisoryList {
    Pool *pool;
    Queue queue;
};

/* internal */
Id
advisory_id(HyAdvisory advisory)
{
    return advisory->a_id;
}

HyAdvisory
advisory_create(Pool *pool, Id a_id)
{
    HyAdvisory advisory = solv_calloc(1, sizeof(*advisory));
    advisory->pool = pool;
    advisory->a_id = a_id;
    return advisory;
}

int
advisory_identical(HyAdvisory left, HyAdvisory right)
{
    return left->a_id == right->a_id;
}

HyAdvisoryList
advisorylist_create(Pool *pool)
{
    HyAdvisoryList advisorylist = solv_calloc(1, sizeof(*advisorylist));
    advisorylist->pool = pool;
    queue_init(&advisorylist->queue);
    return advisorylist;
}

void
advisorylist_add(HyAdvisoryList advisorylist, HyAdvisory advisory)
{
    assert(advisory->pool == advisorylist->pool);
    queue_push(&advisorylist->queue, advisory->a_id);
}

/* public */
void
hy_advisory_free(HyAdvisory advisory)
{
    solv_free(advisory);
}

const char *
hy_advisory_get_title(HyAdvisory advisory)
{
    return pool_lookup_str(advisory->pool, advisory->a_id, SOLVABLE_SUMMARY);
}

const char *
hy_advisory_get_id(HyAdvisory advisory)
{
    const char *id;

    id = pool_lookup_str(advisory->pool, advisory->a_id, SOLVABLE_NAME);
    assert(str_startswith(id, SOLVABLE_NAME_ADVISORY_PREFIX));
    //remove the prefix
    id += strlen(SOLVABLE_NAME_ADVISORY_PREFIX);

    return id;
}

HyAdvisoryType
hy_advisory_get_type(HyAdvisory advisory)
{
    const char *type;
    type = pool_lookup_str(advisory->pool, advisory->a_id, SOLVABLE_PATCHCATEGORY);

    if (type == NULL)
	return HY_ADVISORY_UNKNOWN;
    if (!strcmp (type, "bugfix"))
	return HY_ADVISORY_BUGFIX;
    if (!strcmp (type, "enhancement"))
	return HY_ADVISORY_ENHANCEMENT;
    if (!strcmp (type, "security"))
	return HY_ADVISORY_SECURITY;
    return HY_ADVISORY_UNKNOWN;
}

const char *
hy_advisory_get_description(HyAdvisory advisory)
{
    return pool_lookup_str(advisory->pool, advisory->a_id, SOLVABLE_DESCRIPTION);
}

const char *
hy_advisory_get_rights(HyAdvisory advisory)
{
    return pool_lookup_str(advisory->pool, advisory->a_id, UPDATE_RIGHTS);
}

unsigned long long
hy_advisory_get_updated(HyAdvisory advisory)
{
    return pool_lookup_num(advisory->pool, advisory->a_id, SOLVABLE_BUILDTIME, 0);
}

HyAdvisoryPkgList
hy_advisory_get_packages(HyAdvisory advisory)
{
    Dataiterator di;
    HyAdvisoryPkg pkg;
    Pool *pool = advisory->pool;
    Id a_id = advisory->a_id;
    HyAdvisoryPkgList pkglist = advisorypkglist_create();

    dataiterator_init(&di, pool, 0, a_id, UPDATE_COLLECTION, 0, 0);
    while (dataiterator_step(&di)) {
	dataiterator_setpos(&di);
	pkg = advisorypkg_create();
	advisorypkg_set_string(pkg, HY_ADVISORYPKG_NAME,
		pool_lookup_str(pool, SOLVID_POS, UPDATE_COLLECTION_NAME));
	advisorypkg_set_string(pkg, HY_ADVISORYPKG_EVR,
		pool_lookup_str(pool, SOLVID_POS, UPDATE_COLLECTION_EVR));
	advisorypkg_set_string(pkg, HY_ADVISORYPKG_ARCH,
		pool_lookup_str(pool, SOLVID_POS, UPDATE_COLLECTION_ARCH));
	advisorypkg_set_string(pkg, HY_ADVISORYPKG_FILENAME,
		pool_lookup_str(pool, SOLVID_POS, UPDATE_COLLECTION_FILENAME));
	advisorypkglist_add(pkglist, pkg);
	hy_advisorypkg_free(pkg);
    }
    dataiterator_free(&di);

    return pkglist;
}

HyAdvisoryRefList
hy_advisory_get_references(HyAdvisory advisory)
{
    Dataiterator di;
    HyAdvisoryRef ref;
    Pool *pool = advisory->pool;
    Id a_id = advisory->a_id;
    HyAdvisoryRefList reflist = advisoryreflist_create();

    dataiterator_init(&di, pool, 0, a_id, UPDATE_REFERENCE, 0, 0);
    for (int index = 0; dataiterator_step(&di); index++) {
	ref = advisoryref_create(pool, a_id, index);
	advisoryreflist_add(reflist, ref);
	hy_advisoryref_free(ref);
    }
    dataiterator_free(&di);

    return reflist;
}

void
hy_advisorylist_free(HyAdvisoryList advisorylist)
{
    queue_free(&advisorylist->queue);
    solv_free(advisorylist);
}

int
hy_advisorylist_count(HyAdvisoryList advisorylist)
{
    return advisorylist->queue.count;
}

HyAdvisory
hy_advisorylist_get_clone(HyAdvisoryList advisorylist, int index)
{
    Id a_id = advisorylist->queue.elements[index];
    return advisory_create(advisorylist->pool, a_id);
}
