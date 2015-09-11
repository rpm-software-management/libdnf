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
#include <solv/pool.h>
#include <solv/util.h>

// hawkey
#include "iutil.h"
#include "reldep_internal.h"
#include "sack_internal.h"

struct _HyReldep {
    Pool *pool;
    Id r_id;
};

struct _HyReldepList {
    Pool *pool;
    Queue queue;
};

HyReldep
reldep_create(Pool *pool, Id r_id)
{
    HyReldep reldep = solv_calloc(1, sizeof(*reldep));
    reldep->pool = pool;
    reldep->r_id = r_id;
    return reldep;
}

Id
reldep_id(HyReldep reldep)
{
    return reldep->r_id;
}

HyReldepList
reldeplist_from_queue(Pool *pool, Queue h)
{
    HyReldepList reldeplist = solv_calloc(1, sizeof(*reldeplist));
    reldeplist->pool = pool;
    queue_init_clone(&reldeplist->queue, &h);
    return reldeplist;
}

void
merge_reldeplists(HyReldepList rl1, HyReldepList rl2)
{
    queue_insertn(&rl1->queue, 0, rl2->queue.count, rl2->queue.elements);
}

HyReldep
hy_reldep_create(HySack sack, const char *name, int cmp_type, const char *evr)
{
    Pool *pool = sack_pool(sack);
    Id id = pool_str2id(pool, name, 0);

    if (id == STRID_NULL || id == STRID_EMPTY)
	// stop right there, this will never match anything.
	return NULL;

    if (evr) {
	assert(cmp_type);
        Id ievr = pool_str2id(pool, evr, 1);
        int flags = cmptype2relflags(cmp_type);
        id = pool_rel2id(pool, id, ievr, flags, 1);
    }
    return reldep_create(pool, id);
}

void
hy_reldep_free(HyReldep reldep)
{
    solv_free(reldep);
}

HyReldep
hy_reldep_clone(HyReldep reldep)
{
    return reldep_create(reldep->pool, reldep->r_id);
}

char
*hy_reldep_str(HyReldep reldep)
{
    const char *str = pool_dep2str(reldep->pool, reldep->r_id);
    return solv_strdup(str);
}

HyReldepList
hy_reldeplist_create(HySack sack)
{
    HyReldepList reldeplist = solv_calloc(1, sizeof(*reldeplist));
    reldeplist->pool = sack_pool(sack);
    queue_init(&reldeplist->queue);
    return reldeplist;
}

void
hy_reldeplist_free(HyReldepList reldeplist)
{
    queue_free(&reldeplist->queue);
    solv_free(reldeplist);
}

void
hy_reldeplist_add(HyReldepList reldeplist, HyReldep reldep)
{
    queue_push(&reldeplist->queue, reldep->r_id);
}

int
hy_reldeplist_count(HyReldepList reldeplist)
{
    return reldeplist->queue.count;
}

HyReldep
hy_reldeplist_get_clone(HyReldepList reldeplist, int index)
{
    Id r_id = reldeplist->queue.elements[index];
    return reldep_create(reldeplist->pool, r_id);
}
