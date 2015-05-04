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

#ifndef HY_QUERY_INTERNAL_H
#define HY_QUERY_INTERNAL_H

// libsolv
#include <solv/bitmap.h>

// hawkey
#include "query.h"

union _Match {
    int num;
    HyPackageSet pset;
    HyReldep reldep;
    char *str;
};

enum _match_type {
    _HY_VOID,
    _HY_NUM,
    _HY_PKG,
    _HY_RELDEP,
    _HY_STR,
};

struct _Filter {
    int cmp_type;
    int keyname;
    int match_type;
    union _Match *matches;
    int nmatches;
};

struct _HyQuery {
    HySack sack;
    int flags;
    Map *result;
    struct _Filter *filters;
    int applied;
    int nfilters;
    int downgradable; /* 1 for "only downgradable installed packages" */
    int downgrades; /* 1 for "only downgrades for installed packages" */
    int updatable; /* 1 for "only updatable installed packages" */
    int updates; /* 1 for "only updates for installed packages" */
    int latest; /* 1 for "only the latest version" */
    int latest_per_arch; /* 1 for "only the latest version per arch" */
};

struct _Filter *filter_create(int nmatches);
void filter_reinit(struct _Filter *f, int nmatches);
void filter_free(struct _Filter *f);

static inline HySack query_sack(HyQuery query) { return query->sack; }

#endif // HY_QUERY_INTERNAL_H
