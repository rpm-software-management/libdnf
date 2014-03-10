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

//libsolv
#include <solv/repo.h>
#include <solv/util.h>

// hawkey
#include "advisoryref_internal.h"

struct _HyAdvisoryRef {
    Pool *pool;
    Id a_id;
    int index;
};

/* internal */
HyAdvisoryRef
advisoryref_create(Pool *pool, Id a_id, int index)
{
    HyAdvisoryRef ref = solv_calloc(1, sizeof(*ref));
    ref->pool = pool;
    ref->a_id = a_id;
    ref->index = index;
    return ref;
}

/* public */
void
hy_advisoryref_free(HyAdvisoryRef advisoryref)
{
    solv_free(advisoryref);
}

static const char *
advisoryref_get_str(HyAdvisoryRef advisoryref, Id keyname)
{
    Dataiterator di;
    const char *str = NULL;
    int count = 0;

    dataiterator_init(&di, advisoryref->pool, 0, advisoryref->a_id, UPDATE_REFERENCE, 0, 0);
    while (dataiterator_step(&di)) {
	dataiterator_setpos(&di);
	if (count++ == advisoryref->index) {
	    str = pool_lookup_str(advisoryref->pool, SOLVID_POS, keyname);
	    break;
	}
    }
    dataiterator_free(&di);

    return str;
}

HyAdvisoryRefType
hy_advisoryref_get_type(HyAdvisoryRef advisoryref)
{
    const char *type;
    type = advisoryref_get_str(advisoryref, UPDATE_REFERENCE_TYPE);

    if (type == NULL)
	return HY_REFERENCE_UNKNOWN;
    if (!strcmp (type, "bugzilla"))
	return HY_REFERENCE_BUGZILLA;
    if (!strcmp (type, "cve"))
	return HY_REFERENCE_CVE;
    if (!strcmp (type, "vendor"))
	return HY_REFERENCE_VENDOR;
    return HY_REFERENCE_UNKNOWN;
}

const char *
hy_advisoryref_get_id(HyAdvisoryRef advisoryref)
{
    return advisoryref_get_str(advisoryref, UPDATE_REFERENCE_ID);
}

const char *
hy_advisoryref_get_title(HyAdvisoryRef advisoryref)
{
    return advisoryref_get_str(advisoryref, UPDATE_REFERENCE_TITLE);
}

const char *
hy_advisoryref_get_url(HyAdvisoryRef advisoryref)
{
    return advisoryref_get_str(advisoryref, UPDATE_REFERENCE_HREF);
}
