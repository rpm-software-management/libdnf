/*
 * Copyright (C) 2012-2014 Red Hat, Inc.
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

// libsolv
#include <solv/selection.h>
#include <solv/solver.h>
#include <solv/util.h>

// hawkey
#include "errno.h"
#include "goal_internal.h"
#include "iutil.h"
#include "package_internal.h"
#include "packagelist.h"
#include "query_internal.h"
#include "sack_internal.h"
#include "selector_internal.h"

static int
replace_filter(HySack sack, struct _Filter **fp, int keyname, int cmp_type,
	       const char *match)
{
    if (*fp == NULL)
	*fp = filter_create(1);
    else
	filter_reinit(*fp, 1);

    struct _Filter *f = *fp;

    f->keyname = keyname;
    f->cmp_type = cmp_type;
    if (keyname == HY_PKG_PROVIDES && cmp_type != HY_GLOB) {
	f->match_type = _HY_RELDEP;
	HyReldep reldep = reldep_from_str(sack, match);
	if (reldep == NULL) {
	    filter_free(*fp);
	    *fp = NULL;
	    return HY_E_SELECTOR;
	}
	f->matches[0].reldep = reldep;
	return 0;
    }

    f->match_type = _HY_STR;
    f->matches[0].str = solv_strdup(match);
    return 0;
}

static int
valid_setting(int keyname, int cmp_type)
{
    switch (keyname) {
    case HY_PKG_ARCH:
    case HY_PKG_EVR:
    case HY_PKG_REPONAME:
    case HY_PKG_VERSION:
	return cmp_type == HY_EQ;
    case HY_PKG_PROVIDES:
    case HY_PKG_NAME:
	return (cmp_type == HY_EQ || cmp_type == HY_GLOB);
    case HY_PKG_FILE:
	return 1;
    default:
	return 0;
    }
}

HySelector
hy_selector_create(HySack sack)
{
    HySelector sltr = solv_calloc(1, sizeof(*sltr));
    sltr->sack = sack;
    return sltr;
}

void
hy_selector_free(HySelector sltr)
{
    filter_free(sltr->f_arch);
    filter_free(sltr->f_evr);
    filter_free(sltr->f_file);
    filter_free(sltr->f_name);
    filter_free(sltr->f_provides);
    filter_free(sltr->f_reponame);
    solv_free(sltr);
}

int
hy_selector_set(HySelector sltr, int keyname, int cmp_type, const char *match)
{
    if (!valid_setting(keyname, cmp_type))
	return HY_E_SELECTOR;
    HySack sack = selector_sack(sltr);

    switch (keyname) {
    case HY_PKG_ARCH:
	return replace_filter(sack, &sltr->f_arch, keyname, cmp_type, match);
    case HY_PKG_EVR:
    case HY_PKG_VERSION:
	return replace_filter(sack, &sltr->f_evr, keyname, cmp_type, match);
    case HY_PKG_NAME:
	if (sltr->f_provides || sltr->f_file)
	    return HY_E_SELECTOR;
	return replace_filter(sack, &sltr->f_name, keyname, cmp_type, match);
    case HY_PKG_PROVIDES:
	if (sltr->f_name || sltr->f_file)
	    return HY_E_SELECTOR;
	return replace_filter(sack, &sltr->f_provides, keyname, cmp_type, match);
    case HY_PKG_REPONAME:
        return replace_filter(sack, &sltr->f_reponame, keyname, cmp_type, match);
    case HY_PKG_FILE:
	if (sltr->f_name || sltr->f_provides)
	    return HY_E_SELECTOR;
	return replace_filter(sack, &sltr->f_file, keyname, cmp_type, match);
    default:
	return HY_E_SELECTOR;
    }
}

HyPackageList
hy_selector_matches(HySelector sltr)
{
    HySack sack = selector_sack(sltr);
    Pool *pool = sack_pool(sack);
    Queue job, solvables;

    queue_init(&job);
    sltr2job(sltr, &job, 0);

    queue_init(&solvables);
    selection_solvables(pool, &job, &solvables);

    HyPackageList plist = hy_packagelist_create();
    for (int i = 0; i < solvables.count; i++)
        hy_packagelist_push(plist, package_create(sack, solvables.elements[i]));

    queue_free(&solvables);
    queue_free(&job);
    return plist;
}
