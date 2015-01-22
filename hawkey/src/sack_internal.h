/*
 * Copyright (C) 2012-2015 Red Hat, Inc.
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

#ifndef HY_SACK_INTERNAL_H
#define HY_SACK_INTERNAL_H

#include <stdio.h>

// libsolv
#include <solv/pool.h>

// hawkey
#include "sack.h"

typedef Id(*running_kernel_fn_t)(HySack);

struct _HySack {
    Pool *pool;
    int provides_ready;
    Id running_kernel_id;
    running_kernel_fn_t running_kernel_fn;
    char *cache_dir;
    char *log_file;
    Queue installonly;
    int installonly_limit;
    FILE *log_out;
    Map *pkg_excludes;
    Map *pkg_includes;
    Map *repo_excludes;
    int considered_uptodate;
    int cmdline_repo_created;
};

void sack_make_provides_ready(HySack sack);
Id sack_running_kernel(HySack sack);
void sack_log(HySack sack, int level, const char *format, ...);
int sack_knows(HySack sack, const char *name, const char *version, int flags);
void sack_recompute_considered(HySack sack);
static inline Pool *sack_pool(HySack sack) { return sack->pool; }
static inline Id sack_last_solvable(HySack sack)
{
    return sack_pool(sack)->nsolvables - 1;
}

#endif // HY_SACK_INTERNAL_H
