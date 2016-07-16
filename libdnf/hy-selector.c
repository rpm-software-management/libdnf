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

#include <solv/selection.h>
#include <solv/solver.h>
#include <solv/util.h>

#include "dnf-types.h"
#include "hy-goal-private.h"
#include "hy-iutil.h"
#include "hy-package-private.h"
#include "hy-query-private.h"
#include "dnf-sack-private.h"
#include "hy-selector-private.h"
#include "hy-util.h"

static int
replace_filter(DnfSack *sack, struct _Filter **fp, int keyname, int cmp_type,
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
        DnfReldep *reldep = reldep_from_str (sack, match);
        if (reldep == NULL) {
            filter_free(*fp);
            *fp = NULL;
            return DNF_ERROR_BAD_SELECTOR;
        }
        f->matches[0].reldep = reldep;
        return 0;
    }

    f->match_type = _HY_STR;
    f->matches[0].str = g_strdup(match);
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
hy_selector_create(DnfSack *sack)
{
    HySelector sltr = g_malloc0(sizeof(*sltr));
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
    g_free(sltr);
}

int
hy_selector_set(HySelector sltr, int keyname, int cmp_type, const char *match)
{
    if (!valid_setting(keyname, cmp_type))
        return DNF_ERROR_BAD_SELECTOR;
    DnfSack *sack = selector_sack(sltr);

    switch (keyname) {
    case HY_PKG_ARCH:
        return replace_filter(sack, &sltr->f_arch, keyname, cmp_type, match);
    case HY_PKG_EVR:
    case HY_PKG_VERSION:
        return replace_filter(sack, &sltr->f_evr, keyname, cmp_type, match);
    case HY_PKG_NAME:
        if (sltr->f_provides || sltr->f_file)
            return DNF_ERROR_BAD_SELECTOR;
        return replace_filter(sack, &sltr->f_name, keyname, cmp_type, match);
    case HY_PKG_PROVIDES:
        if (sltr->f_name || sltr->f_file)
            return DNF_ERROR_BAD_SELECTOR;
        return replace_filter(sack, &sltr->f_provides, keyname, cmp_type, match);
    case HY_PKG_REPONAME:
        return replace_filter(sack, &sltr->f_reponame, keyname, cmp_type, match);
    case HY_PKG_FILE:
        if (sltr->f_name || sltr->f_provides)
            return DNF_ERROR_BAD_SELECTOR;
        return replace_filter(sack, &sltr->f_file, keyname, cmp_type, match);
    default:
        return DNF_ERROR_BAD_SELECTOR;
    }
}

GPtrArray *
hy_selector_matches(HySelector sltr)
{
    DnfSack *sack = selector_sack(sltr);
    Pool *pool = dnf_sack_get_pool(sack);
    Queue job, solvables;

    queue_init(&job);
    sltr2job(sltr, &job, 0);

    queue_init(&solvables);
    selection_solvables(pool, &job, &solvables);

    GPtrArray *plist = hy_packagelist_create();
    for (int i = 0; i < solvables.count; i++)
        g_ptr_array_add(plist, dnf_package_new(sack, solvables.elements[i]));

    queue_free(&solvables);
    queue_free(&job);
    return plist;
}
