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

#ifndef HY_IUTIL_H
#define HY_IUTIL_H

#include <regex.h>
#include <solv/bitmap.h>
#include <solv/queue.h>
#include <solv/repo.h>
#include <solv/rules.h>
#include <solv/transaction.h>

#include "dnf-sack.h"

#define CHKSUM_BYTES 32

G_BEGIN_DECLS

/* crypto utils */
int checksum_type2length(int type);

/* filesystem utils */
char *abspath(const char *path);
int is_readable_rpm(const char *fn);
int mkcachedir(char *path);
gboolean mv(const char *old_path, const char *new_path, GError **error);
char *this_username(void);

/* misc utils */
char *read_whole_file(const char *path);
char *pool_tmpdup(Pool *pool, const char *s);
Id running_kernel(DnfSack *sack);

/* libsolv utils */
int cmptype2relflags(int type);
Repo *repo_by_name(DnfSack *sack, const char *name);
HyRepo hrepo_by_name(DnfSack *sack, const char *name);
Id str2archid(Pool *pool, const char *s);
void queue2plist(DnfSack *sack, Queue *q, GPtrArray *plist);
Id what_upgrades(Pool *pool, Id p);
Id what_downgrades(Pool *pool, Id p);
Map *free_map_fully(Map *m);
int is_package(const Pool *pool, const Solvable *s);

/* package version utils */
unsigned long pool_get_epoch(Pool *pool, const char *evr);
void pool_split_evr(Pool *pool, const char *evr, char **epoch, char **version,
                        char **release);

/* reldep utils */
int copy_str_from_subexpr(char** target, const char* source,
    regmatch_t* matches, int i);
int parse_reldep_str(const char *nevra, char **name,
        char **evr, int *cmp_type);
DnfReldep *reldep_from_str(DnfSack *sack, const char *reldep_str);
DnfReldepList *reldeplist_from_str(DnfSack *sack, const char *reldep_str);

/* advisory utils */

/* debug utils */
int dump_solvables_queue(Pool *pool, Queue *q);
const char *id2nevra(Pool *pool, Id id);

/* loop over all package providers of d */
#define FOR_PKG_PROVIDES(v, vp, d)                                      \
    FOR_PROVIDES(v, vp, d)                                              \
        if (!is_package(pool, pool_id2solvable(pool, v)))               \
            continue;                                                   \
        else

/* loop over all package solvables */
#define FOR_PKG_SOLVABLES(p)                                            \
    FOR_POOL_SOLVABLES(p)                                               \
        if (!is_package(pool, pool_id2solvable(pool, p)))               \
            continue;                                                   \
        else

G_END_DECLS

#endif
