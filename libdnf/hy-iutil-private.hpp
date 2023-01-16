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

#ifndef HY_IUTIL_PRIVATE_HPP
#define HY_IUTIL_PRIVATE_HPP

#include "hy-iutil.h"
#include "hy-types.h"
#include "sack/packageset.hpp"
#include <array>
#include <utility>

// Use 8 bytes for libsolv version (API: solv_toolversion)
// to be future proof even though it currently is "1.2"
static constexpr const size_t solv_userdata_solv_toolversion_size{8};
static constexpr const std::array<char, 4> solv_userdata_magic{'\0', 'd', 'n', 'f'};
static constexpr const std::array<char, 4> solv_userdata_dnf_version{'\0', '1', '.', '0'};

static constexpr const int solv_userdata_size = solv_userdata_solv_toolversion_size + \
                                                   solv_userdata_magic.size() + \
                                                   solv_userdata_dnf_version.size() + \
                                                   CHKSUM_BYTES;

struct SolvUserdata {
    char dnf_magic[solv_userdata_magic.size()];
    char dnf_version[solv_userdata_dnf_version.size()];
    char libsolv_version[solv_userdata_solv_toolversion_size];
    unsigned char checksum[CHKSUM_BYTES];
}__attribute__((packed)); ;

int solv_userdata_fill(SolvUserdata *solv_userdata, const unsigned char *checksum, GError** error);
std::unique_ptr<SolvUserdata, decltype(solv_free)*> solv_userdata_read(FILE *fp);
int solv_userdata_verify(const SolvUserdata *solv_userdata, const unsigned char *checksum);

/* crypto utils */
int checksum_cmp(const unsigned char *cs1, const unsigned char *cs2);
int checksum_fp(unsigned char *out, FILE *fp);
int checksum_stat(unsigned char *out, FILE *fp);
int checksumt_l2h(int type);
const char *pool_checksum_str(Pool *pool, const unsigned char *chksum);

const char *id2nevra(Pool *pool, Id id);

/* filesystem utils */
char *abspath(const char *path);
int is_readable_rpm(const char *fn);
int mkcachedir(char *path);
gboolean mv(const char *old_path, const char *new_path, GError **error);
gboolean dnf_remove_recursive_v2(const gchar *path, GError **error);
gboolean dnf_copy_file(const std::string & srcPath, const std::string & dstPath, GError ** error);
gboolean dnf_copy_recursive(const std::string & srcPath, const std::string & dstPath, GError ** error);
gboolean dnf_move_recursive(const gchar *src_dir, const gchar *dst_dir, GError **error);
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
