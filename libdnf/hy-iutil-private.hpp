/*
 * Copyright (C) 2017 Red Hat, Inc.
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

#include "hy-iutil.h"
#include "hy-types.h"
#include "sack/packageset.hpp"

#ifndef HY_IUTIL_PRIVATE_HPP
#define HY_IUTIL_PRIVATE_HPP

/* crypto utils */
int checksum_cmp(const unsigned char *cs1, const unsigned char *cs2);
int checksum_fp(unsigned char *out, FILE *fp);
int checksum_read(unsigned char *csout, FILE *fp);
int checksum_stat(unsigned char *out, FILE *fp);
int checksum_write(const unsigned char *cs, FILE *fp);
int checksumt_l2h(int type);
const char *pool_checksum_str(Pool *pool, const unsigned char *chksum);

const char *id2nevra(Pool *pool, Id id);

/* filesystem utils */
char *abspath(const char *path);
int is_readable_rpm(const char *fn);
int mkcachedir(char *path);
gboolean mv(const char *old_path, const char *new_path, GError **error);
char *this_username(void);

/* misc utils */
char *read_whole_file(const char *path);
Id running_kernel(DnfSack *sack);

/* libsolv utils */
Repo *repo_by_name(DnfSack *sack, const char *name);
HyRepo hrepo_by_name(DnfSack *sack, const char *name);
Id str2archid(Pool *pool, const char *s);
Id what_upgrades(Pool *pool, Id p);
Id what_downgrades(Pool *pool, Id p);
Map *free_map_fully(Map *m);
int is_package(const Pool *pool, const Solvable *s);

/* package version utils */
unsigned long pool_get_epoch(Pool *pool, const char *evr);
void pool_split_evr(Pool *pool, const char *evr, char **epoch, char **version, char **release);

/* reldep utils */
int parse_reldep_str(const char *nevra, char **name, char **evr, int *cmp_type);
GPtrArray * packageSet2GPtrArray(libdnf::PackageSet * pset);

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
