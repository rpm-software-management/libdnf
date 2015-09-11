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

// libsolv
#include <solv/bitmap.h>
#include <solv/queue.h>
#include <solv/repo.h>
#include <solv/rules.h>
#include <solv/transaction.h>

// hawkey
#include "advisory_internal.h"
#include "packagelist.h"
#include "sack.h"

#define CHKSUM_BYTES 32

// log levels (see also SOLV_ERROR etc. in <solv/pool.h>)
#define HY_LL_INFO  (1 << 20)
#define HY_LL_ERROR (1 << 21)

#define HY_LOG_INFO(...) sack_log(sack, HY_LL_INFO, __VA_ARGS__)
#define HY_LOG_ERROR(...) sack_log(sack, HY_LL_ERROR, __VA_ARGS__)

/* crypto utils */
int checksum_cmp(const unsigned char *cs1, const unsigned char *cs2);
int checksum_fp(unsigned char *out, FILE *fp);
int checksum_read(unsigned char *csout, FILE *fp);
int checksum_stat(unsigned char *out, FILE *fp);
int checksum_write(const unsigned char *cs, FILE *fp);
void checksum_dump(const unsigned char *cs);
int checksum_type2length(int type);
int checksumt_l2h(int type);
const char *pool_checksum_str(Pool *pool, const unsigned char *chksum);

/* filesystem utils */
char *abspath(const char *path);
int is_readable_rpm(const char *fn);
int mkcachedir(char *path);
int mv(HySack sack, const char *old, const char *new);
char *this_username(void);

/* misc utils */
unsigned count_nullt_array(const char **a);
const char *ll_name(int level);
char *read_whole_file(const char *path);
int str_endswith(const char *haystack, const char *needle);
int str_startswith(const char *haystack, const char *needle);
char *pool_tmpdup(Pool *pool, const char *s);
char *hy_strndup(const char *s, size_t n);
Id running_kernel(HySack sack);

/* libsolv utils */
int cmptype2relflags(int type);
Repo *repo_by_name(HySack sack, const char *name);
HyRepo hrepo_by_name(HySack sack, const char *name);
Id str2archid(Pool *pool, const char *s);
void queue2plist(HySack sack, Queue *q, HyPackageList plist);
Id what_upgrades(Pool *pool, Id p);
Id what_downgrades(Pool *pool, Id p);
static inline int is_package(Pool *pool, Solvable *s)
{
    return !str_startswith(pool_id2str(pool, s->name), SOLVABLE_NAME_ADVISORY_PREFIX);
}

/* package version utils */
unsigned long pool_get_epoch(Pool *pool, const char *evr);
void pool_split_evr(Pool *pool, const char *evr, char **epoch, char **version,
			char **release);

/* reldep utils */
int copy_str_from_subexpr(char** target, const char* source,
    regmatch_t* matches, int i);
int parse_reldep_str(const char *nevra, char **name,
	char **evr, int *cmp_type);
HyReldep reldep_from_str(HySack sack, const char *reldep_str);
HyReldepList reldeplist_from_str(HySack sack, const char *reldep_str);

/* debug utils */
int dump_jobqueue(Pool *pool, Queue *job);
int dump_nullt_array(const char **a);
int dump_solvables_queue(Pool *pool, Queue *q);
int dump_map(Pool *pool, Map *m);
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

#endif
