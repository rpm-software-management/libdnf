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

#ifndef HY_QUERY_H
#define HY_QUERY_H

#ifdef __cplusplus
extern "C" {
#endif

/* hawkey */
#include "types.h"

enum _hy_query_flags {
    HY_IGNORE_EXCLUDES	= 1 << 0
};

void hy_query_apply(HyQuery q);
HyQuery hy_query_create(HySack sack);
HyQuery hy_query_create_flags(HySack sack, int flags);
void hy_query_free(HyQuery q);
void hy_query_clear(HyQuery q);
HyQuery hy_query_clone(HyQuery q);
int hy_query_filter(HyQuery q, int keyname, int cmp_type, const char *match);
int hy_query_filter_empty(HyQuery q);
int hy_query_filter_in(HyQuery q, int keyname, int cmp_type,
			const char **matches);
int hy_query_filter_num(HyQuery q, int keyname, int cmp_type,
			int match);
int hy_query_filter_num_in(HyQuery q, int keyname, int cmp_type, int nmatches,
			   const int *matches);
int hy_query_filter_package_in(HyQuery q, int keyname, int cmp_type,
			       const HyPackageSet pset);
int hy_query_filter_reldep(HyQuery q, int keyname, const HyReldep reldep);
int hy_query_filter_reldep_in(HyQuery q, int keyname,
			      const HyReldepList reldeplist);
int hy_query_filter_provides(HyQuery q, int cmp_type, const char *name,
			     const char *evr);
int hy_query_filter_provides_in(HyQuery q, char **reldep_strs);
int hy_query_filter_requires(HyQuery q, int cmp_type, const char *name,
			     const char *evr);

/**
 * Filter packages that are installed and have higher version than other not
 * installed packages that are named same.
 *
 * NOTE: this does not guarantee packages filtered in this way are downgradable.
 */
void hy_query_filter_downgradable(HyQuery q, int val);
/**
 * Filter packages that are named same as an installed package but lower version.
 *
 * NOTE: this does not guarantee packages filtered in this way are installable.
 */
void hy_query_filter_downgrades(HyQuery q, int val);
/**
 * Filter packages that are installed and have lower version than other not
 * installed packages that are named same.
 *
 * NOTE: this does not guarantee packages filtered in this way are upgradable.
 */
void hy_query_filter_upgradable(HyQuery q, int val);
/**
 * Filter packages that are named same as an installed package but higher version.
 *
 * NOTE: this does not guarantee packages filtered in this way are installable.
 */
void hy_query_filter_upgrades(HyQuery q, int val);
void hy_query_filter_latest_per_arch(HyQuery q, int val);
void hy_query_filter_latest(HyQuery q, int val);

HyPackageList hy_query_run(HyQuery q);
HyPackageSet hy_query_run_set(HyQuery q);


#ifdef __cplusplus
}
#endif

#endif /* HY_QUERY_H */
